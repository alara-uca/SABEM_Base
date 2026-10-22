/**
 * @file    lcd.c
 * @brief   Biblioteca de dibujo en modo inmediato para la pantalla LCD-PAR-S035 (heredera de
 * glcd_).
 *
 * @details Los algoritmos de dibujo (Bresenham para las líneas, el de punto medio para las
 * circunferencias, el recorrido bit a bit de las fuentes) son los de glcd.c. Sólo cambia el final
 * de cada uno: donde glcd.c escribía en el *framebuffer*, aquí se elige una ventana en la memoria
 * de imagen del ST7796S y se le envían los píxeles (lcd_panel_ventana() y
 * lcd_panel_escribir_pixeles() o lcd_panel_repetir_color()).
 *
 * Organización interna: cada función pública toma el bus, llama a una función privada
 * (lcd_dibujar_*) que hace el trabajo, y lo suelta. Las funciones privadas nunca toman el bus, y
 * por eso pueden llamarse unas a otras (el mutex de lcd_panel no es recursivo). Además trabajan
 * con coordenadas int32_t, para que una circunferencia que se sale por la izquierda o por arriba
 * (coordenadas negativas) se recorte bien en lugar de dar la vuelta.
 *
 * @author      Alejandro Lara Doña - alejandro.lara(at)uca.es | Eduardo Romero
 * @date        2014/2026
 * @version   3.0
 *
 * @copyright GNU General Public License version 3 or later
 */

#include "lcd.h"
#include <stdarg.h>
#include <stdio.h>
#include "lcd_panel.h"
#include "tipos.h"
#include "fuente.h"

// ===== LCD - Macros Privadas =====
/**
 * @brief   Píxeles del carácter más grande (16 x 32): tamaño del búfer de un carácter.
 * @ingroup LCD
 * @private
 */
#define LCD_PIXELES_CARACTER_MAX (16u * 32u)

// ===== LCD - Tipos Privados =====
/**
 * @brief   Configuración del texto (color, fuente y posición) para lcd_printf() y lcd_xprintf().
 * @ingroup LCD
 * @private
 */
typedef struct {
  uint16_t color;                    //!< Color del texto.
  uint16_t color_fondo;              //!< Color del fondo del texto.
  uint32_t fuente;                   //!< Fuente (FUENTE8X16, FUENTE12X24 o FUENTE16X32).
  uint16_t pos_x;                    //!< Coordenada X del siguiente carácter.
  uint16_t pos_y;                    //!< Coordenada Y del siguiente carácter.
  bool_t   desplazamiento_activado;  //!< Borrar la pantalla al llegar abajo (ver lcd_desplazar).
} lcd_config_texto_t;

// ===== LCD - Constantes Privadas =====
/**
 * @brief   Tabla para obtener un puntero a la fuente a partir de un número de fuente.
 * @ingroup LCD
 * @private
 */
static const fuente_t *const kLCDTablaFuentes[LCD_NUMERO_FUENTES] = {&kFuente8x16, &kFuente12x24,
                                                                     &kFuente16x32};

/* Constantes privadas para la decodificación de caracteres y fuentes. */
static const uint8_t kLCDMascaraIndiceBit           = 7u;
static const uint8_t kLCDMascaraBitMasSignificativo = 0x80u;
static const uint8_t kLCDPrefijoUTF8Latin1Inicio    = 0xC2u;
static const uint8_t kLCDPrefijoUTF8Latin1Fin       = 0xC3u;
static const uint8_t kLCDMascaraUTF8Continuacion    = 0xC0u;
static const uint8_t kLCDPrefijoUTF8Continuacion    = 0x80u;
static const uint8_t kLCDDesplazamientoUTF8         = 6u;
static const uint8_t kLCDMascaraUTF8Datos           = 0x3Fu;
static const uint8_t kLCDCaracteresPorTabulador     = 8u;

// ===== LCD - Variables Privadas =====
/**
 * @brief   Configuración actual del texto de lcd_printf().
 * @ingroup LCD
 * @private
 */
static lcd_config_texto_t g_lcd_texto_actual = {.color                   = BLANCO,
                                                .color_fondo             = NEGRO,
                                                .fuente                  = FUENTE16X32,
                                                .pos_x                   = 0,
                                                .pos_y                   = 0,
                                                .desplazamiento_activado = TRUE};

