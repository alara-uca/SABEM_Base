/**
 * @file    lcd.h
 * @brief   Biblioteca de dibujo en modo inmediato para la pantalla LCD-PAR-S035 de la
 * FRDM-MCXA156: texto con tres fuentes de mapa de bits, puntos, líneas, rectángulos y círculos.
 *
 * @details Es la heredera directa de la biblioteca glcd_ del LPC4088: **las mismas funciones, con
 * los mismos argumentos y el mismo comportamiento, con el prefijo lcd_ en lugar de glcd_**, los
 * mismos nombres de colores (NEGRO, BLANCO, ROJO...) y de fuentes (FUENTE8X16, FUENTE12X24,
 * FUENTE16X32), y los mismos algoritmos de dibujo y las mismas fuentes.
 *
 * Lo que cambia es lo que no se ve. glcd_ dibujaba en un *framebuffer* de la SDRAM externa que el
 * controlador de LCD del LPC4088 volcaba continuamente a la pantalla. Aquí no hay SDRAM ni
 * controlador de LCD, pero no hacen falta: el controlador de la pantalla, el ST7796S, tiene su
 * propia memoria de imagen. Cada función de dibujo elige una **ventana** rectangular en esa
 * memoria y le envía una **ráfaga** de píxeles, a través de lcd_panel. Lo que se dibuja se queda
 * en la pantalla hasta que se dibuje otra cosa encima.
 *
 * "Modo inmediato" quiere decir que cada llamada dibuja en el acto y se olvida: la biblioteca no
 * recuerda qué hay en la pantalla. Para escribir un número que cambia hay que escribirlo siempre
 * en el mismo sitio, con color de fondo, para que tape al anterior.
 *
 * Coordenadas: el origen (0, 0) es la esquina superior izquierda, con la pantalla apaisada en
 * la misma orientación que los ejemplos LVGL del SDK; x crece hacia la derecha, hasta
 * kLCDXMaximo (479), e y hacia abajo, hasta kLCDYMaximo (319).
 *
 * Texto: las fuentes contienen los caracteres del 32 al 255 de la codificación **ISO-8859-1
 * (Latin-1)**, que incluye las letras acentuadas, la ñ, ¡, ¿ y º. Como los ficheros fuente se
 * escriben en UTF-8, lcd_printf(), lcd_xprintf() y lcd_texto() traducen las secuencias UTF-8 de
 * esos caracteres, así que `lcd_printf("Año")` sale bien. lcd_caracter(), en cambio, recibe el
 * código Latin-1 tal cual: la ñ es '\\xF1'.
 *
 * Cada función toma el bus de la pantalla (lcd_panel_tomar_bus()) mientras dibuja, así que con
 * FreeRTOS varias tareas pueden dibujar sin mezclar sus píxeles. El cursor, los colores y la
 * fuente de lcd_printf() son uno solo para todo el programa: con varias tareas, cada una debería
 * usar lcd_xprintf(), que no los usa ni los cambia.
 *
 * @note    Se puede combinar con LVGL (lcd_lvgl_*) sólo en zonas distintas de la pantalla: LVGL
 * repinta lo que considera suyo y borraría lo dibujado aquí.
 *
 * @author      Alejandro Lara Doña - alejandro.lara(at)uca.es | Eduardo Romero
 * @date        2014/2026
 * @version   3.0
 *
 * @copyright GNU General Public License version 3 or later
 */

/**
 * @defgroup  LCD LCD - Dibujo en modo inmediato
 * @brief     Funciones lcd_* de dibujo directo sobre la pantalla LCD-PAR-S035.
 */

#ifndef LCD_H
#define LCD_H

#include "lcd_panel.h"
#include "tipos.h"
#include "lcd_colores.h"

// ===== LCD - Macros Publicas =====
/**
 * @defgroup  LCD_Fuentes LCD - Fuentes disponibles
 * @ingroup   LCD
 * @brief     Números de las fuentes de mapa de bits, los mismos que en glcd.h.
 *
 * @details Macros protegidas con `#ifndef` para convivir con las de lcd_lvgl.h, que usa los
 * mismos nombres y valores.
 * @{
 */
#ifndef FUENTE8X16
  #define FUENTE8X16 0u  //!< Fuente pequeña (8 píxeles de ancho y 16 de alto).
