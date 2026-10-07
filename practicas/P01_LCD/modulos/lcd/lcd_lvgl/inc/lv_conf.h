/**
 * @file    lv_conf.h
 * @brief   Configuración de LVGL 9.4 para la FRDM-MCXA156 con la pantalla LCD-PAR-S035
 * (480 x 320, RGB565), común a lcd_lvgl_* y a los widgets de LVGL.
 *
 * @details Parte del lv_conf.h de los ejemplos LVGL del SDK 26.06 para esta placa
 * (examples/lvgl_examples/lvgl_sdk/lv_conf.h, que sólo fija la profundidad de color, más los
 * valores de examples/_boards/frdmmcxa156/lvgl_examples/prj.conf) y de la plantilla
 * middleware/lvgl/lv_conf_template.h. Sólo se escriben aquí las opciones que cambian respecto a
 * los valores por defecto de LVGL; el resto las completa lv_conf_internal.h.
 *
 * CÓMO LLEGA A LVGL: en el SDK 26.06, LVGL se configura por Kconfig (CONFIG_LV_CONF_SKIP=y) y no
 * lee ningún lv_conf.h. El Kconfig del módulo lcd_lvgl define LV_CONF_PATH para que lo incluya
 * igualmente: lo que se define aquí manda y el resto de opciones sale de Kconfig (CONFIG_LV_*).
 *
 * PRESUPUESTO DE MEMORIA (120 KiB de SRAM para todo el programa):
 *  - Búferes de dibujo: 2 x (480 x 20 píxeles x 2 bytes) = 38 400 bytes = 37,5 KiB. Son
 *    variables estáticas de lcd_lvgl_port.c, no salen de LV_MEM_SIZE.
 *  - Montón de LVGL (objetos, estilos, textos de etiquetas, capas): LV_MEM_SIZE = 40 KiB.
 *  - Variables globales de LVGL: del orden de 1 a 2 KiB (estimado, sin medir en el .map).
 *  - Pila de main(): 6 KiB (LCD_LVGL_PILA_PRINCIPAL; sin RTOS, LVGL dibuja en ella).
 *  Con FreeRTOS hace falta además un montón de FreeRTOS de 24 KiB: LVGL crea su tarea de dibujo
 *  con 8 KiB de pila (ver modulos/lcd/README.md).
 *
 * SISTEMA OPERATIVO: LV_USE_OS se elige con LCD_PANEL_CON_FREERTOS, la misma macro que usa
 * lcd_panel. Es una opción del Kconfig de lcd_panel que se activa sola si el proyecto usa
 * FreeRTOS y llega a todos los fuentes (también a los de LVGL) a través de mcux_config.h.
 *  - A 0 o sin definir: LV_OS_NONE. lv_lock()/lv_unlock() no hacen nada.
 *  - A 1: LV_OS_FREERTOS. lv_lock() es un mutex RECURSIVO de FreeRTOS:
 *    configUSE_RECURSIVE_MUTEXES tiene que valer 1 (es su valor por defecto en el SDK).
 *
 * @author      Alejandro Lara Doña - alejandro.lara@uca.es
 * @date        2026
 * @version     1.0
 *
 * @copyright   GNU General Public License version 3 or later
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/* clang-format off */

// ===== Color =====
#define LV_COLOR_DEPTH  16  //!< RGB565, el formato del ST7796S configurado por lcd_panel.

// ===== Memoria =====
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN   //!< Montón propio de LVGL (lv_mem).
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN
#define LV_MEM_SIZE             (40 * 1024U)        //!< Montón de LVGL en bytes (ver @file).

/** Tamaño del búfer de las capas simples (opacidad, sombras...). Sale de LV_MEM_SIZE y sólo se
 * reserva cuando un widget lo necesita; 24 KiB por defecto es demasiado para 120 KiB de SRAM. */
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE   (8 * 1024)

// ===== Refresco =====
#define LV_DEF_REFR_PERIOD  33    //!< Periodo de refresco en ms (unas 30 imágenes/s como máximo).
#define LV_DPI_DEF          130   //!< Densidad aproximada de la pantalla de 3,5" (píxeles/pulgada).

// ===== Sistema operativo =====
#if defined(LCD_PANEL_CON_FREERTOS) && (LCD_PANEL_CON_FREERTOS != 0)
#define LV_USE_OS                   LV_OS_FREERTOS
#define LV_USE_FREERTOS_TASK_NOTIFY 1
#else
#define LV_USE_OS                   LV_OS_NONE
#endif

// ===== Dibujo por software =====
#define LV_USE_DRAW_SW      1
#define LV_DRAW_SW_COMPLEX  1   //!< Necesario para radios, círculos, líneas gruesas y sombras.

// ===== Depuración =====
#define LV_USE_LOG                  0
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_SYSMON               0
#define LV_USE_FLOAT                0

// ===== Fuentes =====
/* lcd_lvgl_* traduce las fuentes de glcd a fuentes de LVGL:
 *   FUENTE8X16  -> lv_font_unscii_16     (monoespaciada, 8 x 16: casa con la de glcd)
 *   FUENTE12X24 -> lv_font_montserrat_24 (PROPORCIONAL: las columnas no casan con glcd)
 *   FUENTE16X32 -> lv_font_montserrat_32 (PROPORCIONAL)
 * Montserrat 14 es la fuente por defecto de los widgets. */
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_24   1
#define LV_FONT_MONTSERRAT_32   1
#define LV_FONT_UNSCII_16       1
#define LV_FONT_DEFAULT         &lv_font_montserrat_14

// ===== Widgets =====
/* Se dejan los valores por defecto de LVGL (todos los widgets básicos activados). lcd_lvgl_*
 * necesita al menos lv_label y lv_line; los ejemplos usan lv_slider y lv_button. */
#define LV_USE_LABEL    1
#define LV_USE_LINE     1
#define LV_USE_SLIDER   1
#define LV_USE_BUTTON   1

/* clang-format on */

#endif  // LV_CONF_H
