/**
 * @file    error.c
 * @brief   Librería para el tratamiento de errores: detiene el programa mostrando dónde falló.
 *
 * @details Implementa `parar_con_error()`, la función a la que llaman las macros `ERROR()` y
 * `ASSERT()` de `error.h`. Si el proyecto usa un módulo de pantalla, escribe en la LCD-PAR-S035
 * el mensaje, el fichero, la función y la línea. Después deja el programa encerrado en un bucle
 * infinito: en un sistema embebido no hay sistema operativo al que volver.
 *
 * La pantalla que se usa la eligen dos opciones del Kconfig del módulo, que llegan aquí como
 * macros a través de mcux_config.h (antes se elegía con `EA_GLCD` en `error.h`):
 *  - `ERROR_EN_LCD`: se activa sola con el módulo lcd. Escribe con `lcd_*`, directamente en la
 *    memoria de imagen del ST7796S.
 *  - `ERROR_EN_LCD_LVGL`: se activa sola con el módulo lcd_lvgl si no está lcd. Escribe con
 *    `lcd_lvgl_*` y fuerza el repintado con `lcd_lvgl_refrescar_ya()`, porque después ya no se
 *    va a llamar a `lcd_lvgl_procesar()`.
 *  - Ninguna: el programa se detiene sin mensaje.
 *
 * Lo más cómodo durante el desarrollo es poner un **punto de ruptura** en esta función: cuando
 * salte, la pila de llamadas del depurador dice exactamente quién llamó mal a quién.
 *
 * @warning Para que el mensaje se vea, la pantalla tiene que estar inicializada
 * (`lcd_inicializar()` o `lcd_lvgl_inicializar()`). Si no lo está, el programa se detiene igual,
 * pero sin mensaje. Con FreeRTOS y lcd_lvgl, si otra tarea tiene tomado LVGL (`lv_lock()`), el
 * mensaje no llega a pintarse.
 *
 * @author      Alejandro Lara Doña - alejandro.lara@uca.es | Eduardo Romero
 * @date        2014/2026
 * @version     3.0
 *
 * @copyright   GNU General Public License version 3 or later
 *
 * @note    Portado al MCX A156 en 2026: sólo cambia la pantalla (`glcd_*` → `lcd_*` o
 *          `lcd_lvgl_*`). En el LPC4088 esta función venía dentro de la biblioteca base cerrada;
 *          ahora se entrega en código fuente, como módulo (ver modulos/README.md).
 */

#include "error.h"
#include "fsl_device_registers.h"
#include "tipos.h"
#if defined(ERROR_EN_LCD) && ERROR_EN_LCD
#include "lcd.h"
#elif defined(ERROR_EN_LCD_LVGL) && ERROR_EN_LCD_LVGL
#include "lcd_lvgl.h"
#endif

// ===== Error - Funciones Publicas =====
/**
 * @brief   Detiene el programa mostrando en la pantalla la ubicación del error y un mensaje.
 * @ingroup ERROR
 *
 * @param[in]   fichero   Nombre del fichero fuente (lo da `__FILE__`).
 * @param[in]   funcion   Nombre de la función (lo da `__FUNCTION__`).
 * @param[in]   linea     Número de línea (lo da `__LINE__`).
 * @param[in]   mensaje   Mensaje breve y descriptivo del error.
 *
 * @note    No retorna nunca.
 */
void parar_con_error(const char *fichero, const char *funcion,
                     const uint32_t linea, const char *mensaje) {
#if defined(ERROR_EN_LCD) && ERROR_EN_LCD
  lcd_borrar(AZUL_OSCURO);
  lcd_xprintf(0, 0, BLANCO, AZUL_OSCURO, FUENTE12X24, "ERROR");
  lcd_xprintf(0, 48, AMARILLO, AZUL_OSCURO, FUENTE8X16, "%s", mensaje);
  lcd_xprintf(0, 80, BLANCO, AZUL_OSCURO, FUENTE8X16, "Fichero: %s", fichero);
  lcd_xprintf(0, 112, BLANCO, AZUL_OSCURO, FUENTE8X16, "Funcion: %s", funcion);
  lcd_xprintf(0, 144, BLANCO, AZUL_OSCURO, FUENTE8X16, "Linea:   %u", (unsigned) linea);
#elif defined(ERROR_EN_LCD_LVGL) && ERROR_EN_LCD_LVGL
  lcd_lvgl_borrar(AZUL_OSCURO);
  lcd_lvgl_xprintf(0, 0, BLANCO, AZUL_OSCURO, FUENTE12X24, "ERROR");
  lcd_lvgl_xprintf(0, 48, AMARILLO, AZUL_OSCURO, FUENTE8X16, "%s", mensaje);
  lcd_lvgl_xprintf(0, 80, BLANCO, AZUL_OSCURO, FUENTE8X16, "Fichero: %s", fichero);
  lcd_lvgl_xprintf(0, 112, BLANCO, AZUL_OSCURO, FUENTE8X16, "Funcion: %s", funcion);
  lcd_lvgl_xprintf(0, 144, BLANCO, AZUL_OSCURO, FUENTE8X16, "Linea:   %u", (unsigned) linea);
  lcd_lvgl_refrescar_ya();
#else
  (void) fichero;
  (void) funcion;
  (void) linea;
  (void) mensaje;
#endif

  while (TRUE) {
  }
}
