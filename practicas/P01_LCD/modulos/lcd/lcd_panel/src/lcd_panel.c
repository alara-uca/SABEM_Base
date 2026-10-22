/**
 * @file    lcd_panel.c
 * @brief   Capa más baja de la pantalla LCD-PAR-S035: bus paralelo 8080 fabricado con el FlexIO y
 * controlador ST7796S.
 *
 * @details Este módulo sigue paso a paso lo que hace el soporte de LVGL del SDK para esta placa
 * (examples/lvgl_examples/lvgl_sdk/lvgl_support/mcx_flexio/lvgl_support.c), pero sin LVGL: deja
 * la pantalla lista y ofrece dos operaciones, "elegir una ventana" y "volcar píxeles en ella".
 *
 * La cadena de software que hay debajo es de cuatro pisos, y conviene tenerla clara:
 *
 *  1. **FlexIO (fsl_flexio_mculcd)**. El MCX A156 no tiene controlador de LCD. El driver
 *     FLEXIO_MCULCD programa cuatro desplazadores y un temporizador del FLEXIO0 para que generen
 *     las formas de onda de un bus paralelo 8080 de 16 líneas de datos (D0..D15), con su línea de
 *     escritura WR y su línea de lectura RD. Las líneas CS (selección) y RS (dato/orden) no las
 *     mueve el FlexIO: son GPIO normales y el driver las gobierna llamando a dos funciones
 *     nuestras, lcd_panel_pin_cs() y lcd_panel_pin_rs().
 *  2. **Adaptador DBI (fsl_dbi_flexio_edma)**. DBI (*Display Bus Interface*) es el nombre que da
 *     el MIPI a este tipo de bus. El adaptador presenta el FlexIO con una interfaz genérica
 *     —"escribe una orden", "escribe datos", "escribe en la memoria de imagen"— que es la que
 *     entienden los drivers de controladores de pantalla del SDK. Se crea SIN canales de eDMA
 *     (los dos últimos argumentos a NULL), igual que en el ejemplo de la placa: entonces cada
 *     escritura en memoria se hace con la CPU, de forma bloqueante, y el aviso de "escritura
 *     terminada" llega antes de que vuelva la llamada.
 *  3. **Driver del ST7796S (fsl_st7796s)**. Envía la secuencia de arranque del controlador y
 *     traduce "ventana" a las órdenes CASET/RASET y "píxeles" a la orden RAMWR.
 *  4. **Este módulo**. Añade lo que necesitan las bibliotecas de la asignatura: inicialización
 *     idempotente, espera al final de la escritura, relleno con un color sin *framebuffer* y un
 *     cerrojo para que dos tareas no mezclen sus píxeles en el bus.
 *
 * El ST7796S tiene su propia memoria de imagen (320 x 480 x 18 bits): lo que se escribe en ella
 * se queda en pantalla hasta que se vuelva a escribir. Por eso no hace falta *framebuffer* en el
 * micro.
 *
 * @author    Alejandro Lara Doña - alejandro.lara(at)uca.es
 * @date      2026
 * @version   1.0
 *
 * @copyright GNU General Public License version 3 or later
 */

#include "lcd_panel.h"
#include "fsl_common.h"           // HAL del fabricante: relojes, reset, SDK_DelayAtLeastUs
#include "fsl_dbi_flexio_edma.h"  // Componente del SDK: adaptador DBI sobre FLEXIO_MCULCD
#include "fsl_flexio_mculcd.h"    // HAL del fabricante: bus 8080 sobre FlexIO
#include "fsl_gpio.h"             // HAL del fabricante: GPIO
#include "fsl_port.h"             // HAL del fabricante: multiplexado de pines
#include "fsl_st7796s.h"          // Componente del SDK: controlador de pantalla ST7796S
#include "tipos.h"

#if LCD_PANEL_CON_FREERTOS
  #include "FreeRTOS.h"
  #include "semphr.h"
  #include "task.h"
#endif

