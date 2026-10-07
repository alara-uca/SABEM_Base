/**
 * @file    lcd_lvgl_port.h
 * @brief   *Port* de LVGL 9.4 para la pantalla LCD-PAR-S035 de la FRDM-MCXA156: une LVGL con
 * lcd_panel (salida de imagen) y con lcd_tactil (entrada táctil).
 *
 * @details Un *port* es el pegamento entre una biblioteca gráfica portable y el hardware
 * concreto. LVGL no sabe nada del ST7796S ni del GT911: necesita que alguien le dé
 *  1. Un reloj en milisegundos (lv_tick_set_cb()), para animaciones y temporizadores.
 *  2. Un *display* (lv_display_create()) con búferes de dibujo en RAM y una función de
 *     volcado (*flush*) que copie un rectángulo ya dibujado a la pantalla.
 *  3. Un dispositivo de entrada (lv_indev_create()) con una función que lea el táctil.
 * Este módulo hace exactamente eso, y nada más. Lo usa lcd_lvgl.c; el programa normalmente no
 * llama a este módulo directamente, sino a lcd_lvgl_inicializar() y a lcd_lvgl_procesar().
 *
 * RELOJ DE LVGL:
 *  - Con FreeRTOS (LCD_PANEL_CON_FREERTOS a 1): el contador de ticks del núcleo,
 *    xTaskGetTickCount(), pasado a milisegundos.
 *  - Sin FreeRTOS: un contador de milisegundos que incrementa la interrupción SysTick. Si
 *    LCD_LVGL_PORT_SYSTICK_PROPIO vale 1 (por defecto), este módulo programa el SysTick a 1 ms y
 *    define SysTick_Handler(). Si el programa ya usa el SysTick para otra cosa, se pone a 0 y el
 *    programa debe llamar a lcd_lvgl_port_contar_1ms() cada milisegundo desde su propia
 *    interrupción. (El ejemplo lvgl_demo_widgets_bm del SDK hace lo mismo con lv_tick_inc().)
 *
 * @author      Alejandro Lara Doña - alejandro.lara@uca.es
 * @date        2026
 * @version     1.0
 *
 * @copyright   GNU General Public License version 3 or later
 */

#ifndef LCD_LVGL_PORT_H
#define LCD_LVGL_PORT_H

#include "tipos.h"
#include "lcd_panel.h"

// ===== LCD_LVGL_PORT - Macros Publicas =====
/**
 * @defgroup  Lcd_Lvgl_Port_Macros LCD_LVGL_PORT - Macros Públicas
 * @ingroup   Lcd_Lvgl_Port
 * @{
 */
#ifndef LCD_LVGL_PORT_SYSTICK_PROPIO
#define LCD_LVGL_PORT_SYSTICK_PROPIO  1   //!< Sin FreeRTOS: 1 = el port usa el SysTick.
#endif

/** Líneas de pantalla que caben en cada búfer de dibujo. Ver el cálculo en lcd_lvgl_port.c. */
#define LCD_LVGL_PORT_LINEAS_BUFER  20u

/** Número de búferes de dibujo: con 2, LVGL dibuja en uno mientras el otro se envía. */
#define LCD_LVGL_PORT_NUM_BUFERES   2u

/** 1 si hay que intercambiar los dos bytes de cada píxel RGB565 antes de enviarlo. Con el
 * FlexIO del MCX A156 y el ST7796S NO hace falta (el ejemplo del SDK envía el búfer tal cual). */
#ifndef LCD_LVGL_PORT_INTERCAMBIAR_BYTES
#define LCD_LVGL_PORT_INTERCAMBIAR_BYTES  0
#endif
//!@}

// ===== LCD_LVGL_PORT - Funciones Publicas =====
/**
 * @brief   Inicializa la pantalla (lcd_panel), el táctil (lcd_tactil) y LVGL, y registra en
 * LVGL el reloj, el *display* y el dispositivo de entrada.
 * @ingroup Lcd_Lvgl_Port
 *
 * @details Llamarla más de una vez no tiene efecto. Con FreeRTOS debe llamarse desde una
 * tarea, con el planificador ya en marcha.
 */
void lcd_lvgl_port_inicializar(void);

/**
 * @brief   Espera a que termine el último volcado de píxeles y suelta el bus de la pantalla si
 * el *port* lo tenía tomado.
 * @ingroup Lcd_Lvgl_Port
 *
 * @details El bus se toma en el primer volcado de un refresco de LVGL y se conserva durante el
 * refresco completo; ésta es la función que lo devuelve. La llaman lcd_lvgl_procesar() y
 * lcd_lvgl_refrescar_ya() al terminar, en contexto de tarea y con lv_lock() tomado (un mutex de
 * FreeRTOS sólo puede soltarlo la tarea que lo tomó, y nunca desde una interrupción).
 */
void lcd_lvgl_port_soltar_bus(void);

/**
 * @brief   Avanza un milisegundo el reloj de LVGL. Sólo sin FreeRTOS y con
 * LCD_LVGL_PORT_SYSTICK_PROPIO a 0: hay que llamarla cada milisegundo desde una interrupción.
 * @ingroup Lcd_Lvgl_Port
 */
void lcd_lvgl_port_contar_1ms(void);

#endif  // LCD_LVGL_PORT_H