//! Búfer donde se compone un carácter antes de enviarlo a la pantalla de una sola ráfaga.
static uint16_t g_lcd_bufer_caracter[LCD_PIXELES_CARACTER_MAX];

//! Búfer donde lcd_printf() y lcd_xprintf() componen el texto con vsnprintf().
static char g_lcd_bufer_printf[LCD_MAX_TEXTO_PRINTF];

// ===== LCD - Funciones Privadas =====
/**
 * @brief   Rellena con un color el rectángulo de vértices (x0, y0) y (x1, y1), recortado a la
 * pantalla. No toma el bus.
 * @ingroup LCD
 * @private
 *
 * @details Es la operación más rápida de la biblioteca: una ventana y un solo color repetido, que
 * lcd_panel_repetir_color() envía sin necesidad de ningún búfer grande. La usan también las
 * líneas horizontales y verticales, el borrado y los círculos.
 */
static void lcd_dibujar_relleno(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) {

  int32_t temp;

  if (x0 > x1) {
    temp = x0;
    x0   = x1;
    x1   = temp;
  }
  if (y0 > y1) {
    temp = y0;
    y0   = y1;
    y1   = temp;
  }

  /* Recorte a los límites de la pantalla. Si no queda nada dentro, no se dibuja nada. */
  if (x1 < kLCDXMinimo || y1 < kLCDYMinimo || x0 > kLCDXMaximo || y0 > kLCDYMaximo) {
    return;
  }
  if (x0 < kLCDXMinimo) {
    x0 = kLCDXMinimo;
  }
  if (y0 < kLCDYMinimo) {
    y0 = kLCDYMinimo;
  }
  if (x1 > kLCDXMaximo) {
    x1 = kLCDXMaximo;
  }
  if (y1 > kLCDYMaximo) {
    y1 = kLCDYMaximo;
  }

  lcd_panel_ventana((uint16_t) x0, (uint16_t) y0, (uint16_t) x1, (uint16_t) y1);
  lcd_panel_repetir_color(color, (uint32_t) (x1 - x0 + 1) * (uint32_t) (y1 - y0 + 1));
}

/**
 * @brief   Dibuja un punto si está dentro de la pantalla. No toma el bus.
 * @ingroup LCD
 * @private
 */
static void lcd_dibujar_punto(int32_t x, int32_t y, uint16_t color) {

  if (x < kLCDXMinimo || x > kLCDXMaximo || y < kLCDYMinimo || y > kLCDYMaximo) {
    return;
  }

  lcd_panel_ventana((uint16_t) x, (uint16_t) y, (uint16_t) x, (uint16_t) y);
  lcd_panel_escribir_pixeles(&color, 1u);
}

/**
 * @brief   Dibuja un segmento de recta con el algoritmo de Bresenham. No toma el bus.
 * @ingroup LCD
 * @private
 *
 * @details Es el algoritmo de glcd_linea(), con un atajo: si el segmento es horizontal o
 * vertical, es un rectángulo de un píxel de grueso, y se dibuja de una sola ráfaga en lugar de
 * punto a punto.
 */
static void lcd_dibujar_linea(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) {

  int32_t x;       // Columna del punto que se está dibujando
  int32_t y;       // Fila del punto que se está dibujando
  int32_t dx;      // Distancia horizontal entre extremos (en valor absoluto)
  int32_t dy;      // Distancia vertical entre extremos (en valor absoluto)
  int32_t p;       // Variable de decisión de Bresenham: su signo elige el siguiente píxel
  int32_t inc_e;   // Incremento de p al avanzar solo en el eje principal (paso E)
  int32_t inc_ne;  // Incremento de p al avanzar también en el eje secundario (paso NE)
  int32_t stepx;   // Sentido de avance en x: +1 o -1
  int32_t stepy;   // Sentido de avance en y: +1 o -1

  if (x0 == x1 || y0 == y1) {
    lcd_dibujar_relleno(x0, y0, x1, y1, color);
    return;
  }

  dx = x1 - x0;
  dy = y1 - y0;

  if (dy < 0) {
    dy    = -dy;
    stepy = -1;
  } else {
    stepy = 1;
  }

  if (dx < 0) {
    dx    = -dx;
    stepx = -1;
  } else {
    stepx = 1;
  }

  x = x0;
  y = y0;
  lcd_dibujar_punto(x, y, color);

  if (dx > dy) {
    p      = 2 * dy - dx;
    inc_e  = 2 * dy;
    inc_ne = 2 * (dy - dx);
    while (x != x1) {
      x = x + stepx;
      if (p < 0) {
        p = p + inc_e;
      } else {
        y = y + stepy;
        p = p + inc_ne;
      }
      lcd_dibujar_punto(x, y, color);
    }
  } else {
    p      = 2 * dx - dy;
    inc_e  = 2 * dx;
    inc_ne = 2 * (dx - dy);
    while (y != y1) {
      y = y + stepy;
      if (p < 0) {
        p = p + inc_e;
      } else {
        x = x + stepx;
        p = p + inc_ne;
      }
      lcd_dibujar_punto(x, y, color);
    }
  }
}

