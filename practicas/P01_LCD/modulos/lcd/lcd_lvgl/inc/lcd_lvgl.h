/**
 * @file    lcd_lvgl.h
 * @brief   Dibujo en modo inmediato EMULADO sobre LVGL para la pantalla LCD-PAR-S035 (480 x 320):
 * las mismas funciones que la antigua glcd_*, con el prefijo lcd_lvgl_.
 *
 * @details MODO INMEDIATO frente a MODO RETENIDO.
 *  - En MODO INMEDIATO (la antigua glcd_*, o lcd_*) cada llamada escribe píxeles en la pantalla
 *    en el acto y se olvida de ellos: la pantalla no "sabe" que allí hay un rectángulo.
 *  - En MODO RETENIDO (LVGL) el programa no pinta píxeles: crea OBJETOS (etiquetas, rectángulos,
 *    líneas, botones...) que LVGL guarda en un árbol en RAM. Más tarde, cuando se llama a
 *    lv_timer_handler(), LVGL averigua qué zonas han cambiado y las repinta él solo.
 *  Esta librería traduce lo primero a lo segundo: CADA LLAMADA CREA (O ACTUALIZA) UN OBJETO
 *  LVGL. Así se puede seguir programando "a la glcd" y, además, mezclar libremente con los
 *  widgets de LVGL (botones, deslizadores...), porque todo son objetos del mismo árbol.
 *
 * QUIÉN POSEE LOS OBJETOS: todos los que crea esta librería son hijos de un contenedor
 * transparente del tamaño de la pantalla, el LIENZO, que a su vez es hijo de la pantalla activa
 * de LVGL (lv_screen_active()) y se mantiene al fondo, detrás de los widgets. Si se borra el
 * lienzo o la pantalla, LVGL libera todos sus hijos.
 *
 * CUÁNDO SE PINTA: las funciones sólo modifican objetos; los píxeles llegan a la pantalla al
 * llamar a lcd_lvgl_procesar() (en el bucle principal, o periódicamente desde una tarea de
 * interfaz con FreeRTOS), o en el acto con lcd_lvgl_refrescar_ya().
 *
 * TAREAS: cada función toma lv_lock() al empezar y lo suelta al terminar, así que con FreeRTOS
 * puede llamarse desde cualquier tarea (nunca desde una interrupción).
 *
 * DIFERENCIAS CON glcd_* que hay que conocer:
 *  - Memoria: cada objeto ocupa RAM del montón de LVGL (decenas o cientos de bytes). Un
 *    programa que dibuja sin parar (p. ej. un rectángulo nuevo en cada vuelta del bucle) agota la
 *    memoria: hay que llamar a lcd_lvgl_borrar() de vez en cuando, o reutilizar objetos. El texto
 *    se reutiliza solo: escribir en la misma posición (x, y) actualiza la etiqueta que ya había.
 *  - lcd_lvgl_punto() crea un objeto por píxel: es MUY caro. Para dibujo masivo se usa lcd_*.
 *  - Fuentes: FUENTE8X16 es UNSCII 16 (monoespaciada de 8 x 16, igual que la de glcd).
 *    FUENTE12X24 y FUENTE16X32 son Montserrat 24 y 32, PROPORCIONALES: la "i" es más estrecha
 *    que la "M", así que las columnas no casan con las de glcd.
 *  - Una etiqueta reescrita con un texto más corto ya no deja restos del texto anterior.
 *
 * REGLA DE MEZCLA: lcd_lvgl_* y los widgets de LVGL se mezclan libremente. lcd_* (directo) y
 * LVGL sólo en zonas distintas de la pantalla, porque LVGL repinta lo que considera suyo.
 *
 * @author      Alejandro Lara Doña - alejandro.lara@uca.es
 * @date        2026
 * @version     1.0
 *
 * @copyright   GNU General Public License version 3 or later
 */

#ifndef LCD_LVGL_H
#define LCD_LVGL_H

#include "tipos.h"
#include "lcd_panel.h"

// ===== LCD_LVGL - Macros Publicas =====
/**
 * @defgroup  Lcd_Lvgl_Macros LCD_LVGL - Macros Públicas
 * @ingroup   Lcd_Lvgl
 * @brief     Colores RGB565 y fuentes con los mismos nombres que en la antigua glcd.h.
 *
 * @details Van protegidos con #ifndef para que no choquen con los de lcd.h si un programa
 * incluye las dos cabeceras (los valores son los mismos).
 * @{
 */
#ifndef RGB
/**
 * @brief   Convierte un color RGB de 24 bits (8:8:8) a RGB565 (5:6:5).
 * @warning r, g y b deben ser de 8 bits; se evalúan una sola vez cada uno.
 */
#define RGB(r, g, b)    ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))
#endif