// ===== LCD_PANEL - Macros Privadas =====
/**
 * @defgroup  LCD_Panel_Macros_Privadas LCD_PANEL - Macros Privadas
 * @ingroup   LCD_Panel
 * @brief     Recursos del MCX A156 que ocupa la pantalla.
 *
 * @details Copiados de lvgl_support_board.h y de pin_mux.c de los ejemplos LVGL de la
 * FRDM-MCXA156 (SDK 26.06). Se repiten aquí para que el módulo no dependa de los ficheros de un
 * ejemplo concreto.
 * @{
 */
#define LCD_PANEL_GPIO_CS         GPIO2  //!< Selección del controlador (CS): P2_19.
#define LCD_PANEL_PIN_CS          19u
#define LCD_PANEL_GPIO_RS         GPIO2  //!< Dato (1) u orden (0) (RS, también D/C): P2_17.
#define LCD_PANEL_PIN_RS          17u
#define LCD_PANEL_GPIO_RST        GPIO3  //!< Reset del ST7796S Y del GT911: P3_0 (¡y LED azul!).
#define LCD_PANEL_PIN_RST         0u
#define LCD_PANEL_GPIO_BLK        GPIO3  //!< Retroiluminación (BLK), activa a nivel alto: P3_22.
#define LCD_PANEL_PIN_BLK         22u
#define LCD_PANEL_PORT_INT_TACTIL PORT2  //!< Pin INT del táctil GT911: P2_15.
#define LCD_PANEL_GPIO_INT_TACTIL GPIO2
#define LCD_PANEL_PIN_INT_TACTIL  15u

#define LCD_PANEL_FLEXIO          FLEXIO0     //!< Instancia de FlexIO que fabrica el bus.
#define LCD_PANEL_FLEXIO_BPS      160000000u  //!< Velocidad del bus (BOARD_FLEXIO_BAUDRATE_BPS).
#define LCD_PANEL_FLEXIO_PIN_D0   0u          //!< FXIO_D0 es la línea de datos D0 del bus.
#define LCD_PANEL_FLEXIO_PIN_WR   31u         //!< FXIO_D31 (P2_23) es WR.
#define LCD_PANEL_FLEXIO_PIN_RD   28u         //!< FXIO_D28 (P2_20) es RD.
#define LCD_PANEL_FLEXIO_TX_INI   0u          //!< Desplazadores 0..3 para escribir...
#define LCD_PANEL_FLEXIO_TX_FIN   3u
#define LCD_PANEL_FLEXIO_RX_INI   0u  //!< ... y los mismos para leer.
#define LCD_PANEL_FLEXIO_RX_FIN   3u
#define LCD_PANEL_FLEXIO_TIMER    0u  //!< Temporizador 0 del FlexIO.

#define LCD_PANEL_PIXELES_BUFER   LCD_PANEL_ANCHO  //!< Búfer de lcd_panel_repetir_color().
//!@}

// ===== LCD_PANEL - Tipos Privados =====
/**
 * @brief   Un pin de un puerto, para las tablas de multiplexado.
 * @ingroup LCD_Panel
 * @private
 */
typedef struct {
  PORT_Type *puerto;  //!< Puerto al que pertenece el pin.
  uint8_t    pin;     //!< Número de pin dentro del puerto.
} lcd_panel_pin_t;

// ===== LCD_PANEL - Constantes Privadas =====
/**
 * @brief   Pines que pasan a la función FLEXIO0_Dn (función alternativa 6 en todos ellos).
 * @ingroup LCD_Panel
 * @private
 */
static const lcd_panel_pin_t kLCDPanelPinesFlexio[] = {
    {PORT0, 16u}, {PORT0, 17u}, {PORT0, 18u}, {PORT0, 19u},  // D0..D3   = FXIO_D0..D3
    {PORT0, 20u}, {PORT0, 21u}, {PORT0, 22u}, {PORT0, 23u},  // D4..D7   = FXIO_D4..D7
    {PORT2, 0u},  {PORT2, 1u},                               // D8, D9   = FXIO_D8, D9
    {PORT4, 2u},  {PORT4, 3u},  {PORT4, 4u},  {PORT4, 5u},   // D10..D13 = FXIO_D10..D13
    {PORT4, 6u},  {PORT4, 7u},                               // D14, D15 = FXIO_D14, D15
    {PORT2, 20u},                                            // RD       = FXIO_D28
    {PORT2, 23u}                                             // WR       = FXIO_D31
};