/**
 * @brief   Dibuja un carácter (código Latin-1) con su fondo. No toma el bus.
 * @ingroup LCD
 * @private
 *
 * @details Recorre el patrón de puntos del carácter bit a bit, igual que glcd_caracter(), pero en
 * lugar de escribir cada píxel en el *framebuffer* lo guarda en g_lcd_bufer_caracter; al terminar,
 * elige como ventana el rectángulo del carácter y lo envía de una sola ráfaga. Si el carácter se
 * sale por la derecha o por abajo, la ventana se recorta y los píxeles que caen fuera no se
 * guardan.
 */
static void lcd_dibujar_caracter(uint8_t c, uint16_t x, uint16_t y, uint16_t color,
                                 uint16_t color_fondo, uint32_t fuente) {

  const fuente_t *fuente_ptr;
  uint16_t        pixeles_ancho;
  uint16_t        pixeles_alto;
  uint16_t        ancho_visible;
  uint16_t        alto_visible;
  uint16_t        bytes_por_fila;
  uint16_t        i;
  uint16_t        j;
  uint32_t        n = 0;
  const uint8_t  *ptr_df;
  uint8_t         b       = 0;
  uint8_t         mascara = 0;

  if (fuente >= LCD_NUMERO_FUENTES || x > kLCDXMaximo || y > kLCDYMaximo) {
    return;
  }

  fuente_ptr = kLCDTablaFuentes[fuente];
  if (c < fuente_ptr->primer_caracter || c > fuente_ptr->ultimo_caracter) {
    return;
  }

  pixeles_ancho  = fuente_ptr->pixeles_ancho;
  pixeles_alto   = fuente_ptr->pixeles_alto;
  bytes_por_fila = pixeles_ancho >> 3;
  if ((pixeles_ancho & kLCDMascaraIndiceBit) != 0) {
    bytes_por_fila++;
  }

  /* Parte del carácter que cae dentro de la pantalla. */
  ancho_visible = pixeles_ancho;
  if (x + pixeles_ancho - 1u > kLCDXMaximo) {
    ancho_visible = kLCDXMaximo - x + 1u;
  }
  alto_visible = pixeles_alto;
  if (y + pixeles_alto - 1u > kLCDYMaximo) {
    alto_visible = kLCDYMaximo - y + 1u;
  }

  ptr_df = fuente_ptr->datos +
           (uint32_t) (c - fuente_ptr->primer_caracter) * bytes_por_fila * pixeles_alto;

  for (i = 0; i < alto_visible; i++) {
    for (j = 0; j < pixeles_ancho; j++) {
      if ((j & kLCDMascaraIndiceBit) == 0) {
        b       = *ptr_df++;
        mascara = kLCDMascaraBitMasSignificativo;
      } else {
        mascara >>= 1;
      }

      if (j < ancho_visible) {
        g_lcd_bufer_caracter[n++] = (b & mascara) ? color : color_fondo;
      }
    }
  }

  lcd_panel_ventana(x, y, x + ancho_visible - 1u, y + alto_visible - 1u);
  lcd_panel_escribir_pixeles(g_lcd_bufer_caracter, n);
}

/**
 * @brief   Lee el siguiente carácter de una cadena UTF-8 y lo devuelve en código Latin-1.
 * @ingroup LCD
 * @private
 *
 * @details Los caracteres del 128 al 255 de Latin-1 (á, ñ, ¿, º...) se escriben en UTF-8 con
 * dos bytes: 0xC2 o 0xC3 seguido de un byte 10xxxxxx. Esta función los junta en uno solo. Los
 * demás bytes se devuelven tal cual (si la cadena ya estaba en Latin-1, también sale bien, salvo
 * en el caso improbable de que un 0xC2 o 0xC3 vaya seguido de un byte de 0x80 a 0xBF).
 *
 * @param[in,out]   ptr_cadena  Puntero al puntero a la cadena: se avanza uno o dos bytes.
 *
 * @return  El código Latin-1 del carácter.
 */
