/**
 * @file    lcd_lvgl.c
 * @brief   Dibujo en modo inmediato EMULADO sobre LVGL: un objeto LVGL por llamada.
 *
 * @details Leer primero la cabecera lcd_lvgl.h, que explica la diferencia entre modo inmediato
 * y modo retenido. Resumen de la traducción que hace este fichero:
 *
 *   | Llamada (modo inmediato)      | Objeto LVGL que crea (modo retenido)                     |
 *   |-------------------------------|----------------------------------------------------------|
 *   | lcd_lvgl_printf / xprintf     | lv_label por cada tramo de texto; se REUTILIZA si ya hay |
 *   | lcd_lvgl_texto / caracter     | una etiqueta de esta librería en la misma (x, y)         |
 *   | lcd_lvgl_rectangulo           | lv_obj con borde de 1 píxel y fondo transparente         |
 *   | lcd_lvgl_rectangulo_relleno   | lv_obj con fondo opaco                                   |
 *   | lcd_lvgl_circunferencia       | lv_obj con borde y radio LV_RADIUS_CIRCLE                |
 *   | lcd_lvgl_circulo              | lv_obj con fondo opaco y radio LV_RADIUS_CIRCLE          |
 *   | lcd_lvgl_linea                | lv_line con dos puntos                                   |
 *   | lcd_lvgl_punto                | lv_obj de 1 x 1 con fondo opaco                          |
 *   | lcd_lvgl_borrar               | lv_obj_clean() de la pantalla + color de fondo           |
 *
 * Todos los objetos son hijos del LIENZO (un lv_obj transparente de 480 x 320 al fondo de la
 * pantalla activa). Ninguno se pinta al crearse: LVGL marca su zona como "sucia" y la repinta en
 * el siguiente lcd_lvgl_procesar() (o en lcd_lvgl_refrescar_ya()).
 *
 * Todas las funciones públicas siguen el mismo patrón:
 *     lv_lock();   ...crear o modificar objetos...   lv_unlock();
 * porque LVGL no es reentrante: con FreeRTOS, la tarea de interfaz puede estar dentro de
 * lv_timer_handler() recorriendo el árbol de objetos mientras otra tarea quiere añadir uno.
 * Las funciones privadas (sufijo _sin_cerrojo o auxiliares) suponen que el cerrojo ya está
 * tomado.
 *
 * @author    Alejandro Lara Doña - alejandro.lara(at)uca.es
 * @date      2026
 * @version   1.0
 *
 * @copyright GNU General Public License version 3 or later
 */

#include "lcd_lvgl.h"
#include <stdarg.h>
#include <stdio.h>
#include "lcd_panel.h"
#include "tipos.h"
#include "lcd_lvgl_port.h"
#include "lvgl.h"

// ===== LCD_LVGL - Tipos Privados =====
/**
 * @brief   Una etiqueta creada por esta librería y la posición en la que se creó, para poder
 * reutilizarla cuando se vuelva a escribir en esa misma posición.
 * @ingroup LCD_LVGL
 * @private
 */
typedef struct {
  lv_obj_t *etiqueta;  //!< La etiqueta (NULL si la entrada está libre).
  int32_t   x;         //!< Coordenada X con la que se creó.
  int32_t   y;         //!< Coordenada Y con la que se creó (se corrige al desplazar).
} lcd_lvgl_etiqueta_t;

/**
 * @brief   Configuración del texto de lcd_lvgl_printf(), como glcd_config_texto_t.
 * @ingroup LCD_LVGL
 * @private
 */
typedef struct {
  uint16_t color;                    //!< Color del texto.
  uint16_t color_fondo;              //!< Color de fondo del texto.
  uint32_t fuente;                   //!< Fuente (FUENTE8X16...).
  int32_t  pos_x;                    //!< Posición X del cursor de texto.
  int32_t  pos_y;                    //!< Posición Y del cursor de texto.
  bool_t   desplazamiento_activado;  //!< TRUE: al llegar abajo se desplaza el contenido.
} lcd_lvgl_config_texto_t;

// ==== LCD_LVGL - Constantes Privadas =====
/**
 * @brief   Constantes privadas para la conversión de color y el formato de texto.
 * @ingroup LCD_LVGL
 * @private
*/
static const uint8_t kMascaraCanalRGB565RojoAzul = 0xF8u;
static const uint8_t kMascaraCanalRGB565Verde    = 0xFCu;
static const uint8_t kCaracteresPorTabulador     = 8u;