#endif
#ifndef FUENTE12X24
  #define FUENTE12X24 1u  //!< Fuente mediana (12 píxeles de ancho y 24 de alto).
#endif
#ifndef FUENTE16X32
  #define FUENTE16X32 2u  //!< Fuente grande (16 píxeles de ancho y 32 de alto).
#endif
#ifndef NUMERO_FUENTES
  #define NUMERO_FUENTES 3u  //!< Número de fuentes disponibles (nombre de glcd.h).
#endif
#define LCD_NUMERO_FUENTES 3u  //!< Número de fuentes disponibles.
//!@}

/**
 * @brief   Longitud máxima, en bytes, del texto que puede generar una llamada a lcd_printf() o
 * lcd_xprintf(), incluido el terminador. Lo que sobre se pierde.
 * @ingroup LCD
 */
#define LCD_MAX_TEXTO_PRINTF 128u

// ===== LCD - Constantes Publicas =====
/**
 * @brief   Límites de la pantalla.
 * @ingroup LCD
 */
enum lcd_tamano {
  kLCDTamanoX = LCD_PANEL_ANCHO,    //!< Número de píxeles en horizontal.
  kLCDTamanoY = LCD_PANEL_ALTO,     //!< Número de píxeles en vertical.
  kLCDXMinimo = 0,                  //!< Píxel inicial en horizontal.
  kLCDXMaximo = (kLCDTamanoX - 1),  //!< Píxel máximo en horizontal.
  kLCDYMinimo = 0,                  //!< Píxel inicial en vertical.
  kLCDYMaximo = (kLCDTamanoY - 1)   //!< Píxel máximo en vertical.
};

// ===== LCD - Funciones Publicas =====
/**
 * @brief   Inicializa la pantalla (a través de lcd_panel_inicializar()) y la deja en negro.
 * @ingroup LCD
 *
 * @details Llamarla más de una vez no tiene efecto. La configuración del texto de lcd_printf()
 * de partida es: blanco sobre negro, fuente FUENTE16X32, cursor en (0, 0) y desplazamiento
 * automático activado.
 */
void lcd_inicializar(void);

/**
 * @brief   Borra la pantalla rellenándola con el color indicado.
 * @ingroup LCD
 *
 * @param[in]   color   Color con el que rellenar la pantalla.
 */
void lcd_borrar(uint16_t color);

/**
 * @brief   Función análoga a printf para imprimir texto en la pantalla, en la posición, colores y
 * fuente actuales, que después avanza. Reconoce los caracteres de control '\\b', '\\f', '\\n',
 * '\\r', '\\t' y '\\v'.
 * @ingroup LCD
 *
 * @details Si un carácter no cabe en lo que queda de línea, pasa a la siguiente. Si el texto
 * llega más abajo de la última línea, vuelve arriba; si además el desplazamiento automático está
 * activado, borra antes la pantalla (ver lcd_desplazar()).
 *
 * @param[in]   ptr_format  Cadena de formato, como la de printf.
 * @param[in]   ...         Argumentos adicionales, como en printf.
 *
 * @return  El número de caracteres que ha generado el formato (como vsnprintf), o un número
 * negativo si hay un error de formato. Si el texto pasa de LCD_MAX_TEXTO_PRINTF - 1 bytes, sólo
 * se imprimen los primeros.
 */
int32_t lcd_printf(const char *ptr_format, ...);

/**
 * @brief   Función parecida a lcd_printf pero indicando las coordenadas donde aparece el texto,
 * su color, el color de fondo y la fuente.
 * @ingroup LCD
 *
 * @param[in]   x             Coordenada X de pantalla en la que se imprimirá.
 * @param[in]   y             Coordenada Y de pantalla en la que se imprimirá.
 * @param[in]   color         Color con el que se imprimirá el texto.
 * @param[in]   color_fondo   Color de fondo con el que se imprimirá el texto.
 * @param[in]   fuente        Fuente de caracteres con la que se imprimirá.
 * @param[in]   ptr_format    Cadena de formato, como la de printf.
 * @param[in]   ...           Argumentos adicionales, como en printf.
 *
 * @return  Igual que lcd_printf().
 *
 * @note  No se modifica la configuración actual de colores, fuente o posición de lcd_printf().
 */
int32_t lcd_xprintf(uint16_t x, uint16_t y, uint16_t color, uint16_t color_fondo, uint32_t fuente,
                    const char *ptr_format, ...);