/**
 * @brief   Pines que se usan como GPIO (función alternativa 0): CS, RS, RST y BLK.
 * @ingroup LCD_Panel
 * @private
 */
static const lcd_panel_pin_t kLCDPanelPinesGPIO[] = {{PORT2, LCD_PANEL_PIN_CS},
                                                     {PORT2, LCD_PANEL_PIN_RS},
                                                     {PORT3, LCD_PANEL_PIN_RST},
                                                     {PORT3, LCD_PANEL_PIN_BLK}};

/**
 * @brief   Tiempo de espera tras la señal de reset, en microsegundos.
 * @ingroup LCD_Panel
 * @private
 */
static const uint32_t kLCDPanelEsperaUs  = 1000u;
static const uint32_t kLCDTactilEsperaUs = 5000u;

// ===== LCD_PANEL - Prototipos de Funciones Privadas =====
static void lcd_panel_pin_cs(bool set);
static void lcd_panel_pin_rs(bool set);

// ===== LCD_PANEL - Variables Privadas =====
/**
 * @brief   Descripción del bus 8080 para el driver FLEXIO_MCULCD: qué pines, qué desplazadores y
 * qué temporizador del FlexIO usa, y qué funciones mueven CS y RS.
 * @ingroup LCD_Panel
 * @private
 */
static FLEXIO_MCULCD_Type g_lcd_panel_bus = {
    .flexioBase          = LCD_PANEL_FLEXIO,
    .busType             = kFLEXIO_MCULCD_8080,
    .dataPinStartIndex   = LCD_PANEL_FLEXIO_PIN_D0,
    .ENWRPinIndex        = LCD_PANEL_FLEXIO_PIN_WR,
    .RDPinIndex          = LCD_PANEL_FLEXIO_PIN_RD,
    .txShifterStartIndex = LCD_PANEL_FLEXIO_TX_INI,
    .txShifterEndIndex   = LCD_PANEL_FLEXIO_TX_FIN,
    .rxShifterStartIndex = LCD_PANEL_FLEXIO_RX_INI,
    .rxShifterEndIndex   = LCD_PANEL_FLEXIO_RX_FIN,
    .timerIndex          = LCD_PANEL_FLEXIO_TIMER,
    .setCSPin            = lcd_panel_pin_cs,
    .setRSPin            = lcd_panel_pin_rs,
    .setRDWRPin          = NULL  // Sólo se usa en los buses 6800.
};

//! Estado del adaptador DBI (lo rellena DBI_FLEXIO_EDMA_CreateXferHandle).
static dbi_flexio_edma_xfer_handle_t g_lcd_panel_dbi;

//! Estado del driver del ST7796S (lo rellena ST7796S_Init).
static st7796s_handle_t g_lcd_panel_st7796s;

//! TRUE cuando lcd_panel_inicializar() ya se ha ejecutado una vez.
static bool_t g_lcd_panel_inicializado = FALSE;

//! Lo pone a TRUE lcd_panel_memoria_escrita() al acabar cada escritura en la memoria de imagen.
static volatile bool_t g_lcd_panel_escritura_terminada = TRUE;

//! Función de aviso pendiente de una escritura asíncrona (NULL si no hay ninguna).
static volatile lcd_panel_fin_t g_lcd_panel_fin_usuario = NULL;

//! Argumento para g_lcd_panel_fin_usuario.
static void *volatile g_lcd_panel_arg_usuario = NULL;

//! Búfer de una línea de píxeles de un mismo color, para lcd_panel_repetir_color().
static uint16_t g_lcd_panel_bufer_color[LCD_PANEL_PIXELES_BUFER];

