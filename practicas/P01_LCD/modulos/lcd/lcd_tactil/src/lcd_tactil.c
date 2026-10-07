/**
 * @file    lcd_tactil.c
 * @brief   Librería del panel táctil de la LCD-PAR-S035 (controlador GT911 en el bus LPI2C2), en
 * modo inmediato: se le pregunta y contesta.
 *
 * @details El trabajo con el GT911 (secuencia de arranque, lectura de su configuración, registro
 * de estado, registros de los puntos de contacto) lo hace el driver fsl_gt911 del SDK. Ese driver
 * no sabe nada del microcontrolador: pide que se le den, en una `gt911_config_t`, cinco funciones
 * que sí lo saben —enviar por I2C, recibir por I2C, esperar unos milisegundos, mover el pin INT y
 * mover el pin de reset—. Este módulo escribe esas cinco funciones para el MCX A156, con el driver
 * fsl_lpi2c, y encima ofrece tres funciones sencillas para el programa.
 *
 * Tres detalles que conviene conocer, todos sacados del ejemplo LVGL de la placa
 * (lvgl_support.c):
 *  - **Reset compartido.** El GT911 y el ST7796S comparten la línea de reset (P3_0). La da
 *    lcd_panel_inicializar(), y mientras dura sujeta el pin INT del GT911 a nivel bajo: así el
 *    GT911 elige la dirección I2C 0x5D (kGT911_I2cAddrMode0). Por eso lcd_tactil_inicializar()
 *    empieza llamando a lcd_panel_inicializar() (que no hace nada si ya se había llamado) y la
 *    función de reset que se le da al driver no hace nada.
 *  - **Sin interrupción.** El GT911 avisa por INT de que tiene datos nuevos, pero aquí no se
 *    usa: lcd_tactil_leer() pregunta directamente. Si el GT911 no tiene un dato nuevo desde la
 *    última lectura (lo renueva unas cien veces por segundo), se devuelve el último conocido.
 *  - **Coordenadas giradas y escaladas.** El GT911 da las coordenadas en la orientación natural
 *    del panel, vertical. lcd_panel pone el ST7796S en kST7796S_Orientation270, apaisado, así
 *    que se giran igual que hace el ejemplo para esa orientación: x = y_gt911 e
 *    y = resolución_x - x_gt911. Además se escalan a 480 x 320 con la resolución que el GT911
 *    tiene guardada en su configuración (la lee GT911_Init): el ejemplo de NXP no escala porque
 *    su GT911 trae 320 x 480, pero hay paneles que traen 240 x 320 y, sin escalar, sólo se
 *    podría tocar en una esquina de 320 x 240.
 *
 * @author      Alejandro Lara Doña - alejandro.lara@uca.es
 * @date        2026
 * @version     1.0
 *
 * @copyright   GNU General Public License version 3 or later
 */

#include "lcd_tactil.h"
#include "fsl_common.h"   // HAL del fabricante: relojes, reset, SDK_DelayAtLeastUs
#include "fsl_port.h"     // HAL del fabricante: multiplexado de pines
#include "fsl_lpi2c.h"    // HAL del fabricante: bus I2C
#include "fsl_gt911.h"    // Componente del SDK: controlador táctil GT911
#include "tipos.h"
#include "lcd_panel.h"

// ===== LCD_TACTIL - Macros Privadas =====
/**
 * @defgroup  Lcd_Tactil_Macros_Privadas LCD_TACTIL - Macros Privadas
 * @ingroup   Lcd_Tactil
 * @brief     Recursos del MCX A156 que usa el táctil (de lvgl_support_board.h y pin_mux.c).
 * @{
 */
#define LCD_TACTIL_I2C          LPI2C2   //!< Interfaz I2C del táctil (BOARD_TOUCH_I2C).
#define LCD_TACTIL_I2C_ID       2u       //!< Su número, para CLOCK_GetLpi2cClkFreq().
#define LCD_TACTIL_PORT_I2C     PORT1    //!< Puerto de SDA y SCL.
#define LCD_TACTIL_PIN_SDA      8u       //!< P1_8 = LPI2C2_SDA (alternativa 3).
#define LCD_TACTIL_PIN_SCL      9u       //!< P1_9 = LPI2C2_SCL (alternativa 3).
#define LCD_TACTIL_PORT_INT     PORT2    //!< Pin INT del GT911: P2_15.
#define LCD_TACTIL_PIN_INT      15u
#define LCD_TACTIL_PUNTOS       5u       //!< Puntos de contacto que se piden al GT911.
//!@}