static uint8_t lcd_siguiente_caracter(const char **ptr_cadena) {

  const uint8_t *p = (const uint8_t *) *ptr_cadena;

  if ((p[0] == kLCDPrefijoUTF8Latin1Inicio || p[0] == kLCDPrefijoUTF8Latin1Fin) &&
      (p[1] & kLCDMascaraUTF8Continuacion) == kLCDPrefijoUTF8Continuacion) {
    *ptr_cadena += 2;
    return (uint8_t) (((p[0] & 0x03u) << kLCDDesplazamientoUTF8) | (p[1] & kLCDMascaraUTF8Datos));
  }

  *ptr_cadena += 1;
  return p[0];
}

/**
 * @brief   Borra la pantalla con un color (el trabajo de lcd_desplazar()). No toma el bus.
 * @ingroup LCD
 * @private
 */
static void lcd_dibujar_borrado(uint16_t color) {
  lcd_dibujar_relleno(kLCDXMinimo, kLCDYMinimo, kLCDXMaximo, kLCDYMaximo, color);
}

/**
 * @brief   Imprime un carácter en la posición de una configuración de texto y la avanza,
 * interpretando los caracteres de control. No toma el bus.
 * @ingroup LCD
 * @private
 *
 * @details Es la antigua fputc() de glcd.c, que ya no se usa como salida de printf: lcd_printf()
 * compone el texto con vsnprintf() y lo pasa aquí carácter a carácter.
 *
 * @param[in,out]   ptr_texto  Configuración de texto que se usa y se actualiza.
 * @param[in]       c          Carácter (Latin-1) a imprimir.
 */
static void lcd_salida_caracter(lcd_config_texto_t *ptr_texto, uint8_t c) {

  const fuente_t *fuente_ptr   = kLCDTablaFuentes[ptr_texto->fuente];
  uint16_t        ancho        = fuente_ptr->pixeles_ancho;
  uint16_t        alto         = fuente_ptr->pixeles_alto;
  uint16_t        incremento_y = 0;
  bool_t          imprimible   = FALSE;

  switch (c) {
    case '\b':  // Retroceso
      ptr_texto->pos_x = (ptr_texto->pos_x >= ancho) ? (uint16_t) (ptr_texto->pos_x - ancho) : 0u;
      break;
    case '\f':  // Salto de página
      ptr_texto->pos_x = 0;
      incremento_y     = kLCDTamanoY;
      break;
    case '\n':  // Salto de línea
      ptr_texto->pos_x = 0;
      incremento_y     = alto;
      break;
    case '\r':  // Retorno de carro
      ptr_texto->pos_x = 0;
      break;
    case '\t':  // Tabulador horizontal: cada 8 caracteres
      ptr_texto->pos_x =
          (uint16_t) (((ptr_texto->pos_x / (kLCDCaracteresPorTabulador * ancho)) + 1u) *
                      kLCDCaracteresPorTabulador * ancho);
      break;
    case '\v':  // Tabulador vertical
      incremento_y = alto;
      break;
    default:
      imprimible = (c >= fuente_ptr->primer_caracter && c <= fuente_ptr->ultimo_caracter);
      break;
  }

  /* Si el carácter no cabe en lo que queda de línea (o un tabulador se ha salido), se pasa a la
   * línea siguiente.
   */
  if ((imprimible && ptr_texto->pos_x + ancho - 1u > kLCDXMaximo) ||
      ptr_texto->pos_x > kLCDXMaximo) {
    ptr_texto->pos_x = 0;
    if (incremento_y < alto) {
      incremento_y = alto;
    }
  }

  /* Si la línea no cabe en la pantalla, se vuelve arriba; con el desplazamiento automático
   * activado, borrando antes la pantalla (ver lcd_desplazar()).
   */
  ptr_texto->pos_y = (uint16_t) (ptr_texto->pos_y + incremento_y);
  if (ptr_texto->pos_y + alto - 1u > kLCDYMaximo) {
    if (ptr_texto->desplazamiento_activado) {
      lcd_dibujar_borrado(ptr_texto->color_fondo);
    }
    ptr_texto->pos_y = 0;
  }

  if (imprimible) {
    lcd_dibujar_caracter(c, ptr_texto->pos_x, ptr_texto->pos_y, ptr_texto->color,
                         ptr_texto->color_fondo, ptr_texto->fuente);
    ptr_texto->pos_x = (uint16_t) (ptr_texto->pos_x + ancho);
  }
}