#if LCD_PANEL_CON_FREERTOS
//! Mutex que protege el bus. Sin memoria dinámica si FreeRTOS admite objetos estáticos.
static SemaphoreHandle_t g_lcd_panel_mutex = NULL;

  #if configSUPPORT_STATIC_ALLOCATION
//! Memoria para el mutex (xSemaphoreCreateMutexStatic).
static StaticSemaphore_t g_lcd_panel_mutex_memoria;
  #endif
#endif

// ===== LCD_PANEL - Funciones Privadas =====
/**
 * @brief   Pone la línea CS a 1 (set = true) o a 0. La llama el driver FLEXIO_MCULCD.
 * @ingroup LCD_Panel
 * @private
 */
static void lcd_panel_pin_cs(bool set) {
  GPIO_PinWrite(LCD_PANEL_GPIO_CS, LCD_PANEL_PIN_CS, set ? 1u : 0u);
}

/**
 * @brief   Pone la línea RS a 1 (dato) o a 0 (orden). La llama el driver FLEXIO_MCULCD.
 * @ingroup LCD_Panel
 * @private
 */
static void lcd_panel_pin_rs(bool set) {
  GPIO_PinWrite(LCD_PANEL_GPIO_RS, LCD_PANEL_PIN_RS, set ? 1u : 0u);
}

/**
 * @brief   Aviso de "escritura en la memoria de imagen terminada". Lo llama el adaptador DBI.
 * @ingroup LCD_Panel
 * @private
 *
 * @details Marca la escritura como terminada y, si la había pedido
 * lcd_panel_escribir_pixeles_async(), llama a la función de aviso del usuario. La borra ANTES de
 * llamarla, por si desde ella se lanza otra escritura asíncrona.
 *
 * @param[in]   estado     Resultado de la transferencia (no se usa: el bus no tiene forma de
 *                         fallar sin DMA).
 * @param[in]   ptr_datos  Dato de usuario del adaptador DBI (no se usa).
 */
static void lcd_panel_memoria_escrita(status_t estado, void *ptr_datos) {

  lcd_panel_fin_t fin = g_lcd_panel_fin_usuario;
  void           *arg = g_lcd_panel_arg_usuario;

  (void) estado;
  (void) ptr_datos;

  g_lcd_panel_escritura_terminada = TRUE;

  if (fin != NULL) {
    g_lcd_panel_fin_usuario = NULL;
    fin(arg);
  }
}

/**
 * @brief   Espera, sin límite, a que termine la escritura en curso.
 * @ingroup LCD_Panel
 * @private
 *
 * @details Con el adaptador DBI sin DMA la escritura ya ha terminado cuando se llega aquí, y el
 * bucle no da ni una vuelta. Se deja para que el módulo siga siendo correcto si algún día se le
 * dan canales de eDMA al adaptador.
 */
static void lcd_panel_esperar_escritura(void) {
  while (!g_lcd_panel_escritura_terminada) {
    ;
  }
}

/**
 * @brief   Multiplexa los pines del bus y de control y los deja con su configuración eléctrica.
 * @ingroup LCD_Panel
 * @private
 *
 * @details Es lo mismo que hace BOARD_InitPins() en el pin_mux.c de los ejemplos LVGL, pero sólo
 * para los pines de la pantalla: así el módulo funciona también en un proyecto cuyo pin_mux.c no
 * los configure. Pines rápidos (*fast slew rate*), sin resistencias, con el búfer de entrada
 * habilitado (el FlexIO también lee por ellos).
 */