// ===== LCD_TACTIL - Variables Privadas =====
//! Estado del driver del GT911 (lo rellena GT911_Init).
static gt911_handle_t lcd_tactil_gt911;

//! TRUE si el GT911 respondió en lcd_tactil_inicializar().
static bool_t lcd_tactil_inicializado = FALSE;

//! Último estado conocido: ¿hay un dedo sobre la pantalla?
static bool_t lcd_tactil_ultimo_pulsado = FALSE;

//! Últimas coordenadas conocidas, ya giradas.
static uint16_t lcd_tactil_ultimo_x = 0;
static uint16_t lcd_tactil_ultimo_y = 0;

// ===== LCD_TACTIL - Funciones Privadas =====
/**
 * @brief   Indica si el LPI2C2 ya está en marcha (inicializado por otro módulo).
 * @ingroup Lcd_Tactil
 * @private
 *
 * @details Se consideran tres cosas, por este orden: que el reloj del LPI2C2 esté habilitado
 * (bit LPI2C2 de MRCC_GLB_CC1), que el periférico esté fuera de reset (bit LPI2C2 de
 * MRCC_GLB_RST1, que a 1 significa «liberado») y que el maestro esté habilitado (bit MEN de su
 * registro MCR, que pone a 1 LPI2C_MasterInit()). El orden importa: leer un registro de un
 * periférico sin reloj provoca un fallo de bus, así que MCR sólo se lee si lo anterior se cumple.
 */
static bool_t lcd_tactil_bus_en_marcha(void) {

  if ((MRCC0->MRCC_GLB_CC1 & MRCC_MRCC_GLB_CC1_LPI2C2_MASK) == 0u) {
    return FALSE;
  }
  if ((MRCC0->MRCC_GLB_RST1 & MRCC_MRCC_GLB_RST1_LPI2C2_MASK) == 0u) {
    return FALSE;
  }
  return (LCD_TACTIL_I2C->MCR & LPI2C_MCR_MEN_MASK) != 0u;
}

/**
 * @brief   Inicializa el LPI2C2 como maestro a LCD_TACTIL_I2C_FRECUENCIA_HZ, con sus pines, si
 * nadie lo ha hecho antes.
 * @ingroup Lcd_Tactil
 * @private
 */
static void lcd_tactil_inicializar_bus(void) {

  const port_pin_config_t configuracion_pin = {
    .pullSelect          = kPORT_PullUp,              // Las resistencias del bus I2C
    .pullValueSelect     = kPORT_LowPullResistor,
    .slewRate            = kPORT_FastSlewRate,
    .passiveFilterEnable = kPORT_PassiveFilterDisable,
    .openDrainEnable     = kPORT_OpenDrainEnable,     // Bus en Y: drenador abierto
    .driveStrength       = kPORT_LowDriveStrength,
#if defined(FSL_FEATURE_PORT_HAS_DRIVE_STRENGTH1) && FSL_FEATURE_PORT_HAS_DRIVE_STRENGTH1
    .driveStrength1      = kPORT_NormalDriveStrength,
#endif
    .mux                 = kPORT_MuxAlt3,             // LPI2C2_SDA / LPI2C2_SCL
    .inputBuffer         = kPORT_InputBufferEnable,
    .invertInput         = kPORT_InputNormal,
    .lockRegister        = kPORT_UnlockRegister
  };
  lpi2c_master_config_t configuracion_i2c;

  if (lcd_tactil_bus_en_marcha()) {
    return;
  }

  /* Reloj funcional del LPI2C2: FRO de 12 MHz sin dividir, como en BOARD_InitHardware() del
   * ejemplo de la placa.
   */
  CLOCK_SetClockDiv(kCLOCK_DivLPI2C2, 1u);
  CLOCK_AttachClk(kFRO12M_to_LPI2C2);

  /* Pines SDA y SCL. */
  CLOCK_EnableClock(kCLOCK_GatePORT1);
  RESET_ReleasePeripheralReset(kPORT1_RST_SHIFT_RSTn);
  PORT_SetPinConfig(LCD_TACTIL_PORT_I2C, LCD_TACTIL_PIN_SDA, &configuracion_pin);
  PORT_SetPinConfig(LCD_TACTIL_PORT_I2C, LCD_TACTIL_PIN_SCL, &configuracion_pin);

  /* El maestro I2C. LPI2C_MasterInit() habilita el reloj de la interfaz, la saca del reset y
   * calcula los divisores a partir de la frecuencia de su reloj funcional.
   */
  LPI2C_MasterGetDefaultConfig(&configuracion_i2c);
  configuracion_i2c.baudRate_Hz = LCD_TACTIL_I2C_FRECUENCIA_HZ;
  LPI2C_MasterInit(LCD_TACTIL_I2C, &configuracion_i2c, CLOCK_GetLpi2cClkFreq(LCD_TACTIL_I2C_ID));
}

