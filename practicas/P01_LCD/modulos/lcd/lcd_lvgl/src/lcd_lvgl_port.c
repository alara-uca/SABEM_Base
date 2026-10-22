/**
 * @file    lcd_lvgl_port.c
 * @brief   *Port* de LVGL 9.4 para la LCD-PAR-S035 (ST7796S por FlexIO 8080 y táctil GT911):
 * reloj, *display* con búferes parciales y dispositivo de entrada táctil.
 *
 * @details Escrito a partir del *port* de referencia del SDK 26.06
 * (examples/lvgl_examples/lvgl_sdk/lvgl_support/mcx_flexio/lvgl_support.c), con dos cambios:
 *  - No toca el ST7796S ni el FlexIO: vuelca con lcd_panel, la única dueña del bus, para que
 *    lcd_* y LVGL puedan convivir.
 *  - No espera a que termine cada volcado dentro de la función de volcado (el ejemplo del SDK sí
 *    lo hace): lo avisa la interrupción de fin, y así LVGL dibuja en un búfer mientras el otro
 *    viaja a la pantalla.
 *
 * CÁLCULO DE LOS BÚFERES: en modo parcial (LV_DISPLAY_RENDER_MODE_PARTIAL) LVGL no necesita
 * una imagen completa de la pantalla (480 x 320 x 2 bytes = 300 KiB, más del doble de la SRAM
 * del micro), sino un búfer donde dibujar un trozo cada vez. La documentación de LVGL recomienda
 * al menos 1/10 de la pantalla (32 líneas); el ejemplo del SDK usa un búfer de 32 líneas. Aquí
 * se usan DOS búferes de 20 líneas:
 *     2 búferes x 480 píxeles x 20 líneas x 2 bytes/píxel = 38 400 bytes = 37,5 KiB,
 * que junto con los 40 KiB del montón de LVGL (LV_MEM_SIZE en lv_conf.h) dejan unos 40 KiB de
 * los 120 KiB para el resto del programa. Con dos búferes el volcado de uno se solapa con el
 * dibujo del siguiente; con uno solo (33 líneas en el mismo espacio) LVGL tendría que esperar.
 *
 * DÓNDE SE TOMA Y SE SUELTA EL BUS DE LA PANTALLA:
 *  - Se TOMA en la función de volcado, la primera vez que se llama dentro de un refresco.
 *  - NO se suelta en la interrupción de fin de volcado: con FreeRTOS el bus es un mutex, y un
 *    mutex no se puede devolver desde una interrupción ni desde otra tarea que la que lo tomó
 *    (herencia de prioridad). La interrupción sólo avisa a LVGL (lv_display_flush_ready()).
 *  - Se SUELTA en lcd_lvgl_port_soltar_bus(), que lcd_lvgl_procesar() y lcd_lvgl_refrescar_ya()
 *    llaman al terminar el refresco, en la misma tarea y con lv_lock() todavía tomado.
 *  Con la configuración actual de lcd_panel (sin eDMA, como el ejemplo de la placa) el volcado
 *  "asíncrono" lo hace la CPU dentro de la propia llamada y el aviso de fin llega antes de que
 *  vuelva; este código funciona igual en ese caso y en el de una interrupción posterior, porque
 *  marca la transferencia en curso ANTES de lanzarla.
 *  Consecuencia: mientras LVGL refresca, una tarea que dibuje con lcd_* espera como mucho lo que
 *  dura un refresco (unos milisegundos por cada franja de 20 líneas modificada).
 *
 * @author    Alejandro Lara Doña - alejandro.lara(at)uca.es
 * @date      2026
 * @version   1.0
 *
 * @copyright GNU General Public License version 3 or later
 */

#include "lcd_lvgl_port.h"
#include "lcd_panel.h"
#include "lcd_tactil.h"
#include "tipos.h"
#include "lvgl.h"

#if LCD_PANEL_CON_FREERTOS
  #include "FreeRTOS.h"
  #include "task.h"

  #if !defined(configUSE_RECURSIVE_MUTEXES) || (configUSE_RECURSIVE_MUTEXES == 0)
    #error "lcd_lvgl_port: LVGL con FreeRTOS necesita configUSE_RECURSIVE_MUTEXES a 1 (lv_lock)."
  #endif