static void lcd_panel_configurar_pines(void) {

  port_pin_config_t configuracion = {.pullSelect          = kPORT_PullDisable,
                                     .pullValueSelect     = kPORT_LowPullResistor,
                                     .slewRate            = kPORT_FastSlewRate,
                                     .passiveFilterEnable = kPORT_PassiveFilterDisable,
                                     .openDrainEnable     = kPORT_OpenDrainDisable,
                                     .driveStrength       = kPORT_LowDriveStrength,
#if defined(FSL_FEATURE_PORT_HAS_DRIVE_STRENGTH1) && FSL_FEATURE_PORT_HAS_DRIVE_STRENGTH1
                                     .driveStrength1 = kPORT_NormalDriveStrength,
#endif
                                     .mux          = kPORT_MuxAlt6,
                                     .inputBuffer  = kPORT_InputBufferEnable,
                                     .invertInput  = kPORT_InputNormal,
                                     .lockRegister = kPORT_UnlockRegister};
  uint32_t i;

  /* Habilita el reloj y saca del reset los PORT y GPIO que se van a usar. Repetirlo si ya lo
   * hizo BOARD_InitPins() no tiene ningún efecto.
   */
  CLOCK_EnableClock(kCLOCK_GatePORT0);
  CLOCK_EnableClock(kCLOCK_GatePORT2);
  CLOCK_EnableClock(kCLOCK_GatePORT3);
  CLOCK_EnableClock(kCLOCK_GatePORT4);
  CLOCK_EnableClock(kCLOCK_GateGPIO2);
  CLOCK_EnableClock(kCLOCK_GateGPIO3);
  RESET_ReleasePeripheralReset(kPORT0_RST_SHIFT_RSTn);
  RESET_ReleasePeripheralReset(kPORT2_RST_SHIFT_RSTn);
  RESET_ReleasePeripheralReset(kPORT3_RST_SHIFT_RSTn);
  RESET_ReleasePeripheralReset(kPORT4_RST_SHIFT_RSTn);
  RESET_ReleasePeripheralReset(kGPIO2_RST_SHIFT_RSTn);
  RESET_ReleasePeripheralReset(kGPIO3_RST_SHIFT_RSTn);

  /* Líneas de datos D0..D15, RD y WR: función FLEXIO0_Dn (alternativa 6). */
  for (i = 0; i < ARRAY_SIZE(kLCDPanelPinesFlexio); i++) {
    PORT_SetPinConfig(kLCDPanelPinesFlexio[i].puerto, kLCDPanelPinesFlexio[i].pin, &configuracion);
  }

  /* CS, RS, RST y BLK: función GPIO (alternativa 0). */
  configuracion.mux = kPORT_MuxAlt0;
  for (i = 0; i < ARRAY_SIZE(kLCDPanelPinesGPIO); i++) {
    PORT_SetPinConfig(kLCDPanelPinesGPIO[i].puerto, kLCDPanelPinesGPIO[i].pin, &configuracion);
  }
}

/**
 * @brief   Configura el pin INT del táctil GT911 como GPIO de entrada con o sin *pull-down*.
 * @ingroup LCD_Panel
 * @private
 *
 * @details El GT911 comparte con el ST7796S la línea de reset (P3_0). Al salir del reset, el GT911
 * mira el nivel de su pin INT para decidir su dirección I2C: a nivel bajo, 0x5D. Como el reset lo
 * da este módulo, es este módulo el que tiene que sujetar INT a nivel bajo mientras tanto; luego
 * lo deja como entrada sin resistencias. (Es lo que hace el ejemplo de la placa en
 * DEMO_InitLcdController() con DEMO_TouchConfigIntPin().)
 *
 * @param[in]   pull_down   TRUE: entrada con *pull-down*. FALSE: entrada sin resistencias.
 */