/**
 * @brief   Compone el texto con vsnprintf() y lo imprime con una configuración de texto. Debe
 * llamarse con el bus tomado (g_lcd_bufer_printf es compartido).
 * @ingroup LCD
 * @private
 *
 * @return  Lo que devuelve vsnprintf().
 */
static int32_t lcd_vprintf(lcd_config_texto_t *ptr_texto, const char *ptr_format, va_list args) {

  int32_t     retval;
  const char *ptr = g_lcd_bufer_printf;

  retval = (int32_t) vsnprintf(g_lcd_bufer_printf, sizeof(g_lcd_bufer_printf), ptr_format, args);

  if (retval > 0) {
    while (*ptr != '\0') {
      lcd_salida_caracter(ptr_texto, lcd_siguiente_caracter(&ptr));
    }
  }

  return retval;
}

// ===== LCD - Funciones Publicas =====
void lcd_inicializar(void) {
  lcd_panel_inicializar();
}

void lcd_borrar(uint16_t color) {
  lcd_panel_tomar_bus();
  lcd_dibujar_borrado(color);
  lcd_panel_soltar_bus();
}

void lcd_color_texto(uint16_t color) {
  g_lcd_texto_actual.color = color;
}

void lcd_fondo_texto(uint16_t color_fondo) {
  g_lcd_texto_actual.color_fondo = color_fondo;
}

void lcd_xy_texto(uint16_t x, uint16_t y) {

  if (x > kLCDXMaximo || y > kLCDYMaximo) {
    return;
  }
  g_lcd_texto_actual.pos_x = x;
  g_lcd_texto_actual.pos_y = y;
}

void lcd_desplazar(uint16_t lineas) {

  (void) lineas;

  lcd_panel_tomar_bus();
  lcd_dibujar_borrado(g_lcd_texto_actual.color_fondo);
  lcd_panel_soltar_bus();

  g_lcd_texto_actual.pos_y = 0;
}

void lcd_activar_desplazamiento(bool_t activo) {
  g_lcd_texto_actual.desplazamiento_activado = activo;
}

void lcd_seleccionar_fuente(uint32_t fuente) {

  if (fuente >= LCD_NUMERO_FUENTES) {
    return;
  }
  g_lcd_texto_actual.fuente = fuente;
}

int32_t lcd_printf(const char *ptr_format, ...) {

  int32_t retval;
  va_list args;

  lcd_panel_tomar_bus();
  va_start(args, ptr_format);
  retval = lcd_vprintf(&g_lcd_texto_actual, ptr_format, args);
  va_end(args);
  lcd_panel_soltar_bus();

  return retval;
}

int32_t lcd_xprintf(uint16_t x, uint16_t y, uint16_t color, uint16_t color_fondo, uint32_t fuente,
                    const char *ptr_format, ...) {

  int32_t            retval;
  va_list            args;
  lcd_config_texto_t texto;

  if (fuente >= LCD_NUMERO_FUENTES) {
    return -1;
  }

  /* Una configuración de texto local, con la posición, colores y fuente indicados: así la de
   * lcd_printf() no se toca, ni siquiera un momento.
   */
  texto.color                   = color;
  texto.color_fondo             = color_fondo;
  texto.fuente                  = fuente;
  texto.pos_x                   = x;
  texto.pos_y                   = y;
  texto.desplazamiento_activado = g_lcd_texto_actual.desplazamiento_activado;

  lcd_panel_tomar_bus();
  va_start(args, ptr_format);
  retval = lcd_vprintf(&texto, ptr_format, args);
  va_end(args);
  lcd_panel_soltar_bus();

  return retval;
}

void lcd_caracter(char c, uint16_t x, uint16_t y, uint16_t color, uint16_t color_fondo,
                  uint32_t fuente) {

  lcd_panel_tomar_bus();
  lcd_dibujar_caracter((uint8_t) c, x, y, color, color_fondo, fuente);
  lcd_panel_soltar_bus();
}