#endif  // LCD_PANEL_CON_FREERTOS

// ===== LCD_LVGL_PORT - Variables Privadas =====
/**
 * @brief   Búferes de dibujo de LVGL: LCD_LVGL_PORT_NUM_BUFERES franjas de
 * LCD_LVGL_PORT_LINEAS_BUFER líneas de la pantalla en RGB565 (2 bytes por píxel).
 * @ingroup LCD_LVGL_Port
 * @private
 */
SDK_ALIGN(static uint8_t buferes_dibujo[LCD_LVGL_PORT_NUM_BUFERES]
                                       [LCD_PANEL_ANCHO * LCD_LVGL_PORT_LINEAS_BUFER * 2u],
          4);

static bool_t          g_inicializado = FALSE;  //!< TRUE tras lcd_lvgl_port_inicializar().
static bool_t          g_bus_tomado   = FALSE;  //!< TRUE si el port tiene el bus de la LCD.
static volatile bool_t g_transferencia_en_curso = FALSE;  //!< TRUE mientras viaja un volcado.
static uint16_t        g_tactil_x               = 0;      //!< Última coordenada X leída del táctil.
static uint16_t        g_tactil_y               = 0;      //!< Última coordenada Y leída del táctil.

#if !LCD_PANEL_CON_FREERTOS
static volatile uint32_t g_milisegundos = 0;  //!< Reloj de LVGL sin FreeRTOS.
#endif

// ===== LCD_LVGL_PORT - Funciones Privadas =====
/**
 * @brief   Devuelve a LVGL los milisegundos transcurridos desde el arranque.
 * @ingroup LCD_LVGL_Port
 * @private
 *
 * @details LVGL la llama cada vez que necesita saber la hora (temporizadores, animaciones,
 * periodo de refresco, lectura del táctil). Se registra con lv_tick_set_cb().
 */
static uint32_t lcd_lvgl_port_leer_ms(void) {
#if LCD_PANEL_CON_FREERTOS
  return (uint32_t) pdTICKS_TO_MS(xTaskGetTickCount());
#else
  return g_milisegundos;
#endif
}

/**
 * @brief   Se ejecuta en la interrupción de fin de volcado de lcd_panel.
 * @ingroup LCD_LVGL_Port
 * @private
 *
 * @details Sólo avisa: marca que no hay transferencia en curso y le dice a LVGL que el búfer
 * vuelve a estar libre (lv_display_flush_ready() puede llamarse desde una interrupción). El bus
 * NO se suelta aquí (ver la cabecera del fichero).
 *
 * @param[in]   ptr_arg   El *display* de LVGL, pasado a lcd_panel_escribir_pixeles_async().
 */
static void lcd_lvgl_port_fin_volcado(void *ptr_arg) {
  g_transferencia_en_curso = FALSE;
  lv_display_flush_ready((lv_display_t *) ptr_arg);
}

/**
 * @brief   Función de volcado (*flush_cb*) de LVGL: envía a la pantalla el rectángulo que LVGL
 * acaba de dibujar en uno de los búferes.
 * @ingroup LCD_LVGL_Port
 * @private
 *
 * @details LVGL la llama desde lv_timer_handler() (o desde lv_refr_now()) una vez por cada
 * franja modificada. Antes de llamarla, LVGL ya ha esperado a que termine el volcado anterior,
 * así que el bus está libre de transferencias.
 *
 * @param[in]   ptr_display   *Display* de LVGL que se está refrescando.
 * @param[in]   ptr_area      Rectángulo de pantalla, con extremos incluidos.
 * @param[in]   ptr_px_map    Píxeles RGB565 del rectángulo, de izquierda a derecha y de arriba
 *                            abajo.
 */
