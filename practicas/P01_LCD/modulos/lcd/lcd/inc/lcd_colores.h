/**
 * @file    lcd_colores.h
 * @brief   Colores RGB565 con nombre para la pantalla LCD-PAR-S035: los mismos que tenía la
 * biblioteca glcd_ del LPC4088, en español y en inglés.
 *
 * @details Están en una cabecera aparte, y no dentro de lcd.h, para que cualquier otra biblioteca
 * de pantalla de la asignatura que trabaje con colores RGB565 de 16 bits pueda incluir ésta en
 * lugar de volver a definirlos.
 *
 * Un color RGB565 ocupa 16 bits: 5 de rojo (bits 15..11), 6 de verde (bits 10..5) y 5 de azul
 * (bits 4..0). La macro LCD_RGB() lo construye a partir de las tres componentes de 8 bits, que es
 * como se dan los colores en casi cualquier programa de dibujo, descartando los bits de menos peso.
 *
 * @author      Alejandro Lara Doña - alejandro.lara@uca.es | Eduardo Romero
 * @date        2014/2026
 * @version     3.0
 *
 * @copyright   GNU General Public License version 3 or later
 */

#ifndef LCD_COLORES_H
#define LCD_COLORES_H

#include "tipos.h"

// ===== LCD - Macros Publicas =====
/**
 * @brief   Convierte un color RGB de 24 bits (8:8:8) al formato de 16 bits (5:6:5) de la pantalla.
 * @ingroup LCD
 *
 * @param[in]  r  Intensidad de rojo, 0-255.
 * @param[in]  g  Intensidad de verde, 0-255.
 * @param[in]  b  Intensidad de azul, 0-255.
 *
 * @return  Color en formato RGB565.
 *
 * @warning r, g y b deben ser de 8 bits. Los argumentos se evalúan una sola vez cada uno.
 */
#define LCD_RGB(r, g, b)  ((uint16_t)((((r) & 0xF8u) << 8) | (((g) & 0xFCu) << 3) | \
                                      (((b) & 0xF8u) >> 3)))

/**
 * @brief   Nombre que tenía LCD_RGB() en la biblioteca glcd_, conservado por compatibilidad.
 * @ingroup LCD
 */
#ifndef RGB
#define RGB(r, g, b)      LCD_RGB(r, g, b)
#endif

// ===== LCD - Constantes Publicas =====
/**
 * @defgroup  Lcd_Colores LCD - Colores con nombre
 * @ingroup   LCD
 * @brief     Colores más comunes, en español y en inglés. Son los mismos nombres y valores que en
 * glcd.h.
 *
 * @details Son macros protegidas con `#ifndef`, y no un enumerado, para que convivan sin
 * conflicto con las mismas definiciones de lcd_lvgl.h: la primera cabecera que se incluya las
 * define, y la segunda ve que ya existen y no las toca (los valores son idénticos).
 * @{
 */
#ifndef NEGRO
#define NEGRO         LCD_RGB(0, 0, 0)
#endif
#ifndef AZUL_OSCURO
#define AZUL_OSCURO   LCD_RGB(0, 0, 128)
#endif
#ifndef VERDE_OSCURO
#define VERDE_OSCURO  LCD_RGB(0, 128, 0)
#endif
#ifndef CIAN_OSCURO
#define CIAN_OSCURO   LCD_RGB(0, 128, 128)
#endif
#ifndef MARRON
#define MARRON        LCD_RGB(128, 0, 0)
#endif
#ifndef PURPURA
#define PURPURA       LCD_RGB(128, 0, 128)
#endif
#ifndef OLIVA
#define OLIVA         LCD_RGB(128, 255, 0)
#endif
#ifndef NARANJA
#define NARANJA       LCD_RGB(255, 128, 0)
#endif
#ifndef GRIS_CLARO
#define GRIS_CLARO    LCD_RGB(192, 192, 192)
#endif
#ifndef GRIS_OSCURO
#define GRIS_OSCURO   LCD_RGB(128, 128, 128)
#endif
#ifndef AZUL
#define AZUL          LCD_RGB(0, 0, 255)
#endif
#ifndef VERDE
#define VERDE         LCD_RGB(0, 255, 0)
#endif
#ifndef CIAN
#define CIAN          LCD_RGB(0, 255, 255)
#endif
#ifndef ROJO
#define ROJO          LCD_RGB(255, 0, 0)
#endif
#ifndef MAGENTA
#define MAGENTA       LCD_RGB(255, 0, 255)
#endif
#ifndef AMARILLO
#define AMARILLO      LCD_RGB(255, 255, 0)
#endif
#ifndef BLANCO
#define BLANCO        LCD_RGB(255, 255, 255)
#endif
#ifndef BLACK
#define BLACK         LCD_RGB(0, 0, 0)
#endif
#ifndef DARKBLUE
#define DARKBLUE      LCD_RGB(0, 0, 128)
#endif
#ifndef DARKGREEN
#define DARKGREEN     LCD_RGB(0, 128, 0)
#endif
#ifndef DARKCYAN
#define DARKCYAN      LCD_RGB(0, 128, 128)
#endif
#ifndef MAROON
#define MAROON        LCD_RGB(128, 0, 0)
#endif
#ifndef PURPLE
#define PURPLE        LCD_RGB(128, 0, 128)
#endif
#ifndef OLIVE
#define OLIVE         LCD_RGB(128, 255, 0)
#endif
#ifndef ORANGE
#define ORANGE        LCD_RGB(255, 128, 0)
#endif
#ifndef LIGHTGREY
#define LIGHTGREY     LCD_RGB(192, 192, 192)
#endif
#ifndef DARKGREY
#define DARKGREY      LCD_RGB(128, 128, 128)
#endif
#ifndef BLUE
#define BLUE          LCD_RGB(0, 0, 255)
#endif
#ifndef GREEN
#define GREEN         LCD_RGB(0, 255, 0)
#endif
#ifndef CYAN
#define CYAN          LCD_RGB(0, 255, 255)
#endif
#ifndef RED
#define RED           LCD_RGB(255, 0, 0)
#endif
#ifndef YELLOW
#define YELLOW        LCD_RGB(255, 255, 0)
#endif
#ifndef WHITE
#define WHITE         LCD_RGB(255, 255, 255)
#endif
//!@}

#endif  // LCD_COLORES_H