static void lcd_panel_configurar_int_tactil(bool_t pull_down) {

  const gpio_pin_config_t entrada       = {.pinDirection = kGPIO_DigitalInput, .outputLogic = 0u};
  const port_pin_config_t configuracion = {.pullSelect =
                                               pull_down ? kPORT_PullDown : kPORT_PullDisable,
                                           .pullValueSelect     = kPORT_LowPullResistor,
                                           .slewRate            = kPORT_FastSlewRate,
                                           .passiveFilterEnable = kPORT_PassiveFilterDisable,
                                           .openDrainEnable     = kPORT_OpenDrainDisable,
                                           .driveStrength       = kPORT_LowDriveStrength,
#if defined(FSL_FEATURE_PORT_HAS_DRIVE_STRENGTH1) && FSL_FEATURE_PORT_HAS_DRIVE_STRENGTH1
                                           .driveStrength1 = kPORT_NormalDriveStrength,
#endif
                                           .mux          = kPORT_MuxAlt0,
                                           .inputBuffer  = kPORT_InputBufferEnable,
                                           .invertInput  = kPORT_InputNormal,
                                           .lockRegister = kPORT_UnlockRegister};

  GPIO_PinInit(LCD_PANEL_GPIO_INT_TACTIL, LCD_PANEL_PIN_INT_TACTIL, &entrada);
  PORT_SetPinConfig(LCD_PANEL_PORT_INT_TACTIL, LCD_PANEL_PIN_INT_TACTIL, &configuracion);
}