/**
 * @brief   Imprime un carácter en pantalla. No se interpretan caracteres de control.
 * @ingroup LCD
 *
 * @param[in]   c             Carácter a imprimir, en código Latin-1. Sólo se imprimirá si está
 *                            en la fuente indicada y las coordenadas (x, y) están dentro de la
 *                            pantalla; si el carácter se sale por la derecha o por abajo, se
 *                            dibuja la parte que cabe.
 * @param[in]   x             Coordenada X de la esquina superior izquierda del carácter.
 * @param[in]   y             Coordenada Y de la esquina superior izquierda del carácter.
 * @param[in]   color         Color con el que se imprimirá el carácter.
 * @param[in]   color_fondo   Color de fondo con el que se imprimirá el carácter.
 * @param[in]   fuente        Fuente de caracteres con la que se imprimirá. Si no es válida, no se
 *                            imprime nada.
 */
void lcd_caracter(char c, uint16_t x, uint16_t y, uint16_t color, uint16_t color_fondo,
                  uint32_t fuente);

/**
 * @brief   Imprime una cadena de caracteres en la pantalla. No se interpretan los caracteres de
 * control. Si un carácter no cabe en la línea, pasa a la siguiente; si no cabe en la pantalla,
 * vuelve arriba.
 * @ingroup LCD
 *
 * @param[in]   x             Coordenada X de pantalla en la que se imprimirá.
 * @param[in]   y             Coordenada Y de pantalla en la que se imprimirá.
 * @param[in]   color         Color con el que se imprimirá el texto.
 * @param[in]   color_fondo   Color de fondo con el que se imprimirá el texto.
 * @param[in]   fuente        Fuente de caracteres con la que se imprimirá.
 * @param[in]   ptr_str       Puntero a la cadena a imprimir (UTF-8 o ASCII). Sólo se imprimirá si
 *                            las coordenadas (x, y) están dentro de la pantalla.
 */
void lcd_texto(uint16_t x, uint16_t y, uint16_t color, uint16_t color_fondo, uint32_t fuente,
               const char *ptr_str);

/**
 * @brief   Fija el color que se usará para la salida de texto con lcd_printf.
 * @ingroup LCD
 *
 * @param[in]   color   Color de texto que usará lcd_printf.
 */
void lcd_color_texto(uint16_t color);

/**
 * @brief   Fija el color de fondo que se usará para la salida de texto con lcd_printf.
 * @ingroup LCD
 *
 * @param[in]   color_fondo   Color de fondo que usará lcd_printf.
 */
void lcd_fondo_texto(uint16_t color_fondo);

/**
 * @brief   Fija las coordenadas en las que se mostrará el texto en la siguiente llamada a
 * lcd_printf. Sólo tiene efecto si las coordenadas están dentro de la pantalla.
 * @ingroup LCD
 *
 * @param[in]   x   Coordenada X de la pantalla.
 * @param[in]   y   Coordenada Y de la pantalla.
 */
void lcd_xy_texto(uint16_t x, uint16_t y);

/**
 * @brief   Desplaza hacia arriba el contenido de la pantalla. **En esta pantalla, lo que hace es
 * borrarla entera con el color de fondo de texto actual.**
 * @ingroup LCD
 *
 * @details glcd_desplazar() movía el contenido del *framebuffer* con memmove. Aquí no hay
 * *framebuffer*: la imagen está en la memoria del ST7796S, y no hay forma barata de moverla:
 *  - El ST7796S tiene desplazamiento vertical por hardware (órdenes VSCRDEF, 0x33, y VSCRSADD,
 *    0x37), pero desplaza a lo largo de las 480 filas físicas del panel, que con la orientación
 *    apaisada que usamos son las COLUMNAS de la imagen: movería el contenido de lado, no hacia
 *    arriba. [PENDIENTE VERIFICAR en placa]
 *  - Leer la memoria de imagen para volver a escribirla más arriba (orden RAMRD) es posible, pero
 *    muy lento, y el ST7796S devuelve los píxeles en otro formato (18 bits).
 *
 * Así que se hace lo que haría un terminal sin memoria: borrar y volver a empezar por arriba.
 * El texto de lcd_printf() vuelve a la primera línea después de llamarla.
 *
 * @param[in]   lineas  Líneas a desplazar (no se usa: siempre se borra toda la pantalla). Se
 *                      conserva por compatibilidad con glcd_desplazar().
 */