// ===== LCD_LVGL - Variables Privadas =====
/** Las tres fuentes de glcd traducidas a fuentes de LVGL (activadas en lv_conf.h). */
static const lv_font_t *const kTablaFuentes[NUMERO_FUENTES] = {
    &lv_font_unscii_16,      // FUENTE8X16:  monoespaciada, 8 x 16, como la de glcd.
    &lv_font_montserrat_24,  // FUENTE12X24: proporcional.
    &lv_font_montserrat_32   // FUENTE16X32: proporcional.
};

static lv_obj_t           *g_lienzo = NULL;                      //!< Padre de todo lo dibujado.
static lcd_lvgl_etiqueta_t g_etiquetas[LCD_LVGL_MAX_ETIQUETAS];  //!< Etiquetas reutilizables.
static bool_t              g_inicializado = FALSE;               //!< TRUE tras inicializar.

/** Configuración de texto actual. Mismos valores iniciales que glcd. */
static lcd_lvgl_config_texto_t g_texto_actual = {.color                   = BLANCO,
                                                 .color_fondo             = NEGRO,
                                                 .fuente                  = FUENTE16X32,
                                                 .pos_x                   = 0,
                                                 .pos_y                   = 0,
                                                 .desplazamiento_activado = TRUE};

// ===== LCD_LVGL - Funciones Privadas =====
/**
 * @brief   Convierte un color RGB565 de 16 bits (el de glcd) al tipo de color de LVGL.
 * @ingroup LCD_LVGL
 * @private
 *
 * @details lv_color_t guarda 8 bits por canal. Se extraen los 5 bits de rojo (15..11), los 6 de
 * verde (10..5) y los 5 de azul (4..0), y se colocan en la parte alta de cada byte, que es lo
 * que hace la macro RGB() al revés. LVGL los vuelve a reducir a RGB565 al dibujar, porque
 * LV_COLOR_DEPTH vale 16, sin pérdida.
 */
static lv_color_t lcd_lvgl_color(uint16_t color) {
  return lv_color_make(
      (uint8_t) ((color >> 8) & kMascaraCanalRGB565RojoAzul),  // NOLINT(readability-magic-numbers)
      (uint8_t) ((color >> 3) & kMascaraCanalRGB565Verde),
      (uint8_t) ((color << 3) & kMascaraCanalRGB565RojoAzul));
}

/**
 * @brief   Evento LV_EVENT_DELETE de una etiqueta: la quita de la tabla de reutilización.
 * @ingroup LCD_LVGL
 * @private
 *
 * @details LVGL avisa cuando destruye un objeto, lo destruya quien lo destruya (lcd_lvgl_borrar,
 * lcd_lvgl_desplazar, o el propio programa con lv_obj_clean()). Así la tabla nunca apunta a una
 * etiqueta que ya no existe.
 */
static void lcd_lvgl_olvidar_etiqueta(lv_event_t *ptr_evento) {
  lv_obj_t *objeto = lv_event_get_target_obj(ptr_evento);
  uint32_t  i;

  for (i = 0; i < LCD_LVGL_MAX_ETIQUETAS; i++) {
    if (g_etiquetas[i].etiqueta == objeto) {
      g_etiquetas[i].etiqueta = NULL;
    }
  }
}

/**
 * @brief   Evento LV_EVENT_DELETE del lienzo: olvida el puntero para crear otro cuando haga falta.
 * @ingroup LCD_LVGL
 * @private
 */
static void lcd_lvgl_olvidar_lienzo(lv_event_t *ptr_evento) {
  if (lv_event_get_target_obj(ptr_evento) == g_lienzo) {
    g_lienzo = NULL;
  }
}

/**
 * @brief   Evento LV_EVENT_DELETE de una línea: libera el vector de puntos.
 * @ingroup LCD_LVGL
 * @private
 *
 * @details lv_line NO copia los puntos: guarda el puntero. Por eso el vector se reserva con
 * lv_malloc() al crear la línea y se libera aquí, cuando LVGL destruye la línea.
 */
static void lcd_lvgl_liberar_puntos(lv_event_t *ptr_evento) {
  lv_free(lv_event_get_user_data(ptr_evento));
}

/**
 * @brief   Olvida todas las etiquetas de la tabla (no las destruye).
 * @ingroup LCD_LVGL
 * @private
 */
static void lcd_lvgl_olvidar_etiquetas(void) {
  uint32_t i;

  for (i = 0; i < LCD_LVGL_MAX_ETIQUETAS; i++) {
    g_etiquetas[i].etiqueta = NULL;
  }
}