// ===== LCD_PANEL - Funciones Publicas =====
void lcd_panel_inicializar(void) {

  const gpio_pin_config_t salida_a_1 = {.pinDirection = kGPIO_DigitalOutput, .outputLogic = 1u};
  const gpio_pin_config_t salida_a_0 = {.pinDirection = kGPIO_DigitalOutput, .outputLogic = 0u};

  /* La configuración del ST7796S es la del ejemplo de la placa: orientación apaisada girada
   * 270 grados, RGB565, sin señal TE, colores invertidos, imagen reflejada y filtro BGR. Las
   * tres últimas no son gustos: son lo que necesita el panel concreto de la LCD-PAR-S035 para
   * que los colores y la imagen salgan bien.
   */
  const st7796s_config_t configuracion_st7796s = {.driverPreset = kST7796S_DriverPresetLCDPARS035,
                                                  .pixelFormat  = kST7796S_PixelFormatRGB565,
                                                  .orientationMode = kST7796S_Orientation270,
                                                  .teConfig        = kST7796S_TEDisabled,
                                                  .invertDisplay   = true,
                                                  .flipDisplay     = true,
                                                  .bgrFilter       = true};
  flexio_mculcd_config_t configuracion_flexio;

  /* Inicialización idempotente: la segunda llamada y las siguientes no hacen nada. */
  if (g_lcd_panel_inicializado) {
    return;
  }
  g_lcd_panel_inicializado = TRUE;

#if LCD_PANEL_CON_FREERTOS
  /* La configuración de FreeRTOS que genera el SDK trae configSUPPORT_STATIC_ALLOCATION a 0: en
   * ese caso el mutex sale del montón de FreeRTOS. */
  #if configSUPPORT_STATIC_ALLOCATION
  g_lcd_panel_mutex = xSemaphoreCreateMutexStatic(&g_lcd_panel_mutex_memoria);
  #else
  g_lcd_panel_mutex = xSemaphoreCreateMutex();
  #endif
#endif

  /* 1. Pines. CS, RS y RST salen a 1 (inactivos); la retroiluminación, apagada hasta que la
   * memoria de imagen esté limpia, para no enseñar la basura que contiene al arrancar.
   */
  lcd_panel_configurar_pines();
  GPIO_PinInit(LCD_PANEL_GPIO_CS, LCD_PANEL_PIN_CS, &salida_a_1);
  GPIO_PinInit(LCD_PANEL_GPIO_RS, LCD_PANEL_PIN_RS, &salida_a_1);
  GPIO_PinInit(LCD_PANEL_GPIO_RST, LCD_PANEL_PIN_RST, &salida_a_1);
  GPIO_PinInit(LCD_PANEL_GPIO_BLK, LCD_PANEL_PIN_BLK, &salida_a_0);

  /* 2. Reloj del FlexIO: FRO_HF sin dividir, como en BOARD_InitHardware() del ejemplo. El módulo
   * es el dueño del FlexIO: le da reloj y lo saca del reset él mismo, sin depender de lo que haga
   * el hardware_init.c del proyecto.
   */
  CLOCK_SetClockDiv(kCLOCK_DivFLEXIO0, 1u);
  CLOCK_AttachClk(kFRO_HF_to_FLEXIO0);
  CLOCK_EnableClock(kCLOCK_GateFLEXIO0);
  RESET_ReleasePeripheralReset(kFLEXIO0_RST_SHIFT_RSTn);

  /* 3. El bus 8080 sobre el FlexIO. FLEXIO_MCULCD_GetDefaultConfig() deja el FlexIO habilitado
   * y activo también en depuración; sólo cambiamos la velocidad. FLEXIO_MCULCD_Init() calcula él
   * el divisor del temporizador a partir de la frecuencia de reloj del FlexIO.
   */
  FLEXIO_MCULCD_GetDefaultConfig(&configuracion_flexio);
  configuracion_flexio.baudRate_Bps = LCD_PANEL_FLEXIO_BPS;
  if (FLEXIO_MCULCD_Init(&g_lcd_panel_bus, &configuracion_flexio, CLOCK_GetFlexioClkFreq()) !=
      kStatus_Success) {
    return;
  }

  /* 4. El adaptador DBI, sin canales de eDMA (NULL, NULL): las escrituras en memoria de imagen
   * las hace la CPU y son bloqueantes.
   */
  if (DBI_FLEXIO_EDMA_CreateXferHandle(&g_lcd_panel_dbi, &g_lcd_panel_bus, NULL, NULL) !=
      kStatus_Success) {
    return;
  }

  /* 5. Reset del ST7796S (y del GT911, que comparte la línea): RST a 0 al menos 10 us —damos
   * 1 ms—, con el pin INT del táctil sujeto a nivel bajo para que el GT911 elija la dirección
   * I2C 0x5D; RST a 1, INT libre, y 5 ms de espera para que el ST7796S acepte órdenes.
   *
   * Las esperas son activas (SDK_DelayAtLeastUs), también con FreeRTOS: así la función puede
   * llamarse antes de arrancar el planificador.
   */
  GPIO_PinWrite(LCD_PANEL_GPIO_RST, LCD_PANEL_PIN_RST, 0u);
  lcd_panel_configurar_int_tactil(TRUE);
  SDK_DelayAtLeastUs(kLCDPanelEsperaUs, SystemCoreClock);
  GPIO_PinWrite(LCD_PANEL_GPIO_RST, LCD_PANEL_PIN_RST, 1u);
  lcd_panel_configurar_int_tactil(FALSE);
  SDK_DelayAtLeastUs(kLCDTactilEsperaUs, SystemCoreClock);

  /* 6. Secuencia de arranque del ST7796S, a través del adaptador DBI. */
  if (ST7796S_Init(&g_lcd_panel_st7796s, &configuracion_st7796s, &g_dbiFlexioEdmaXferOps,
                   &g_lcd_panel_dbi) != kStatus_Success) {
    return;
  }

  /* 7. Nuestra función de aviso de "escritura en memoria terminada". */
  ST7796S_SetMemoryDoneCallback(&g_lcd_panel_st7796s, lcd_panel_memoria_escrita, NULL);

  /* 8. Memoria de imagen a negro, pantalla encendida y retroiluminación encendida. */
  lcd_panel_ventana(0u, 0u, LCD_PANEL_ANCHO - 1u, LCD_PANEL_ALTO - 1u);
  lcd_panel_repetir_color(0x0000u, (uint32_t) LCD_PANEL_ANCHO * LCD_PANEL_ALTO);
  (void) ST7796S_EnableDisplay(&g_lcd_panel_st7796s, true);
  GPIO_PinWrite(LCD_PANEL_GPIO_BLK, LCD_PANEL_PIN_BLK, 1u);
}