#ifndef NEGRO
#define NEGRO         RGB(0, 0, 0)
#endif
#ifndef AZUL_OSCURO
#define AZUL_OSCURO   RGB(0, 0, 128)
#endif
#ifndef VERDE_OSCURO
#define VERDE_OSCURO  RGB(0, 128, 0)
#endif
#ifndef CIAN_OSCURO
#define CIAN_OSCURO   RGB(0, 128, 128)
#endif
#ifndef MARRON
#define MARRON        RGB(128, 0, 0)
#endif
#ifndef PURPURA
#define PURPURA       RGB(128, 0, 128)
#endif
#ifndef OLIVA
#define OLIVA         RGB(128, 255, 0)
#endif
#ifndef NARANJA
#define NARANJA       RGB(255, 128, 0)
#endif
#ifndef GRIS_CLARO
#define GRIS_CLARO    RGB(192, 192, 192)
#endif
#ifndef GRIS_OSCURO
#define GRIS_OSCURO   RGB(128, 128, 128)
#endif
#ifndef AZUL
#define AZUL          RGB(0, 0, 255)
#endif
#ifndef VERDE
#define VERDE         RGB(0, 255, 0)
#endif
#ifndef CIAN
#define CIAN          RGB(0, 255, 255)
#endif
#ifndef ROJO
#define ROJO          RGB(255, 0, 0)
#endif
#ifndef MAGENTA
#define MAGENTA       RGB(255, 0, 255)
#endif
#ifndef AMARILLO
#define AMARILLO      RGB(255, 255, 0)
#endif
#ifndef BLANCO
#define BLANCO        RGB(255, 255, 255)
#endif
#ifndef BLACK
#define BLACK         RGB(0, 0, 0)
#endif
#ifndef DARKBLUE
#define DARKBLUE      RGB(0, 0, 128)
#endif
#ifndef DARKGREEN
#define DARKGREEN     RGB(0, 128, 0)
#endif
#ifndef DARKCYAN
#define DARKCYAN      RGB(0, 128, 128)
#endif
#ifndef MAROON
#define MAROON        RGB(128, 0, 0)
#endif
#ifndef PURPLE
#define PURPLE        RGB(128, 0, 128)
#endif
#ifndef OLIVE
#define OLIVE         RGB(128, 255, 0)
#endif
#ifndef ORANGE
#define ORANGE        RGB(255, 128, 0)
#endif
#ifndef LIGHTGREY
#define LIGHTGREY     RGB(192, 192, 192)
#endif
#ifndef DARKGREY
#define DARKGREY      RGB(128, 128, 128)
#endif
#ifndef BLUE
#define BLUE          RGB(0, 0, 255)
#endif
#ifndef GREEN
#define GREEN         RGB(0, 255, 0)
#endif
#ifndef CYAN
#define CYAN          RGB(0, 255, 255)
#endif
#ifndef RED
#define RED           RGB(255, 0, 0)
#endif
#ifndef YELLOW
#define YELLOW        RGB(255, 255, 0)
#endif
#ifndef WHITE
#define WHITE         RGB(255, 255, 255)
#endif

#ifndef FUENTE8X16
#define FUENTE8X16    0u  //!< UNSCII 16: monoespaciada, 8 x 16 píxeles.
#endif
#ifndef FUENTE12X24
#define FUENTE12X24   1u  //!< Montserrat 24: proporcional, 24 píxeles de alto de línea aprox.
#endif
#ifndef FUENTE16X32
#define FUENTE16X32   2u  //!< Montserrat 32: proporcional, 32 píxeles de alto de línea aprox.
#endif
#ifndef NUMERO_FUENTES
#define NUMERO_FUENTES  3u  //!< Número de fuentes disponibles.
#endif

#define LCD_LVGL_MAX_ETIQUETAS  32u   //!< Etiquetas que se recuerdan para reutilizarlas por (x, y).
#define LCD_LVGL_MAX_TEXTO      128u  //!< Caracteres máximos de una llamada a printf/xprintf.
//!@}

// ===== LCD_LVGL - Funciones Publicas =====
/**
 * @brief   Inicializa la pantalla, el táctil y LVGL (a través de lcd_lvgl_port) y deja la
 * pantalla activa en negro, con el lienzo de dibujo creado.
 * @ingroup Lcd_Lvgl
 *
 * @details Llamarla más de una vez no tiene efecto. Con FreeRTOS, desde una tarea y con el
 * planificador en marcha.
 */
void lcd_lvgl_inicializar(void);