static void lcd_lvgl_port_volcar(lv_display_t *ptr_display, const lv_area_t *ptr_area,
                                 uint8_t *ptr_px_map) {
  uint32_t n = (uint32_t) lv_area_get_width(ptr_area) * (uint32_t) lv_area_get_height(ptr_area);

  if (!g_bus_tomado) {
    lcd_panel_tomar_bus();
    g_bus_tomado = TRUE;
  }

#if LCD_LVGL_PORT_INTERCAMBIAR_BYTES
  lv_draw_sw_rgb565_swap(ptr_px_map, n);
#endif

  lcd_panel_ventana((uint16_t) ptr_area->x1, (uint16_t) ptr_area->y1, (uint16_t) ptr_area->x2,
                    (uint16_t) ptr_area->y2);
  g_transferencia_en_curso = TRUE;
  lcd_panel_escribir_pixeles_async((const uint16_t *) ptr_px_map, n, lcd_lvgl_port_fin_volcado,
                                   ptr_display);
}

/**
 * @brief   Función de lectura (*read_cb*) del dispositivo de entrada de LVGL.
 * @ingroup LCD_LVGL_Port
 * @private
 *
 * @details LVGL la llama periódicamente (cada LV_DEF_REFR_PERIOD ms) desde lv_timer_handler().
 * Se apoya en la misma librería del táctil que usa el programa, lcd_tactil_leer(). Si no hay
 * dedo, se devuelve el último punto conocido con estado "suelto", como pide LVGL.
 *
 * @param[in]   ptr_indev   Dispositivo de entrada (no se usa).
 * @param[out]  ptr_datos   Estado y coordenadas que se entregan a LVGL.
 */
static void lcd_lvgl_port_leer_tactil(lv_indev_t *ptr_indev, lv_indev_data_t *ptr_datos) {
  (void) ptr_indev;

  if (lcd_tactil_leer(&g_tactil_x, &g_tactil_y)) {
    ptr_datos->state = LV_INDEV_STATE_PRESSED;
  } else {
    ptr_datos->state = LV_INDEV_STATE_RELEASED;
  }
  ptr_datos->point.x = (int32_t) g_tactil_x;
  ptr_datos->point.y = (int32_t) g_tactil_y;
}

// ===== LCD_LVGL_PORT - Funciones Publicas =====
void lcd_lvgl_port_inicializar(void) {
  lv_display_t *display;
  lv_indev_t   *indev;

  if (g_inicializado) {
    return;
  }

  // Pantalla: pines, FlexIO y ST7796S (no hace nada si lcd_inicializar() ya la preparó).
  lcd_panel_inicializar();

  // LVGL y su reloj.
  lv_init();
  lv_tick_set_cb(lcd_lvgl_port_leer_ms);
#if !LCD_PANEL_CON_FREERTOS && LCD_LVGL_PORT_SYSTICK_PROPIO
  (void) SysTick_Config(SystemCoreClock / 1000u);  // Una interrupción por milisegundo.
#endif

  // Display: 480 x 320, dos búferes parciales y la función de volcado.
  display = lv_display_create((int32_t) LCD_PANEL_ANCHO, (int32_t) LCD_PANEL_ALTO);
  lv_display_set_buffers(display, buferes_dibujo[0],
                         (LCD_LVGL_PORT_NUM_BUFERES > 1u) ? buferes_dibujo[1] : NULL,
                         (uint32_t) sizeof(buferes_dibujo[0]), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(display, lcd_lvgl_port_volcar);

  // Táctil como dispositivo de entrada de tipo puntero (si el GT911 responde).
  if (lcd_tactil_inicializar()) {
    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lcd_lvgl_port_leer_tactil);
  }

  g_inicializado = TRUE;
}

void lcd_lvgl_port_soltar_bus(void) {
  while (g_transferencia_en_curso) {
    // Espera activa corta: como mucho, lo que tarda en viajar una franja de 20 líneas.
  }
  if (g_bus_tomado) {
    g_bus_tomado = FALSE;
    lcd_panel_soltar_bus();
  }
}

void lcd_lvgl_port_contar_1ms(void) {
#if !LCD_PANEL_CON_FREERTOS
  g_milisegundos++;
#endif
}

#if !LCD_PANEL_CON_FREERTOS && LCD_LVGL_PORT_SYSTICK_PROPIO
/**
 * @brief   Interrupción del SysTick (sin FreeRTOS): cuenta un milisegundo para LVGL.
 * @ingroup LCD_LVGL_Port
 * @private
 *
 * @note    Con FreeRTOS el SysTick es del núcleo (xPortSysTickHandler) y esta función no existe.
 */
void SysTick_Handler(void) {
  g_milisegundos++;
}
#endif