void lcd_desplazar(uint16_t lineas);

/**
 * @brief   Activa o desactiva el desplazamiento automático (ver lcd_desplazar()) cuando la
 * salida de texto mediante lcd_printf sobrepasa la línea inferior. Desactivado, el texto
 * vuelve arriba y se escribe encima de lo que haya.
 * @ingroup LCD
 *
 * @param[in]   activo   TRUE => activar, FALSE => desactivar.
 */
void lcd_activar_desplazamiento(bool_t activo);

/**
 * @brief   Selecciona la fuente de caracteres que se usará para la salida de texto mediante la
 * función lcd_printf. Si el número no es válido, no hace nada.
 * @ingroup LCD
 *
 * @param[in]   fuente  Número de la fuente a seleccionar (FUENTE8X16, FUENTE12X24, FUENTE16X32).
 */
void lcd_seleccionar_fuente(uint32_t fuente);

/**
 * @brief   Dibuja un punto (píxel) en la pantalla. Si está fuera, no hace nada.
 * @ingroup LCD
 *
 * @param[in]   x       Coordenada X del punto.
 * @param[in]   y       Coordenada Y del punto.
 * @param[in]   color   Color del punto.
 *
 * @note    Cada punto es una ventana de 1 x 1 más un píxel: una docena de escrituras en el bus.
 * Para rellenar zonas, lcd_rectangulo_relleno() es muchísimo más rápido.
 */
void lcd_punto(uint16_t x, uint16_t y, uint16_t color);

/**
 * @brief   Dibuja un segmento de línea recta definido por las coordenadas de sus extremos. Se
 * dibuja la porción contenida en la pantalla (si la hay).
 * @ingroup LCD
 *
 * @param[in]   x0      Coordenada X del primer extremo del segmento.
 * @param[in]   y0      Coordenada Y del primer extremo del segmento.
 * @param[in]   x1      Coordenada X del segundo extremo del segmento.
 * @param[in]   y1      Coordenada Y del segundo extremo del segmento.
 * @param[in]   color   Color del segmento.
 */
void lcd_linea(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

/**
 * @brief   Dibuja un rectángulo (sólo el borde) definido por las coordenadas de los vértices de
 * una diagonal. Se dibuja la porción contenida en la pantalla.
 * @ingroup LCD
 *
 * @param[in]   x0      Coordenada X de un vértice.
 * @param[in]   y0      Coordenada Y de un vértice.
 * @param[in]   x1      Coordenada X del vértice opuesto al dado por (x0, y0).
 * @param[in]   y1      Coordenada Y del vértice opuesto al dado por (x0, y0).
 * @param[in]   color   Color del rectángulo.
 */
void lcd_rectangulo(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

/**
 * @brief   Dibuja un rectángulo relleno definido por las coordenadas de los vértices de una
 * diagonal. Se dibuja la porción contenida en la pantalla.
 * @ingroup LCD
 *
 * @param[in]   x0      Coordenada X de un vértice.
 * @param[in]   y0      Coordenada Y de un vértice.
 * @param[in]   x1      Coordenada X del vértice opuesto al dado por (x0, y0).
 * @param[in]   y1      Coordenada Y del vértice opuesto al dado por (x0, y0).
 * @param[in]   color   Color del rectángulo.
 */
void lcd_rectangulo_relleno(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

/**
 * @brief   Dibuja una circunferencia. Se dibuja la parte contenida en la pantalla.
 * @ingroup LCD
 *
 * @param[in]   xc      Coordenada X del centro.
 * @param[in]   yc      Coordenada Y del centro.
 * @param[in]   radio   Radio de la circunferencia (en píxeles).
 * @param[in]   color   Color de la circunferencia.
 */
void lcd_circunferencia(uint16_t xc, uint16_t yc, uint16_t radio, uint16_t color);

/**
 * @brief   Dibuja un círculo relleno. Se dibuja la parte contenida en la pantalla.
 * @ingroup LCD
 *
 * @param[in]   xc      Coordenada X del centro.
 * @param[in]   yc      Coordenada Y del centro.
 * @param[in]   radio   Radio del círculo (en píxeles).
 * @param[in]   color   Color del círculo.
 */
void lcd_circulo(uint16_t xc, uint16_t yc, uint16_t radio, uint16_t color);

#endif  // LCD_H