/**
 * @brief   Deja que LVGL haga su trabajo: atiende temporizadores y animaciones, lee el táctil,
 * envía eventos a los widgets y repinta las zonas que han cambiado.
 * @ingroup Lcd_Lvgl
 *
 * @details Hay que llamarla a menudo (cada 5 a 30 ms): en el bucle principal sin RTOS, o en
 * una tarea de interfaz con FreeRTOS. Sin ella NADA de lo dibujado llega a la pantalla.
 *
 * @return  Milisegundos que se puede esperar hasta la siguiente llamada (lo que devuelve
 * lv_timer_handler()).
 */
uint32_t lcd_lvgl_procesar(void);

/**
 * @brief   Repinta en el acto las zonas que han cambiado, sin esperar a lcd_lvgl_procesar().
 * @ingroup Lcd_Lvgl
 *
 * @details Útil para ver algo inmediatamente (p. ej. un mensaje antes de una operación
 * larga). No atiende al táctil ni a los temporizadores: no sustituye a lcd_lvgl_procesar().
 */
void lcd_lvgl_refrescar_ya(void);

/**
 * @brief   Borra la pantalla: elimina TODOS los objetos de la pantalla activa (también los
 * widgets que haya creado el programa), olvida las etiquetas y pone el fondo del color indicado.
 * @ingroup Lcd_Lvgl
 * @param[in]   color   Color RGB565 del fondo.
 */
void lcd_lvgl_borrar(uint16_t color);

/**
 * @brief   Análoga a printf: escribe texto con la posición, colores y fuente actuales.
 * @ingroup Lcd_Lvgl
 *
 * @details Interpreta '\\n', '\\r', '\\t', '\\b', '\\v' y '\\f' como glcd_printf. Si el texto
 * sobrepasa el borde derecho pasa a la línea siguiente, y si sobrepasa el inferior desplaza el
 * contenido hacia arriba (si el desplazamiento está activado) o vuelve arriba.
 *
 * @param[in]   formato   Cadena de formato, como en printf.
 * @param[in]   ...       Argumentos que corresponden a los especificadores de formato.
 * @return  Número de caracteres que ha generado el formato; negativo si hay error.
 * @note    Se admiten como mucho LCD_LVGL_MAX_TEXTO caracteres por llamada.
 */
int32_t lcd_lvgl_printf(const char *formato, ...);

/**
 * @brief   Como lcd_lvgl_printf(), pero indicando posición, colores y fuente. No cambia la
 * configuración de texto actual.
 * @ingroup Lcd_Lvgl
 * @param[in]   x             Coordenada X del primer carácter.
 * @param[in]   y             Coordenada Y del primer carácter.
 * @param[in]   color         Color del texto.
 * @param[in]   color_fondo   Color de fondo del texto.
 * @param[in]   fuente        FUENTE8X16, FUENTE12X24 o FUENTE16X32.
 * @param[in]   formato       Cadena de formato, como en printf.
 * @param[in]   ...           Argumentos que corresponden a los especificadores de formato.
 * @return  Número de caracteres que ha generado el formato; negativo si hay error.
 */
int32_t lcd_lvgl_xprintf(uint16_t x, uint16_t y, uint16_t color, uint16_t color_fondo,
                         uint32_t fuente, const char *formato, ...);

/**
 * @brief   Escribe un carácter en (x, y). No interpreta caracteres de control.
 * @ingroup Lcd_Lvgl
 * @param[in]   c             Carácter (sólo se escribe si es imprimible).
 * @param[in]   x             Coordenada X.
 * @param[in]   y             Coordenada Y.
 * @param[in]   color         Color del carácter.
 * @param[in]   color_fondo   Color de fondo.
 * @param[in]   fuente        FUENTE8X16, FUENTE12X24 o FUENTE16X32.
 */
void lcd_lvgl_caracter(char c, uint16_t x, uint16_t y, uint16_t color, uint16_t color_fondo,
                       uint32_t fuente);

/**
 * @brief   Escribe una cadena en (x, y), en una sola etiqueta. No interpreta caracteres de
 * control. A diferencia de glcd_texto(), lo que no cabe a la derecha se recorta (no salta de
 * línea).
 * @ingroup Lcd_Lvgl
 * @param[in]   x             Coordenada X.
 * @param[in]   y             Coordenada Y.
 * @param[in]   color         Color del texto.
 * @param[in]   color_fondo   Color de fondo.
 * @param[in]   fuente        FUENTE8X16, FUENTE12X24 o FUENTE16X32.
 * @param[in]   cadena        Texto terminado en '\\0'.
 */
void lcd_lvgl_texto(uint16_t x, uint16_t y, uint16_t color, uint16_t color_fondo, uint32_t fuente,
                    const char *cadena);

/**
 * @brief   Fija el color del texto de lcd_lvgl_printf().
 * @ingroup Lcd_Lvgl
 * @param[in]   color   Color RGB565.
 */
void lcd_lvgl_color_texto(uint16_t color);