void lcd_texto(uint16_t x, uint16_t y, uint16_t color, uint16_t color_fondo, uint32_t fuente,
               const char *ptr_str) {

  const fuente_t *fuente_ptr;
  uint8_t         c;

  if (ptr_str == NULL || fuente >= LCD_NUMERO_FUENTES || x > kLCDXMaximo || y > kLCDYMaximo) {
    return;
  }
  fuente_ptr = kLCDTablaFuentes[fuente];

  lcd_panel_tomar_bus();

  while (*ptr_str != '\0') {
    c = lcd_siguiente_caracter(&ptr_str);

    /* Si no cabe en la línea, a la siguiente; si no cabe en la pantalla, arriba. */
    if (x + fuente_ptr->pixeles_ancho - 1u > kLCDXMaximo) {
      x = 0;
      y = (uint16_t) (y + fuente_ptr->pixeles_alto);
    }
    if (y + fuente_ptr->pixeles_alto - 1u > kLCDYMaximo) {
      y = 0;
    }

    lcd_dibujar_caracter(c, x, y, color, color_fondo, fuente);
    x = (uint16_t) (x + fuente_ptr->pixeles_ancho);
  }

  lcd_panel_soltar_bus();
}

void lcd_punto(uint16_t x, uint16_t y, uint16_t color) {
  lcd_panel_tomar_bus();
  lcd_dibujar_punto(x, y, color);
  lcd_panel_soltar_bus();
}

void lcd_linea(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
  lcd_panel_tomar_bus();
  lcd_dibujar_linea(x0, y0, x1, y1, color);
  lcd_panel_soltar_bus();
}

void lcd_rectangulo(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
  lcd_panel_tomar_bus();
  lcd_dibujar_linea(x0, y0, x1, y0, color);
  lcd_dibujar_linea(x1, y0, x1, y1, color);
  lcd_dibujar_linea(x1, y1, x0, y1, color);
  lcd_dibujar_linea(x0, y1, x0, y0, color);
  lcd_panel_soltar_bus();
}

void lcd_rectangulo_relleno(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
  lcd_panel_tomar_bus();
  lcd_dibujar_relleno(x0, y0, x1, y1, color);
  lcd_panel_soltar_bus();
}

void lcd_circunferencia(uint16_t xc, uint16_t yc, uint16_t radio, uint16_t color) {

  int32_t x   = radio;
  int32_t y   = 0;
  int32_t err = 0;
  int32_t cx  = xc;
  int32_t cy  = yc;

  lcd_panel_tomar_bus();

  /* Algoritmo del punto medio: se calcula un octante y se dibujan los ocho puntos simétricos. */
  while (x >= y) {
    lcd_dibujar_punto(cx + x, cy + y, color);
    lcd_dibujar_punto(cx + y, cy + x, color);
    lcd_dibujar_punto(cx - y, cy + x, color);
    lcd_dibujar_punto(cx - x, cy + y, color);
    lcd_dibujar_punto(cx - x, cy - y, color);
    lcd_dibujar_punto(cx - y, cy - x, color);
    lcd_dibujar_punto(cx + y, cy - x, color);
    lcd_dibujar_punto(cx + x, cy - y, color);

    if (err <= 0) {
      y   += 1;
      err += 2 * y + 1;
    } else {
      x   -= 1;
      err -= 2 * x + 1;
    }
  }

  lcd_panel_soltar_bus();
}

void lcd_circulo(uint16_t xc, uint16_t yc, uint16_t radio, uint16_t color) {

  int32_t x   = radio;
  int32_t y   = 0;
  int32_t err = 0;
  int32_t cx  = xc;
  int32_t cy  = yc;

  lcd_panel_tomar_bus();

  /* El mismo recorrido, pero uniendo cada pareja de puntos simétricos con una línea horizontal,
   * que se dibuja de una sola ráfaga.
   */
  while (x >= y) {
    lcd_dibujar_linea(cx - x, cy - y, cx + x, cy - y, color);
    lcd_dibujar_linea(cx - x, cy + y, cx + x, cy + y, color);
    lcd_dibujar_linea(cx - y, cy - x, cx + y, cy - x, color);
    lcd_dibujar_linea(cx - y, cy + x, cx + y, cy + x, color);

    if (err <= 0) {
      y   += 1;
      err += 2 * y + 1;
    } else {
      x   -= 1;
      err -= 2 * x + 1;
    }
  }

  lcd_panel_soltar_bus();
}