/**
 * @brief   Escritura I2C para el driver del GT911: START, dirección+W, sub-dirección (el número
 * de registro, de 2 bytes en el GT911), datos y STOP, en una sola transferencia.
 * @ingroup Lcd_Tactil
 * @private
 */
static status_t lcd_tactil_i2c_enviar(uint8_t direccion, uint32_t subdireccion,
                                      uint8_t tamano_subdireccion, const uint8_t *datos,
                                      uint8_t n) {

  lpi2c_master_transfer_t transferencia;

  transferencia.flags = (uint32_t) kLPI2C_TransferDefaultFlag;
  transferencia.slaveAddress = direccion;
  transferencia.direction = kLPI2C_Write;
  transferencia.subaddress = subdireccion;
  transferencia.subaddressSize = tamano_subdireccion;
  transferencia.data = (void *) datos;  // El driver no la modifica en una escritura
  transferencia.dataSize = n;

  return LPI2C_MasterTransferBlocking(LCD_TACTIL_I2C, &transferencia);
}

/**
 * @brief   Lectura I2C para el driver del GT911: START, dirección+W, sub-dirección, REPEATED
 * START, dirección+R, datos y STOP, en una sola transferencia.
 * @ingroup Lcd_Tactil
 * @private
 */
static status_t lcd_tactil_i2c_recibir(uint8_t direccion, uint32_t subdireccion,
                                       uint8_t tamano_subdireccion, uint8_t *datos, uint8_t n) {

  lpi2c_master_transfer_t transferencia;

  transferencia.flags = (uint32_t) kLPI2C_TransferDefaultFlag;
  transferencia.slaveAddress = direccion;
  transferencia.direction = kLPI2C_Read;
  transferencia.subaddress = subdireccion;
  transferencia.subaddressSize = tamano_subdireccion;
  transferencia.data = datos;
  transferencia.dataSize = n;

  return LPI2C_MasterTransferBlocking(LCD_TACTIL_I2C, &transferencia);
}

/**
 * @brief   Espera activa de unos milisegundos, para el driver del GT911 (sólo la usa GT911_Init).
 * @ingroup Lcd_Tactil
 * @private
 */
static void lcd_tactil_esperar_ms(uint32_t ms) {
  SDK_DelayAtLeastUs(ms * 1000u, SystemCoreClock);
}

/**
 * @brief   Configura el pin INT del GT911 como entrada con *pull-down*, con *pull-up* o sin
 * resistencias, según pida el driver del GT911.
 * @ingroup Lcd_Tactil
 * @private
 */
static void lcd_tactil_pin_int(gt911_int_pin_mode_t modo) {

  port_pin_config_t configuracion = {
    .pullSelect          = kPORT_PullDisable,
    .pullValueSelect     = kPORT_LowPullResistor,
    .slewRate            = kPORT_FastSlewRate,
    .passiveFilterEnable = kPORT_PassiveFilterDisable,
    .openDrainEnable     = kPORT_OpenDrainDisable,
    .driveStrength       = kPORT_LowDriveStrength,
#if defined(FSL_FEATURE_PORT_HAS_DRIVE_STRENGTH1) && FSL_FEATURE_PORT_HAS_DRIVE_STRENGTH1
    .driveStrength1      = kPORT_NormalDriveStrength,
#endif
    .mux                 = kPORT_MuxAlt0,
    .inputBuffer         = kPORT_InputBufferEnable,
    .invertInput         = kPORT_InputNormal,
    .lockRegister        = kPORT_UnlockRegister
  };

  switch (modo) {
    case kGT911_IntPinPullUp:
      configuracion.pullSelect = kPORT_PullUp;
      break;
    case kGT911_IntPinPullDown:
      configuracion.pullSelect = kPORT_PullDown;
      break;
    case kGT911_IntPinInput:
    default:
      configuracion.pullSelect = kPORT_PullDisable;
      break;
  }

  PORT_SetPinConfig(LCD_TACTIL_PORT_INT, LCD_TACTIL_PIN_INT, &configuracion);
}

