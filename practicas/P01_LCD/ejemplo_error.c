/**
 * @file    ejemplo_error.c
 * @brief   Ejemplo del módulo error: ASSERT() comprueba los parámetros de una función y, cuando
 * uno es incorrecto, detiene el programa mostrando en la pantalla dónde ha fallado.
 *
 * @details Una cuenta atrás de 5 a 0 divide 100 entre el valor actual. Cuando el divisor llega a
 * 0, el ASSERT() de dividir() llama a parar_con_error(), que muestra en la pantalla el mensaje,
 * el fichero, la función (dividir) y la línea del ASSERT(), y deja el programa parado.
 *
 * Proyecto: copiarlo como main.c de un proyecto con la carpeta modulos/ y activar en prj.conf:
 *     CONFIG_MCUX_COMPONENT_LCD=y
 *     CONFIG_MCUX_COMPONENT_ERROR=y
 * Con esos dos módulos, error escribe con lcd_ (opción ERROR_EN_LCD, que se activa sola). Con
 * lcd_lvgl en lugar de lcd se usaría lcd_lvgl_ (ERROR_EN_LCD_LVGL); sin módulo de pantalla, el
 * programa se para igual, pero sin mensaje. Ver modulos/README.md. Probado en la placa con
 * frdmmcxa156_flexio_mculcd_edma_transfer.
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
#include "error.h"

// ===== Ejemplo_Error - Prototipos =====
/**
 * @brief   Inicializa pines, relojes y consola de depuración. Está en el hardware_init.c del
 * proyecto, y su prototipo, en app.h.
 */
void BOARD_InitHardware(void);

// ===== Ejemplo_Error - Funciones Privadas =====
/**
 * @brief   Espera activa de unos milisegundos.
 */
static void esperar_ms(uint32_t ms) {
  SDK_DelayAtLeastUs(ms * 1000u, SystemCoreClock);
}

/**
 * @brief   División entera que comprueba su parámetro con ASSERT().
 *
 * @param[in]   dividendo   Número que se divide.
 * @param[in]   divisor     Número entre el que se divide. No puede ser 0.
 * @return  Cociente de la división.
 */
static int32_t dividir(int32_t dividendo, int32_t divisor) {

  ASSERT(divisor != 0, "dividir: el divisor no puede ser 0");

  return dividendo / divisor;
}

// ===== Ejemplo_Error - Programa Principal =====
/**
 * @brief   Cuenta atrás que acaba dividiendo entre 0 para provocar el error.
 */
int main(void) {

  int32_t divisor;

  BOARD_InitHardware();
  lcd_inicializar();   // Sin pantalla inicializada, el error no se vería (ver error.c)

  lcd_borrar(NEGRO);
  lcd_xprintf(0, 0, AMARILLO, NEGRO, FUENTE16X32, "Ejemplo del modulo error");

  for (divisor = 5; divisor >= 0; divisor--) {
    lcd_xprintf(0, 60 + (uint16_t) (5 - divisor) * 30, BLANCO, NEGRO, FUENTE12X24,
                "100 / %d = ...", (int) divisor);
    esperar_ms(1000);
    lcd_xprintf(0, 60 + (uint16_t) (5 - divisor) * 30, BLANCO, NEGRO, FUENTE12X24,
                "100 / %d = %d   ", (int) divisor, (int) dividir(100, divisor));
  }

  while (TRUE) {
    // No se llega aquí: con divisor = 0, el ASSERT() de dividir() para el programa.
  }
}
