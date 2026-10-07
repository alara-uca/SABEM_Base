/**
 * @file    lcd_tactil.h
 * @brief   Librería independiente del panel táctil de la LCD-PAR-S035 (controlador GT911, bus
 * LPI2C2), en modo inmediato.
 *
 * @details Un solo driver del táctil para todo: lo usa directamente el programa
 * (lcd_tactil_leer()) y lo usa también el dispositivo de entrada de LVGL (lcd_lvgl_port.c). Las
 * coordenadas se devuelven ya giradas a la orientación apaisada de lcd_panel (0..479, 0..319).
 *
 * @note    El LPI2C2 es un bus COMPARTIDO (táctil, RTC DS3231 y sensor de distancia en SABM). Este
 * módulo lo inicializa sólo si nadie lo ha hecho antes, a la frecuencia común
 * LCD_TACTIL_I2C_FRECUENCIA_HZ, y con FreeRTOS la exclusión mutua del bus es responsabilidad
 * de quien lo comparte (P10 de SABM).
 *
 * @note    CONTRATO DE INTERFAZ fijado el 21/09/2026: lcd_lvgl_port.c se escribe contra estas
 * firmas.
 *
 * @author      Alejandro Lara Doña - alejandro.lara@uca.es
 * @date        2026
 * @version     1.0
 *
 * @copyright   GNU General Public License version 3 or later
 */

#ifndef LCD_TACTIL_H
#define LCD_TACTIL_H

#include "tipos.h"

// ===== LCD_TACTIL - Macros Publicas =====
#define LCD_TACTIL_I2C_FRECUENCIA_HZ  100000u  //!< Frecuencia SCL del bus compartido.

// ===== LCD_TACTIL - Funciones Publicas =====
/**
 * @brief   Inicializa el bus (si hace falta) y el GT911.
 *
 * @details Pasos:
 *  1. Llama a lcd_panel_inicializar() (que no hace nada si ya se había llamado): el GT911
 *     comparte la línea de reset con el ST7796S, y es ese reset —con el pin INT del GT911 sujeto a
 *     nivel bajo— el que fija su dirección I2C en 0x5D.
 *  2. Pone en marcha el LPI2C2 (reloj FRO 12 MHz, pines P1_8/P1_9, LPI2C_MasterInit() a
 *     LCD_TACTIL_I2C_FRECUENCIA_HZ) **sólo si no lo está ya**. Lo averigua mirando que el reloj
 *     del LPI2C2 esté habilitado (MRCC_GLB_CC1), que esté fuera de reset (MRCC_GLB_RST1) y que su
 *     maestro esté habilitado (bit MEN de MCR). Si otro módulo lo inicializó antes, se respeta su
 *     configuración, incluida la frecuencia.
 *  3. Inicializa el GT911 con el driver fsl_gt911 del SDK.
 *
 * Llamarla más de una vez no tiene efecto si la primera tuvo éxito. Hace esperas activas de
 * unos 80 ms en total (también con FreeRTOS).
 *
 * @return  TRUE si el GT911 ha respondido; FALSE si no.
 */
bool_t lcd_tactil_inicializar(void);

/**
 * @brief   Lee el primer punto de contacto.
 *
 * @details Pregunta al GT911 por I2C (no usa la interrupción de su pin INT). El GT911 renueva
 * sus datos unas cien veces por segundo; si se le pregunta antes, no tiene nada nuevo que dar, y
 * la función devuelve lo último que se supo. Por eso puede llamarse tan a menudo como se quiera.
 * Si lcd_tactil_inicializar() no ha tenido éxito, devuelve siempre FALSE.
 *
 * @param[out]  x   Coordenada horizontal (0..LCD_PANEL_ANCHO-1). No se toca si no hay toque.
 * @param[out]  y   Coordenada vertical (0..LCD_PANEL_ALTO-1). No se toca si no hay toque.
 * @return  TRUE si hay un dedo sobre la pantalla; FALSE si no.
 */
bool_t lcd_tactil_leer(uint16_t *x, uint16_t *y);

/**
 * @brief   Indica si la pantalla está pulsada, sin devolver coordenadas.
 *
 * @details Equivale a lcd_tactil_leer(NULL, NULL): x e y pueden ser NULL en lcd_tactil_leer().
 */
bool_t lcd_tactil_pulsado(void);

#endif  // LCD_TACTIL_H
