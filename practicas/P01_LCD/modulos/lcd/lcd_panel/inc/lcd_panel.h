/**
 * @file    lcd_panel.h
 * @brief   Capa más baja de la pantalla LCD-PAR-S035: única dueña del controlador ST7796S y del
 * bus paralelo 8080 que fabrica el FlexIO.
 *
 * @details Las tres librerías de dibujo (lcd_*, lcd_lvgl_* y el *port* de LVGL) escriben en la
 * pantalla EXCLUSIVAMENTE a través de este módulo. Por eso se pueden mezclar: nadie más toca el
 * ST7796S ni el FlexIO. El módulo ofrece:
 *  - Inicialización del bus y del controlador (orientación apaisada, 480 x 320, RGB565).
 *  - Selección de una ventana rectangular y volcado de píxeles en ella, en modo bloqueante (lo
 *    que usa lcd_*) y en modo asíncrono con aviso por *callback* (lo que usa LVGL).
 *  - Un cerrojo del bus (lcd_panel_tomar_bus() / lcd_panel_soltar_bus()): un mutex de FreeRTOS
 *    si el proyecto lo usa (macro LCD_PANEL_CON_FREERTOS a 1), y operaciones vacías si no.
 *
 * El bus es un 8080 de **16 líneas de datos** (FXIO_D0..D15), más WR y RD, fabricado con el
 * FLEXIO0 por el driver FLEXIO_MCULCD del SDK; CS, RS y RST son GPIO. El módulo configura él mismo
 * todos esos pines (no depende del pin_mux.c del proyecto), y también sujeta el pin INT del táctil
 * durante el reset, porque el GT911 comparte la línea RST con el ST7796S (ver lcd_tactil.h).
 *
 * @author    Alejandro Lara Doña - alejandro.lara(at)uca.es
 * @date      2026
 * @version   1.0
 *
 * @copyright GNU General Public License version 3 or later
 */

/**
 * @defgroup  LCD_Panel LCD_PANEL - Pantalla, capa baja
 * @brief     Controlador ST7796S y bus 8080 fabricado con el FlexIO.
 */

#ifndef LCD_PANEL_H
#define LCD_PANEL_H

#include "tipos.h"

// ===== LCD_PANEL - Macros Publicas =====
/**
 * @defgroup  LCD_Panel_Macros LCD_PANEL - Macros Públicas
 * @ingroup   LCD_Panel
 * @{
 */
#define LCD_PANEL_ANCHO 480u  //!< Píxeles en horizontal, en orientación apaisada.
#define LCD_PANEL_ALTO  320u  //!< Píxeles en vertical, en orientación apaisada.

#ifndef LCD_PANEL_CON_FREERTOS
  #define LCD_PANEL_CON_FREERTOS 0  //!< 1: el cerrojo del bus es un mutex de FreeRTOS.
#endif
//!@}

// ===== LCD_PANEL - Tipos Publicos =====
/**
 * @brief   Función a la que llama lcd_panel_escribir_pixeles_async() al terminar el volcado.
 * @ingroup LCD_Panel
 *
 * @details Se ejecuta en contexto de interrupción: debe ser corta.
 *
 * @param[in]   ptr_arg   El argumento que se pasó a lcd_panel_escribir_pixeles_async().
 */
typedef void (*lcd_panel_fin_t)(void *ptr_arg);

// ===== LCD_PANEL - Funciones Publicas =====
/**
 * @brief   Inicializa pines, FlexIO y ST7796S, y deja la pantalla encendida y en negro.
 *
 * @details Llamarla más de una vez no tiene efecto: la segunda y siguientes vuelven sin hacer
 * nada, para que lcd_inicializar() y lcd_lvgl_inicializar() puedan llamarla las dos.
 */
void lcd_panel_inicializar(void);

/**
 * @brief   Selecciona la ventana rectangular [x0..x1] x [y0..y1] (extremos incluidos) en la que
 * se escribirán los siguientes píxeles, de izquierda a derecha y de arriba abajo.
 */
void lcd_panel_ventana(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/**
 * @brief   Escribe n píxeles RGB565 en la ventana seleccionada y espera a que terminen.
 */
void lcd_panel_escribir_pixeles(const uint16_t *ptr_pixeles, uint32_t n);

/**
 * @brief   Escribe n veces el mismo color en la ventana seleccionada (rellenos y borrado sin
 * *framebuffer*). Bloqueante.
 */
void lcd_panel_repetir_color(uint16_t color, uint32_t n);

/**
 * @brief   Empieza a escribir n píxeles en la ventana seleccionada y vuelve sin esperar. Al
 * terminar llama a ptr_fin(ptr_arg) desde la interrupción. El búfer debe seguir vivo hasta
 * entonces.
 *
 * @note    Con la configuración actual —adaptador DBI sin eDMA, igual que el ejemplo de la placa
 * en el SDK 26.06— la escritura la hace la CPU dentro de esta misma llamada, y ptr_fin(ptr_arg)
 * se ejecuta ANTES de que la función vuelva, en el contexto de quien llama (no en una
 * interrupción). El código cliente debe funcionar en los dos casos: no debe suponer que ptr_fin()
 * llega más tarde, ni que llega desde una interrupción. Si n es 0 o la pantalla no está
 * inicializada, se llama a ptr_fin(ptr_arg) igualmente, para que nadie se quede esperando un aviso
 * que no llegaría.
 */
void lcd_panel_escribir_pixeles_async(const uint16_t *ptr_pixeles, uint32_t n,
                                      lcd_panel_fin_t ptr_fin, void *ptr_arg);

/**
 * @brief   Toma el bus de la pantalla (mutex con FreeRTOS; nada sin él). Cada secuencia
 * ventana + escritura de un cliente debe ir entre tomar y soltar.
 *
 * @warning El mutex NO es recursivo (configUSE_RECURSIVE_MUTEXES vale 0 en el FreeRTOSConfig.h
 * de la asignatura): una tarea que ya tiene el bus no debe volver a tomarlo. No se puede llamar
 * desde una interrupción. Antes de arrancar el planificador no hace nada.
 */
void lcd_panel_tomar_bus(void);

/**
 * @brief   Suelta el bus de la pantalla tomado con lcd_panel_tomar_bus().
 */
void lcd_panel_soltar_bus(void);

#endif  // LCD_PANEL_H