/**
 * @brief   Fija el color de fondo del texto de lcd_lvgl_printf().
 * @ingroup Lcd_Lvgl
 * @param[in]   color_fondo   Color RGB565.
 */
void lcd_lvgl_fondo_texto(uint16_t color_fondo);

/**
 * @brief   Fija la posición del siguiente lcd_lvgl_printf(). Sin efecto si está fuera de la
 * pantalla.
 * @ingroup Lcd_Lvgl
 * @param[in]   x   Coordenada X.
 * @param[in]   y   Coordenada Y.
 */
void lcd_lvgl_xy_texto(uint16_t x, uint16_t y);

/**
 * @brief   Desplaza hacia arriba lo dibujado por lcd_lvgl_* el número de líneas (píxeles)
 * indicado. Los objetos que quedan por completo fuera de la pantalla se eliminan.
 * @ingroup Lcd_Lvgl
 * @param[in]   lineas  Píxeles que se desplaza.
 * @note    Sólo mueve los objetos del lienzo: los widgets de LVGL se quedan donde están.
 */
void lcd_lvgl_desplazar(uint16_t lineas);

/**
 * @brief   Activa o desactiva el desplazamiento automático de lcd_lvgl_printf() al llegar
 * abajo.
 * @ingroup Lcd_Lvgl
 * @param[in]   activo   TRUE: activar; FALSE: desactivar (se vuelve a la parte de arriba).
 */
void lcd_lvgl_activar_desplazamiento(bool_t activo);

/**
 * @brief   Selecciona la fuente de lcd_lvgl_printf().
 * @ingroup Lcd_Lvgl
 * @param[in]   fuente  FUENTE8X16, FUENTE12X24 o FUENTE16X32. Si no es válida, no se hace nada.
 */
void lcd_lvgl_seleccionar_fuente(uint32_t fuente);

/**
 * @brief   Dibuja un punto.
 * @ingroup Lcd_Lvgl
 * @param[in]   x       Coordenada X.
 * @param[in]   y       Coordenada Y.
 * @param[in]   color   Color.
 * @warning Crea UN OBJETO LVGL POR PÍXEL (del orden de 100 bytes cada uno): sirve para marcar
 * unos pocos puntos, no para dibujar curvas o imágenes. Para eso está lcd_*.
 */
void lcd_lvgl_punto(uint16_t x, uint16_t y, uint16_t color);

/**
 * @brief   Dibuja un segmento de recta entre (x0, y0) y (x1, y1), de 1 píxel de grosor.
 * @ingroup Lcd_Lvgl
 * @param[in]   x0      Coordenada X del primer extremo.
 * @param[in]   y0      Coordenada Y del primer extremo.
 * @param[in]   x1      Coordenada X del segundo extremo.
 * @param[in]   y1      Coordenada Y del segundo extremo.
 * @param[in]   color   Color.
 */
void lcd_lvgl_linea(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

/**
 * @brief   Dibuja el contorno de un rectángulo definido por dos vértices opuestos (incluidos).
 * @ingroup Lcd_Lvgl
 * @param[in]   x0      Coordenada X de un vértice.
 * @param[in]   y0      Coordenada Y de un vértice.
 * @param[in]   x1      Coordenada X del vértice opuesto.
 * @param[in]   y1      Coordenada Y del vértice opuesto.
 * @param[in]   color   Color del contorno.
 */
void lcd_lvgl_rectangulo(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

/**
 * @brief   Dibuja un rectángulo relleno definido por dos vértices opuestos (incluidos).
 * @ingroup Lcd_Lvgl
 * @param[in]   x0      Coordenada X de un vértice.
 * @param[in]   y0      Coordenada Y de un vértice.
 * @param[in]   x1      Coordenada X del vértice opuesto.
 * @param[in]   y1      Coordenada Y del vértice opuesto.
 * @param[in]   color   Color del relleno.
 */
void lcd_lvgl_rectangulo_relleno(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                                 uint16_t color);

/**
 * @brief   Dibuja una circunferencia (sólo el contorno).
 * @ingroup Lcd_Lvgl
 * @param[in]   xc      Coordenada X del centro.
 * @param[in]   yc      Coordenada Y del centro.
 * @param[in]   radio   Radio en píxeles.
 * @param[in]   color   Color.
 */
void lcd_lvgl_circunferencia(uint16_t xc, uint16_t yc, uint16_t radio, uint16_t color);

/**
 * @brief   Dibuja un círculo relleno.
 * @ingroup Lcd_Lvgl
 * @param[in]   xc      Coordenada X del centro.
 * @param[in]   yc      Coordenada Y del centro.
 * @param[in]   radio   Radio en píxeles.
 * @param[in]   color   Color.
 */
void lcd_lvgl_circulo(uint16_t xc, uint16_t yc, uint16_t radio, uint16_t color);

#endif  // LCD_LVGL_H
