/**
 * @file    ejemplo_lcd.c
 * @brief   Ejemplo de la biblioteca lcd_: texto con las tres fuentes, colores, figuras, borrado y
 * un contador que se actualiza en el mismo sitio.
 *
 * @details Proyecto: copiarlo como main.c de un proyecto con la carpeta modulos/ y activar en
 * prj.conf CONFIG_MCUX_COMPONENT_LCD=y (ver modulos/lcd/README.md). No usa LVGL ni
 * FreeRTOS.
 * Probado en la placa con frdmmcxa156_flexio_mculcd_edma_transfer.
 *
 * @author      Alejandro Lara Doña - alejandro.lara@uca.es
 * @date        2026
 * @version     1.0
 *
 * @copyright   GNU General Public License version 3 or later
 */

#include "fsl_common.h"   // SDK_DelayAtLeastUs, SystemCoreClock
#include "board.h"
#include "tipos.h"
#include "lcd.h"

// ===== Ejemplo_LCD - Prototipos =====
/**
 * @brief   Inicializa pines, relojes y consola de depuración. Está en el hardware_init.c del
 * proyecto, y su prototipo, en app.h.
 */
void BOARD_InitHardware(void);

// ===== Ejemplo_LCD - Funciones Privadas =====
/**
 * @brief   Espera activa de unos milisegundos (todavía no hemos visto los temporizadores).
 */
static void esperar_ms(uint32_t ms) {
  SDK_DelayAtLeastUs(ms * 1000u, SystemCoreClock);
}

/**
 * @brief   Primera pantalla: texto con las tres fuentes, con lcd_xprintf y con lcd_printf.
 */
static void demo_texto(void) {

  lcd_borrar(NEGRO);

  lcd_xprintf(0, 0, AMARILLO, NEGRO, FUENTE16X32, "lcd_ en la FRDM-MCXA156");
  lcd_xprintf(0, 40, BLANCO, AZUL_OSCURO, FUENTE12X24, "Fuente de 12x24 píxeles");
  lcd_xprintf(0, 70, VERDE, NEGRO, FUENTE8X16, "Fuente de 8x16: ¡acentos, eñes y ¿signos?, 25 ºC");

  /* lcd_printf escribe donde se quedó, con los colores y la fuente seleccionados, e interpreta
   * los caracteres de control.
   */
  lcd_seleccionar_fuente(FUENTE8X16);
  lcd_color_texto(CIAN);
  lcd_fondo_texto(NEGRO);
  lcd_xy_texto(0, 100);
  lcd_printf("Linea 1 con lcd_printf\n");
  lcd_printf("Linea 2: %d + %d = %d\n", 2, 3, 2 + 3);
  lcd_printf("Tab:\t|\t|\t|\n");

  lcd_texto(0, 170, NEGRO, BLANCO, FUENTE12X24, "lcd_texto no interpreta \\n");
}

/**
 * @brief   Segunda pantalla: puntos, líneas, rectángulos y círculos, algunos recortados por los
 * bordes de la pantalla.
 */
static void demo_figuras(void) {

  uint16_t i;

  lcd_borrar(GRIS_OSCURO);

  lcd_rectangulo(0, 0, LCD_X_MAXIMO, LCD_Y_MAXIMO, BLANCO);
  lcd_rectangulo_relleno(20, 20, 140, 100, ROJO);
  lcd_rectangulo(160, 20, 280, 100, AMARILLO);
  lcd_circulo(360, 60, 40, VERDE);
  lcd_circunferencia(360, 60, 50, CIAN);

  for (i = 0; i < 12; i++) {
    lcd_linea(20, 300, (uint16_t)(20 + i * 20), 130, (uint16_t)(i % 2 ? NARANJA : MAGENTA));
  }

  for (i = 0; i < 100; i++) {
    lcd_punto((uint16_t)(300 + i), (uint16_t)(200 + (i * i) / 100), BLANCO);
  }

  lcd_circulo(470, 310, 30, AZUL);  // Se sale por la derecha y por abajo: se recorta
}

// ===== Ejemplo_LCD - Programa Principal =====
/**
 * @brief   Alterna las dos pantallas de demostración, con un contador en la esquina inferior.
 */
int main(void) {

  uint32_t contador = 0;
  uint32_t i;

  BOARD_InitHardware();
  lcd_inicializar();

  while (1) {
    demo_texto();

    /* Un número que cambia se escribe siempre en el mismo sitio y con color de fondo, para que
     * tape al anterior: la biblioteca no recuerda lo que hay en la pantalla.
     */
    for (i = 0; i < 30; i++) {
      lcd_xprintf(0, 280, BLANCO, ROJO, FUENTE16X32, "Contador: %5u", (unsigned) contador++);
      esperar_ms(100);
    }

    demo_figuras();
    esperar_ms(3000);
  }
}