void lcd_panel_ventana(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {

  if (!g_lcd_panel_inicializado) {
    return;
  }

  /* Órdenes CASET (columnas x0..x1) y RASET (filas y0..y1). Tras ellas, la orden RAMWR que
   * manda ST7796S_WritePixels() empieza a escribir en (x0, y0).
   */
  (void) ST7796S_SelectArea(&g_lcd_panel_st7796s, x0, y0, x1, y1);
}

void lcd_panel_escribir_pixeles(const uint16_t *ptr_pixeles, uint32_t n) {

  if (!g_lcd_panel_inicializado || n == 0u) {
    return;
  }

  g_lcd_panel_fin_usuario         = NULL;
  g_lcd_panel_escritura_terminada = FALSE;

  /* ST7796S_WritePixels() no modifica los píxeles (los pasa como const al adaptador DBI), pero
   * su prototipo no lleva const: de ahí la conversión.
   */
  (void) ST7796S_WritePixels(&g_lcd_panel_st7796s, (uint16_t *) ptr_pixeles, n);

  lcd_panel_esperar_escritura();
}

void lcd_panel_repetir_color(uint16_t color, uint32_t n) {

  uint32_t i;
  uint32_t trozo;
  bool_t   primer_trozo = TRUE;

  if (!g_lcd_panel_inicializado || n == 0u) {
    return;
  }

  /* Rellena el búfer de una línea con el color (como mucho, n píxeles). */
  trozo = (n < LCD_PANEL_PIXELES_BUFER) ? n : LCD_PANEL_PIXELES_BUFER;
  for (i = 0; i < trozo; i++) {
    g_lcd_panel_bufer_color[i] = color;
  }

  /* Y lo envía las veces que haga falta. El primer trozo va con la orden RAMWR, que coloca el
   * puntero de escritura del ST7796S al principio de la ventana; los siguientes, con RAMWRC
   * (Write Memory Continue, 0x3C), que sigue escribiendo donde se quedó. Si todos fueran con
   * RAMWR, cada trozo volvería a empezar por la esquina de la ventana.
   */
  while (n > 0u) {
    trozo = (n < LCD_PANEL_PIXELES_BUFER) ? n : LCD_PANEL_PIXELES_BUFER;

    g_lcd_panel_fin_usuario         = NULL;
    g_lcd_panel_escritura_terminada = FALSE;

    if (primer_trozo) {
      (void) ST7796S_WritePixels(&g_lcd_panel_st7796s, g_lcd_panel_bufer_color, trozo);
      primer_trozo = FALSE;
    } else {
      (void) g_lcd_panel_st7796s.xferOps->writeMemory(g_lcd_panel_st7796s.xferOpsData,
                                                      kMIPI_DBI_WriteMemoryContinue,
                                                      g_lcd_panel_bufer_color, trozo * 2u);
    }

    lcd_panel_esperar_escritura();
    n -= trozo;
  }
}

void lcd_panel_escribir_pixeles_async(const uint16_t *ptr_pixeles, uint32_t n,
                                      lcd_panel_fin_t ptr_fin, void *ptr_arg) {

  /* Sin pantalla o sin píxeles no hay nada que escribir, pero se avisa igualmente: quien espera
   * el aviso (LVGL) se quedaría bloqueado para siempre si no llegase.
   */
  if (!g_lcd_panel_inicializado || n == 0u) {
    if (ptr_fin != NULL) {
      ptr_fin(ptr_arg);
    }
    return;
  }

  g_lcd_panel_arg_usuario         = ptr_arg;
  g_lcd_panel_fin_usuario         = ptr_fin;
  g_lcd_panel_escritura_terminada = FALSE;

  (void) ST7796S_WritePixels(&g_lcd_panel_st7796s, (uint16_t *) ptr_pixeles, n);
}

void lcd_panel_tomar_bus(void) {
#if LCD_PANEL_CON_FREERTOS
  /* Antes de arrancar el planificador sólo hay un hilo de ejecución y no hay nada que proteger. */
  if (g_lcd_panel_mutex != NULL && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
    (void) xSemaphoreTake(g_lcd_panel_mutex, portMAX_DELAY);
  }
#endif
}

void lcd_panel_soltar_bus(void) {
#if LCD_PANEL_CON_FREERTOS
  if (g_lcd_panel_mutex != NULL && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
    (void) xSemaphoreGive(g_lcd_panel_mutex);
  }
#endif
}
