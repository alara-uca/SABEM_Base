/**
 * @file    ejemplo_lcd_lvgl.c
 * @brief   Ejemplo sin RTOS de lcd_lvgl_*: texto y figuras "a la glcd" mezclados con un widget
 * nativo de LVGL (un deslizador que se mueve con el táctil).
 *
 * @details Qué muestra:
 *  - El modo inmediato EMULADO: lcd_lvgl_xprintf(), lcd_lvgl_printf(), rectángulos, círculos y
 *    líneas. Cada llamada crea (o actualiza) un objeto de LVGL.
 *  - Un widget de LVGL (lv_slider) creado con la API nativa de LVGL, entre lv_lock() y
 *    lv_unlock(), en la misma pantalla. Al moverlo, su evento escribe el valor con
 *    lcd_lvgl_xprintf() en la misma posición, así que se actualiza SIEMPRE LA MISMA etiqueta.
 *  - El bucle principal: sin lcd_lvgl_procesar() no se pinta nada ni se lee el táctil.
 *
 * Proyecto: copiarlo como main.c de un proyecto con la carpeta modulos/ y activar en prj.conf
 * CONFIG_MCUX_COMPONENT_LCD_LVGL=y (ver modulos/lcd/README.md). Sin RTOS, LVGL dibuja en la
 * pila de main(): el módulo la amplía a 0x1800 bytes (LCD_LVGL_PILA_PRINCIPAL); con los 0x800
 * por defecto se desborda y la pantalla se queda en negro. Probado en la placa con
 * frdmmcxa156_flexio_mculcd_edma_transfer, táctil incluido.
 *
 * @author    Alejandro Lara Doña - alejandro.lara(at)uca.es
 * @date      2026
 * @version   1.0
 *
 * @copyright GNU General Public License version 3 or later
 */

#include "board.h"
#include "tipos.h"
#include "lvgl.h"
#include "lcd_lvgl.h"

// ===== EJEMPLO - Macros Privadas =====
#define EJEMPLO_PERIODO_CONTADOR_MS  500u  //!< Cada cuánto se actualiza el contador.

// ===== EJEMPLO - Prototipos Externos =====
/** Inicialización de relojes y pines de la placa: hardware_init.c del proyecto,
 * declarada en su app.h. */
void BOARD_InitHardware(void);

// ===== EJEMPLO - Funciones Privadas =====
/**
 * @brief   Evento LV_EVENT_VALUE_CHANGED del deslizador: muestra su valor.
 *
 * @details LVGL llama a esta función desde lcd_lvgl_procesar(), cuando el táctil mueve el
 * deslizador. Se puede usar lcd_lvgl_* desde aquí: el cerrojo de LVGL admite tomarse otra vez.
 */
static void ejemplo_al_mover_deslizador(lv_event_t *evento) {
  lv_obj_t *deslizador = lv_event_get_target_obj(evento);

  lcd_lvgl_xprintf(90, 170, AMARILLO, NEGRO, FUENTE12X24, "Deslizador: %3d %%",
                   (int) lv_slider_get_value(deslizador));
}

// ===== EJEMPLO - Programa Principal =====
int main(void) {
  lv_obj_t *deslizador;
  uint32_t instante;
  uint32_t contador = 0;

  BOARD_InitHardware();
  lcd_lvgl_inicializar();  // Pantalla, táctil y LVGL. Deja la pantalla en negro.

  /* Modo inmediato emulado: cada línea crea un objeto LVGL. Nada se ve todavía: se pintará en
   * el primer lcd_lvgl_procesar() del bucle. */
  lcd_lvgl_rectangulo(4, 4, 475, 51, CIAN);
  lcd_lvgl_xprintf(16, 12, BLANCO, NEGRO, FUENTE16X32, "SABEM - LCD con LVGL");
  lcd_lvgl_circulo(440, 90, 16, ROJO);
  lcd_lvgl_circunferencia(440, 90, 22, BLANCO);
  lcd_lvgl_linea(10, 150, 400, 150, GRIS_CLARO);

  lcd_lvgl_seleccionar_fuente(FUENTE8X16);
  lcd_lvgl_color_texto(VERDE);
  lcd_lvgl_xy_texto(10, 64);
  lcd_lvgl_printf("Modo inmediato emulado: un objeto LVGL por llamada.\n");
  lcd_lvgl_printf("Mueve el deslizador con el dedo.\n");

  /* Widget nativo de LVGL, en la misma pantalla. La API de LVGL no toma el cerrojo por sí
   * misma: se toma aquí (sin FreeRTOS no hace nada, pero así el código vale para los dos casos).
   */
  lv_lock();
  deslizador = lv_slider_create(lv_screen_active());
  lv_obj_set_width(deslizador, 360);
  lv_obj_align(deslizador, LV_ALIGN_BOTTOM_MID, 0, -40);
  lv_slider_set_range(deslizador, 0, 100);
  lv_slider_set_value(deslizador, 50, LV_ANIM_OFF);
  lv_obj_add_event_cb(deslizador, ejemplo_al_mover_deslizador, LV_EVENT_VALUE_CHANGED, NULL);
  lv_unlock();
  lcd_lvgl_xprintf(90, 170, AMARILLO, NEGRO, FUENTE12X24, "Deslizador: %3d %%", 50);

  instante = lv_tick_get();
  while (1) {
    /* Motor de LVGL: repinta lo que ha cambiado, lee el táctil y llama a los eventos. */
    (void) lcd_lvgl_procesar();

    /* Tarea periódica sin RTOS, con el reloj de LVGL: cada 500 ms se reescribe el contador en
     * la misma posición, así que se reutiliza la misma etiqueta (no se acumulan objetos). */
    if (lv_tick_elaps(instante) >= EJEMPLO_PERIODO_CONTADOR_MS) {
      instante = lv_tick_get();
      contador++;
      lcd_lvgl_xprintf(10, 110, BLANCO, AZUL_OSCURO, FUENTE12X24, "Contador: %lu",
                       (unsigned long) contador);
    }
  }
}