/**
 * @brief   Devuelve el lienzo de la pantalla activa, creándolo si no existe.
 * @ingroup LCD_LVGL
 * @private
 *
 * @details El lienzo es un lv_obj sin estilos (transparente, sin borde ni márgenes) del tamaño
 * de la pantalla, que no se puede pulsar ni desplazar, para que los toques lleguen a los widgets.
 * Se coloca el primero de los hijos de la pantalla (índice 0): así lo que dibuja lcd_lvgl_* queda
 * DETRÁS de los widgets. Si el programa ha cambiado de pantalla (lv_screen_load()), se crea un
 * lienzo nuevo en la pantalla nueva.
 */
static lv_obj_t *lcd_lvgl_obtener_lienzo(void) {
  lv_obj_t *pantalla = lv_screen_active();

  if ((g_lienzo == NULL) || (lv_obj_get_parent(g_lienzo) != pantalla)) {
    lcd_lvgl_olvidar_etiquetas();
    g_lienzo = lv_obj_create(pantalla);
    lv_obj_remove_style_all(g_lienzo);
    lv_obj_set_pos(g_lienzo, 0, 0);
    lv_obj_set_size(g_lienzo, (int32_t) LCD_PANEL_ANCHO, (int32_t) LCD_PANEL_ALTO);
    lv_obj_remove_flag(g_lienzo, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(g_lienzo, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_to_index(g_lienzo, 0);
    lv_obj_add_event_cb(g_lienzo, lcd_lvgl_olvidar_lienzo, LV_EVENT_DELETE, NULL);
  }
  return g_lienzo;
}

/**
 * @brief   Crea en el lienzo un objeto rectangular vacío (sin estilos) en la posición y con el
 * tamaño indicados. Es la base de rectángulos, círculos y puntos.
 * @ingroup LCD_LVGL
 * @private
 */
static lv_obj_t *lcd_lvgl_crear_figura(int32_t x, int32_t y, int32_t ancho, int32_t alto) {
  lv_obj_t *figura = lv_obj_create(lcd_lvgl_obtener_lienzo());

  lv_obj_remove_style_all(figura);  // Sin el tema: ni fondo, ni borde, ni márgenes.
  lv_obj_set_pos(figura, x, y);
  lv_obj_set_size(figura, ancho, alto);
  lv_obj_remove_flag(figura, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(figura, LV_OBJ_FLAG_SCROLLABLE);
  return figura;
}

/**
 * @brief   Muestra un texto en (x, y) reutilizando la etiqueta de esa posición si existe o
 * creando una nueva.
 * @ingroup LCD_LVGL
 * @private
 *
 * @details lv_label_set_text() COPIA el texto en memoria de LVGL, así que el búfer del que
 * viene puede ser una variable local. La etiqueta reutilizada se pasa al frente
 * (lv_obj_move_to_index(..., -1)): como en modo inmediato, lo último escrito queda encima.
 */
static void lcd_lvgl_escribir_etiqueta(int32_t x, int32_t y, uint16_t color, uint16_t color_fondo,
                                       uint32_t fuente, const char *ptr_texto) {
  lv_obj_t *etiqueta = NULL;
  uint32_t  i;

  // ¿Hay ya una etiqueta nuestra en esta posición? Entonces se reutiliza.
  for (i = 0; (i < LCD_LVGL_MAX_ETIQUETAS) && (etiqueta == NULL); i++) {
    if ((g_etiquetas[i].etiqueta != NULL) && (g_etiquetas[i].x == x) && (g_etiquetas[i].y == y)) {
      etiqueta = g_etiquetas[i].etiqueta;
      lv_obj_move_to_index(etiqueta, -1);
    }
  }

  // Si no, se crea una y se anota en la primera entrada libre (si no hay, no se anota: se
  // dibuja igual, pero no se podrá reutilizar).
  if (etiqueta == NULL) {
    etiqueta = lv_label_create(lcd_lvgl_obtener_lienzo());
    lv_obj_remove_style_all(etiqueta);
    lv_obj_set_size(etiqueta, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_pos(etiqueta, x, y);
    lv_obj_add_event_cb(etiqueta, lcd_lvgl_olvidar_etiqueta, LV_EVENT_DELETE, NULL);
    for (i = 0; i < LCD_LVGL_MAX_ETIQUETAS; i++) {
      if (g_etiquetas[i].etiqueta == NULL) {
        g_etiquetas[i].etiqueta = etiqueta;
        g_etiquetas[i].x        = x;
        g_etiquetas[i].y        = y;
        break;
      }
    }
  }

  lv_obj_set_style_text_font(etiqueta, kTablaFuentes[fuente], LV_PART_MAIN);
  lv_obj_set_style_text_color(etiqueta, lcd_lvgl_color(color), LV_PART_MAIN);
  lv_obj_set_style_bg_color(etiqueta, lcd_lvgl_color(color_fondo), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(etiqueta, LV_OPA_COVER, LV_PART_MAIN);
  lv_label_set_text(etiqueta, ptr_texto);
}

/**
 * @brief   Desplaza hacia arriba todos los objetos del lienzo (sin tomar el cerrojo).
 * @ingroup LCD_LVGL
 * @private
 *
 * @details En modo inmediato se movían píxeles de la memoria de imagen; aquí se mueven
 * OBJETOS: a cada hijo del lienzo se le resta `lineas` a su coordenada Y, y el que queda
 * entero por encima del borde superior se destruye (así no se acumulan objetos invisibles). Se
 * recorre la lista de hijos de atrás hacia delante porque destruir un hijo corre los índices de
 * los siguientes.
 */
static void lcd_lvgl_desplazar_sin_cerrojo(int32_t lineas) {
  lv_obj_t *padre = lcd_lvgl_obtener_lienzo();
  lv_obj_t *hijo;
  uint32_t  i;
  int32_t   y;

  lv_obj_update_layout(padre);  // Que las coordenadas de los hijos estén al día.

  i = lv_obj_get_child_count(padre);
  while (i > 0u) {
    i--;
    hijo = lv_obj_get_child(padre, (int32_t) i);
    y    = lv_obj_get_y(hijo) - lineas;
    if ((y + lv_obj_get_height(hijo)) <= 0) {
      lv_obj_delete(hijo);  // Su evento LV_EVENT_DELETE la quita de la tabla de etiquetas.
    } else {
      lv_obj_set_y(hijo, y);
    }
  }

  for (i = 0; i < LCD_LVGL_MAX_ETIQUETAS; i++) {
    if (g_etiquetas[i].etiqueta != NULL) {
      g_etiquetas[i].y -= lineas;
    }
  }
}

/**
 * @brief   Baja el cursor de texto `incremento` píxeles y, si la línea ya no cabe, desplaza el
 * contenido (o vuelve arriba si el desplazamiento está desactivado), como hacía glcd.
 * @ingroup LCD_LVGL
 * @private
 */
static void lcd_lvgl_bajar_cursor(int32_t incremento, int32_t alto_linea) {
  int32_t exceso;

  g_texto_actual.pos_y += incremento;
  exceso                = g_texto_actual.pos_y + alto_linea - (int32_t) LCD_PANEL_ALTO;
  if (exceso > 0) {
    if (g_texto_actual.desplazamiento_activado) {
      if (exceso > (int32_t) LCD_PANEL_ALTO) {
        exceso = (int32_t) LCD_PANEL_ALTO;
      }
      lcd_lvgl_desplazar_sin_cerrojo(exceso);
      g_texto_actual.pos_y -= exceso;
      if (g_texto_actual.pos_y < 0) {
        g_texto_actual.pos_y = 0;
      }
    } else {
      g_texto_actual.pos_y = 0;
    }
  }
}

/**
 * @brief   Motor de texto de lcd_lvgl_printf() (sin tomar el cerrojo).
 * @ingroup LCD_LVGL
 * @private
 *
 * @details glcd pintaba carácter a carácter. Aquí se agrupan los caracteres imprimibles
 * seguidos en un TRAMO, y cada tramo se convierte en UNA etiqueta en la posición del cursor. Un
 * tramo se cierra al llegar un carácter de control ('\\n', '\\r', ...), al llegar al borde
 * derecho o al terminar el texto. El avance del cursor se calcula con el ancho real de cada
 * carácter en la fuente (lv_font_get_glyph_width()), que en Montserrat no es constante.
 */
static void lcd_lvgl_imprimir_sin_cerrojo(const char *ptr_texto) {
  const lv_font_t *fuente        = kTablaFuentes[g_texto_actual.fuente];
  int32_t          alto_linea    = lv_font_get_line_height(fuente);
  int32_t          ancho_espacio = (int32_t) lv_font_get_glyph_width(fuente, ' ', 0);
  int32_t          ancho_tramo   = 0;
  int32_t          ancho_c;
  char             tramo[LCD_LVGL_MAX_TEXTO + 1u];
  uint32_t         n = 0;
  char             c;

  if (ancho_espacio <= 0) {
    ancho_espacio = 1;
  }

  // Si la línea actual ya no cabe entera (cursor demasiado abajo), se desplaza antes de empezar.
  lcd_lvgl_bajar_cursor(0, alto_linea);

  while (*ptr_texto != '\0') {
    c = *ptr_texto++;

    if ((c >= ' ') && (c <= '~')) {
      // Carácter imprimible: se añade al tramo, saltando de línea si no cabe.
      ancho_c = (int32_t) lv_font_get_glyph_width(fuente, (uint32_t) c, 0);
      if ((g_texto_actual.pos_x + ancho_tramo + ancho_c) > (int32_t) LCD_PANEL_ANCHO) {
        if (n > 0u) {
          tramo[n] = '\0';
          lcd_lvgl_escribir_etiqueta(g_texto_actual.pos_x, g_texto_actual.pos_y,
                                     g_texto_actual.color, g_texto_actual.color_fondo,
                                     g_texto_actual.fuente, tramo);
          n           = 0;
          ancho_tramo = 0;
        }
        g_texto_actual.pos_x = 0;
        lcd_lvgl_bajar_cursor(alto_linea, alto_linea);
      }
      tramo[n++]   = c;
      ancho_tramo += ancho_c;
      continue;
    }

    // Carácter de control: primero se cierra el tramo pendiente.
    if (n > 0u) {
      tramo[n] = '\0';
      lcd_lvgl_escribir_etiqueta(g_texto_actual.pos_x, g_texto_actual.pos_y, g_texto_actual.color,
                                 g_texto_actual.color_fondo, g_texto_actual.fuente, tramo);
      g_texto_actual.pos_x += ancho_tramo;
      n                     = 0;
      ancho_tramo           = 0;
    }

    switch (c) {
      case '\n':  // Salto de línea.
        g_texto_actual.pos_x = 0;
        lcd_lvgl_bajar_cursor(alto_linea, alto_linea);
        break;
      case '\r':  // Retorno de carro.
        g_texto_actual.pos_x = 0;
        break;
      case '\t':  // Tabulador: siguiente múltiplo de 8 espacios.
        g_texto_actual.pos_x =
            ((g_texto_actual.pos_x / (kCaracteresPorTabulador * ancho_espacio)) + 1) * kCaracteresPorTabulador * ancho_espacio;
        if (g_texto_actual.pos_x >= (int32_t) LCD_PANEL_ANCHO) {
          g_texto_actual.pos_x = 0;
          lcd_lvgl_bajar_cursor(alto_linea, alto_linea);
        }
        break;
      case '\b':  // Retroceso de un espacio.
        g_texto_actual.pos_x =
            (g_texto_actual.pos_x > ancho_espacio) ? (g_texto_actual.pos_x - ancho_espacio) : 0;
        break;
      case '\v':  // Tabulador vertical.
        lcd_lvgl_bajar_cursor(alto_linea, alto_linea);
        break;
      case '\f':  // Salto de página.
        g_texto_actual.pos_x = 0;
        lcd_lvgl_bajar_cursor((int32_t) LCD_PANEL_ALTO, alto_linea);
        break;
      default:
        break;
    }
  }

  // Tramo final.
  if (n > 0u) {
    tramo[n] = '\0';
    lcd_lvgl_escribir_etiqueta(g_texto_actual.pos_x, g_texto_actual.pos_y, g_texto_actual.color,
                               g_texto_actual.color_fondo, g_texto_actual.fuente, tramo);
    g_texto_actual.pos_x += ancho_tramo;
  }
}

/**
 * @brief   lcd_lvgl_borrar() sin tomar el cerrojo.
 * @ingroup LCD_LVGL
 * @private
 */
static void lcd_lvgl_borrar_sin_cerrojo(uint16_t color) {
  lv_obj_t *pantalla = lv_screen_active();

  lv_obj_clean(pantalla);  // Destruye TODOS sus hijos: el lienzo y lo que contenga, y widgets.
  lcd_lvgl_olvidar_etiquetas();
  lv_obj_set_style_bg_color(pantalla, lcd_lvgl_color(color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(pantalla, LV_OPA_COVER, LV_PART_MAIN);
  (void) lcd_lvgl_obtener_lienzo();  // Lienzo nuevo y vacío.
}

// ===== LCD_LVGL - Funciones Publicas =====
void lcd_lvgl_inicializar(void) {
  /* Pantalla, táctil y LVGL (reloj, display e indev). Después de esto ya existe una pantalla
   * activa vacía, propiedad de LVGL. */
  lcd_lvgl_port_inicializar();

  lv_lock();
  if (!g_inicializado) {
    lcd_lvgl_borrar_sin_cerrojo(NEGRO);
    g_inicializado = TRUE;
  }
  lv_unlock();
}

uint32_t lcd_lvgl_procesar(void) {
  uint32_t espera;

  /* lv_timer_handler() es el "motor" de LVGL: ejecuta sus temporizadores, entre ellos el de
   * refresco, que repinta en los búferes las zonas sucias y las vuelca con lcd_panel, y el de
   * lectura del táctil. Al terminar se devuelve el bus de la pantalla (ver lcd_lvgl_port.c).
   * Todo ello con lv_lock() tomado, para que ninguna otra tarea toque LVGL entretanto. */
  lv_lock();
  espera = lv_timer_handler();
  lcd_lvgl_port_soltar_bus();
  lv_unlock();
  return espera;
}

void lcd_lvgl_refrescar_ya(void) {
  /* lv_refr_now() hace sólo la parte de repintado de lv_timer_handler(), en el acto. */
  lv_lock();
  lv_refr_now(NULL);
  lcd_lvgl_port_soltar_bus();
  lv_unlock();
}

void lcd_lvgl_borrar(uint16_t color) {
  /* Modo inmediato: glcd rellenaba toda la memoria de imagen con un color.
   * Modo retenido: se destruyen todos los objetos de la pantalla activa (LVGL libera su
   * memoria) y se cambia el color de fondo de la propia pantalla, que es un objeto más. La
   * pantalla entera queda sucia y se repinta en el siguiente lcd_lvgl_procesar(). */
  lv_lock();
  lcd_lvgl_borrar_sin_cerrojo(color);
  lv_unlock();
}

int32_t lcd_lvgl_printf(const char *ptr_formato, ...) {
  char    texto[LCD_LVGL_MAX_TEXTO + 1u];
  int32_t resultado;
  va_list argumentos;

  /* Primero se forma el texto en un búfer local, como haría printf (no hace falta el cerrojo:
   * todavía no se toca LVGL). */
  va_start(argumentos, ptr_formato);
  resultado = (int32_t) vsnprintf(texto, sizeof(texto), ptr_formato, argumentos);
  va_end(argumentos);

  /* Modo retenido: cada tramo de texto se convierte en una etiqueta (lv_label), hija del
   * lienzo, en la posición del cursor; si ya había una etiqueta de esta librería en esa
   * posición, se le cambia el texto en lugar de crear otra. LVGL copia el texto, así que el
   * búfer local puede desaparecer al volver. */
  if (resultado >= 0) {
    lv_lock();
    lcd_lvgl_imprimir_sin_cerrojo(texto);
    lv_unlock();
  }
  return resultado;
}

int32_t lcd_lvgl_xprintf(uint16_t x, uint16_t y, uint16_t color, uint16_t color_fondo,
                         uint32_t fuente, const char *ptr_formato, ...) {
  char                    texto[LCD_LVGL_MAX_TEXTO + 1u];
  lcd_lvgl_config_texto_t anterior;
  int32_t                 resultado;
  va_list                 argumentos;

  if (fuente >= NUMERO_FUENTES) {
    return -1;
  }

  va_start(argumentos, ptr_formato);
  resultado = (int32_t) vsnprintf(texto, sizeof(texto), ptr_formato, argumentos);
  va_end(argumentos);

  /* Igual que lcd_lvgl_printf(), pero con posición, colores y fuente propios: se guarda la
   * configuración actual, se usa la indicada y se restaura, como en glcd_xprintf(). Escribir
   * varias veces en la misma (x, y) actualiza siempre la misma etiqueta: es la forma natural de
   * mostrar un valor que cambia. */
  if (resultado >= 0) {
    lv_lock();
    anterior                   = g_texto_actual;
    g_texto_actual.pos_x       = (int32_t) x;
    g_texto_actual.pos_y       = (int32_t) y;
    g_texto_actual.color       = color;
    g_texto_actual.color_fondo = color_fondo;
    g_texto_actual.fuente      = fuente;
    lcd_lvgl_imprimir_sin_cerrojo(texto);
    g_texto_actual = anterior;
    lv_unlock();
  }
  return resultado;
}

void lcd_lvgl_caracter(char c, uint16_t x, uint16_t y, uint16_t color, uint16_t color_fondo,
                       uint32_t fuente) {
  char cadena[2];

  if ((x >= LCD_PANEL_ANCHO) || (y >= LCD_PANEL_ALTO) || (fuente >= NUMERO_FUENTES) || (c < ' ') ||
      (c > '~')) {
    return;
  }

  /* Modo retenido: una etiqueta con un único carácter (o la que ya hubiera en esa posición). */
  cadena[0] = c;
  cadena[1] = '\0';
  lv_lock();
  lcd_lvgl_escribir_etiqueta((int32_t) x, (int32_t) y, color, color_fondo, fuente, cadena);
  lv_unlock();
}

void lcd_lvgl_texto(uint16_t x, uint16_t y, uint16_t color, uint16_t color_fondo, uint32_t fuente,
                    const char *ptr_cadena) {
  if ((ptr_cadena == NULL) || (x >= LCD_PANEL_ANCHO) || (y >= LCD_PANEL_ALTO) ||
      (fuente >= NUMERO_FUENTES)) {
    return;
  }

  /* Modo retenido: toda la cadena en una sola etiqueta en (x, y). */
  lv_lock();
  lcd_lvgl_escribir_etiqueta((int32_t) x, (int32_t) y, color, color_fondo, fuente, ptr_cadena);
  lv_unlock();
}

void lcd_lvgl_color_texto(uint16_t color) {
  /* Sólo cambia la configuración que usará el próximo lcd_lvgl_printf(): no crea objetos. */
  lv_lock();
  g_texto_actual.color = color;
  lv_unlock();
}

void lcd_lvgl_fondo_texto(uint16_t color_fondo) {
  lv_lock();
  g_texto_actual.color_fondo = color_fondo;
  lv_unlock();
}

void lcd_lvgl_xy_texto(uint16_t x, uint16_t y) {
  if ((x >= LCD_PANEL_ANCHO) || (y >= LCD_PANEL_ALTO)) {
    return;
  }
  lv_lock();
  g_texto_actual.pos_x = (int32_t) x;
  g_texto_actual.pos_y = (int32_t) y;
  lv_unlock();
}

void lcd_lvgl_desplazar(uint16_t lineas) {
  /* Modo retenido: se mueven hacia arriba los objetos del lienzo y se destruyen los que salen
   * del todo por arriba. La zona que queda libre abajo muestra el fondo de la pantalla. */
  lv_lock();
  lcd_lvgl_desplazar_sin_cerrojo((int32_t) lineas);
  lv_unlock();
}

void lcd_lvgl_activar_desplazamiento(bool_t activo) {
  lv_lock();
  g_texto_actual.desplazamiento_activado = activo;
  lv_unlock();
}

void lcd_lvgl_seleccionar_fuente(uint32_t fuente) {
  if (fuente >= NUMERO_FUENTES) {
    return;
  }
  lv_lock();
  g_texto_actual.fuente = fuente;
  lv_unlock();
}

void lcd_lvgl_punto(uint16_t x, uint16_t y, uint16_t color) {
  lv_obj_t *punto;

  if ((x >= LCD_PANEL_ANCHO) || (y >= LCD_PANEL_ALTO)) {
    return;
  }

  /* Modo retenido: un objeto de 1 x 1 píxel con fondo opaco. ¡Un objeto entero para un solo
   * píxel! Por eso esta función es cara en memoria y en tiempo de repintado. */
  lv_lock();
  punto = lcd_lvgl_crear_figura((int32_t) x, (int32_t) y, 1, 1);
  lv_obj_set_style_bg_color(punto, lcd_lvgl_color(color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(punto, LV_OPA_COVER, LV_PART_MAIN);
  lv_unlock();
}

void lcd_lvgl_linea(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
  lv_obj_t           *linea;
  lv_point_precise_t *puntos;
  int32_t             x_min = (x0 < x1) ? (int32_t) x0 : (int32_t) x1;
  int32_t             y_min = (y0 < y1) ? (int32_t) y0 : (int32_t) y1;

  /* Modo retenido: un objeto lv_line colocado en la esquina superior izquierda del segmento,
   * con sus dos extremos expresados RELATIVOS a esa esquina (así, al desplazar la pantalla,
   * basta con mover el objeto). lv_line guarda el PUNTERO a los puntos, no una copia: deben
   * vivir tanto como la línea. Se reservan con lv_malloc() y se liberan en su evento
   * LV_EVENT_DELETE (lcd_lvgl_liberar_puntos()). */
  lv_lock();
  puntos = lv_malloc(2u * sizeof(lv_point_precise_t));
  if (puntos != NULL) {
    puntos[0].x = (int32_t) x0 - x_min;
    puntos[0].y = (int32_t) y0 - y_min;
    puntos[1].x = (int32_t) x1 - x_min;
    puntos[1].y = (int32_t) y1 - y_min;

    linea = lv_line_create(lcd_lvgl_obtener_lienzo());
    lv_obj_remove_style_all(linea);
    lv_obj_set_size(linea, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_pos(linea, x_min, y_min);
    lv_obj_remove_flag(linea, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_line_width(linea, 1, LV_PART_MAIN);
    lv_obj_set_style_line_color(linea, lcd_lvgl_color(color), LV_PART_MAIN);
    lv_obj_set_style_line_opa(linea, LV_OPA_COVER, LV_PART_MAIN);
    lv_line_set_points(linea, puntos, 2);
    lv_obj_add_event_cb(linea, lcd_lvgl_liberar_puntos, LV_EVENT_DELETE, puntos);
  }
  lv_unlock();
}

void lcd_lvgl_rectangulo(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
  lv_obj_t *rectangulo;
  int32_t   x_min = (x0 < x1) ? (int32_t) x0 : (int32_t) x1;
  int32_t   y_min = (y0 < y1) ? (int32_t) y0 : (int32_t) y1;
  int32_t   ancho = ((x0 < x1) ? (int32_t) (x1 - x0) : (int32_t) (x0 - x1)) + 1;
  int32_t   alto  = ((y0 < y1) ? (int32_t) (y1 - y0) : (int32_t) (y0 - y1)) + 1;

  /* Modo retenido: un lv_obj del tamaño del rectángulo, con fondo transparente y un borde de
   * 1 píxel del color pedido. Los vértices indicados están incluidos (de ahí el +1). */
  lv_lock();
  rectangulo = lcd_lvgl_crear_figura(x_min, y_min, ancho, alto);
  lv_obj_set_style_border_width(rectangulo, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(rectangulo, lcd_lvgl_color(color), LV_PART_MAIN);
  lv_obj_set_style_border_opa(rectangulo, LV_OPA_COVER, LV_PART_MAIN);
  lv_unlock();
}

void lcd_lvgl_rectangulo_relleno(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                                 uint16_t color) {
  lv_obj_t *rectangulo;
  int32_t   x_min = (x0 < x1) ? (int32_t) x0 : (int32_t) x1;
  int32_t   y_min = (y0 < y1) ? (int32_t) y0 : (int32_t) y1;
  int32_t   ancho = ((x0 < x1) ? (int32_t) (x1 - x0) : (int32_t) (x0 - x1)) + 1;
  int32_t   alto  = ((y0 < y1) ? (int32_t) (y1 - y0) : (int32_t) (y0 - y1)) + 1;

  /* Modo retenido: un lv_obj del tamaño del rectángulo con el fondo opaco del color pedido. */
  lv_lock();
  rectangulo = lcd_lvgl_crear_figura(x_min, y_min, ancho, alto);
  lv_obj_set_style_bg_color(rectangulo, lcd_lvgl_color(color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(rectangulo, LV_OPA_COVER, LV_PART_MAIN);
  lv_unlock();
}

void lcd_lvgl_circunferencia(uint16_t xc, uint16_t yc, uint16_t radio, uint16_t color) {
  lv_obj_t *circunferencia;
  int32_t   lado = 2 * (int32_t) radio + 1;

  /* Modo retenido: un lv_obj cuadrado que envuelve la circunferencia, con radio de esquina
   * LV_RADIUS_CIRCLE (esquinas tan redondeadas que el cuadrado se vuelve círculo), fondo
   * transparente y borde de 1 píxel. */
  lv_lock();
  circunferencia = lcd_lvgl_crear_figura((int32_t) xc - (int32_t) radio,
                                         (int32_t) yc - (int32_t) radio, lado, lado);
  lv_obj_set_style_radius(circunferencia, LV_RADIUS_CIRCLE, LV_PART_MAIN);
  lv_obj_set_style_border_width(circunferencia, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(circunferencia, lcd_lvgl_color(color), LV_PART_MAIN);
  lv_obj_set_style_border_opa(circunferencia, LV_OPA_COVER, LV_PART_MAIN);
  lv_unlock();
}

void lcd_lvgl_circulo(uint16_t xc, uint16_t yc, uint16_t radio, uint16_t color) {
  lv_obj_t *circulo;
  int32_t   lado = 2 * (int32_t) radio + 1;

  /* Modo retenido: como la circunferencia, pero con el fondo opaco en lugar del borde. */
  lv_lock();
  circulo = lcd_lvgl_crear_figura((int32_t) xc - (int32_t) radio, (int32_t) yc - (int32_t) radio,
                                  lado, lado);
  lv_obj_set_style_radius(circulo, LV_RADIUS_CIRCLE, LV_PART_MAIN);
  lv_obj_set_style_bg_color(circulo, lcd_lvgl_color(color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(circulo, LV_OPA_COVER, LV_PART_MAIN);
  lv_unlock();
}