/**
 * @brief   Función de reset para el driver del GT911: no hace nada, porque el reset es el del
 * ST7796S y ya lo ha dado lcd_panel_inicializar() (ver el comentario de cabecera).
 * @ingroup Lcd_Tactil
 * @private
 */
static void lcd_tactil_pin_reset(bool nivel_alto) {
  (void) nivel_alto;
}

// ===== LCD_TACTIL - Funciones Publicas =====
bool_t lcd_tactil_inicializar(void) {

  const gt911_config_t configuracion = {
    .I2C_SendFunc     = lcd_tactil_i2c_enviar,
    .I2C_ReceiveFunc  = lcd_tactil_i2c_recibir,
    .timeDelayMsFunc  = lcd_tactil_esperar_ms,
    .intPinFunc       = lcd_tactil_pin_int,
    .pullResetPinFunc = lcd_tactil_pin_reset,
    .touchPointNum    = LCD_TACTIL_PUNTOS,       // Como el ejemplo: así GT911_Init no tiene que
    .i2cAddrMode      = kGT911_I2cAddrMode0,     // reescribir la configuración del GT911.
    .intTrigMode      = kGT911_IntFallingEdge
  };

  if (lcd_tactil_inicializado) {
    return TRUE;
  }

  /* 1. El reset del GT911 lo da lcd_panel (idempotente). */
  lcd_panel_inicializar();

  /* 2. El bus I2C, si nadie lo ha puesto en marcha. */
  lcd_tactil_inicializar_bus();

  /* 3. El GT911: el driver comprueba su identificador ("911"), lee su configuración y, entre
   * otras cosas, su resolución, que hace falta para girar las coordenadas.
   */
  lcd_tactil_inicializado = (GT911_Init(&lcd_tactil_gt911, &configuracion) == kStatus_Success);

  return lcd_tactil_inicializado;
}

bool_t lcd_tactil_leer(uint16_t *x, uint16_t *y) {

  int x_gt911, y_gt911;   // El driver del GT911 usa int
  int32_t x_pantalla, y_pantalla;
  status_t estado;

  if (!lcd_tactil_inicializado) {
    return FALSE;
  }

  estado = GT911_GetSingleTouch(&lcd_tactil_gt911, &x_gt911, &y_gt911);

  if (estado == kStatus_Success && lcd_tactil_gt911.resolutionX > 0u &&
      lcd_tactil_gt911.resolutionY > 0u) {
    /* Giro para kST7796S_Orientation270 (el mismo que en DEMO_ReadTouch del ejemplo) y escala a
     * la pantalla: el GT911 da las coordenadas en la resolución que tiene guardada en su
     * configuración, que no tiene por qué ser la de la pantalla (en algunos paneles es 240 x 320
     * en lugar de 320 x 480). Si coinciden, la escala no cambia nada. Después, recorte a la
     * pantalla por si el GT911 da el valor del borde.
     */
    x_pantalla = ((int32_t) y_gt911 * (int32_t) LCD_PANEL_ANCHO) /
                 (int32_t) lcd_tactil_gt911.resolutionY;
    y_pantalla = (((int32_t) lcd_tactil_gt911.resolutionX - x_gt911) * (int32_t) LCD_PANEL_ALTO) /
                 (int32_t) lcd_tactil_gt911.resolutionX;
    x_pantalla = CONSTRAIN(x_pantalla, 0, (int32_t) LCD_PANEL_ANCHO - 1);
    y_pantalla = CONSTRAIN(y_pantalla, 0, (int32_t) LCD_PANEL_ALTO - 1);

    lcd_tactil_ultimo_x = (uint16_t) x_pantalla;
    lcd_tactil_ultimo_y = (uint16_t) y_pantalla;
    lcd_tactil_ultimo_pulsado = TRUE;
  } else if (estado == (status_t) kStatus_TOUCHPANEL_NotTouched) {
    lcd_tactil_ultimo_pulsado = FALSE;
  } else {
    /* kStatus_Fail: el GT911 no tiene un dato nuevo desde la última lectura (o el bus ha
     * fallado). Vale lo último que se supo.
     */
  }

  if (lcd_tactil_ultimo_pulsado) {
    if (x != NULL) {
      *x = lcd_tactil_ultimo_x;
    }
    if (y != NULL) {
      *y = lcd_tactil_ultimo_y;
    }
  }

  return lcd_tactil_ultimo_pulsado;
}

bool_t lcd_tactil_pulsado(void) {
  return lcd_tactil_leer(NULL, NULL);
}
