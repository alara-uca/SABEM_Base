/**
 * @file    ejemplo_tactil.c
 * @brief   Ejemplo de lcd_tactil_ con lcd_: dibuja un punto donde se toca la pantalla, muestra
 * las coordenadas y borra el dibujo al tocar el botón "Borrar".
 *
 * @details Proyecto: copiarlo como main.c de un proyecto con la carpeta modulos/ y activar en
 * prj.conf CONFIG_MCUX_COMPONENT_LCD=y y CONFIG_MCUX_COMPONENT_LCD_TACTIL=y (ver
 * modulos/lcd/README.md). No usa LVGL ni FreeRTOS. Probado en la placa con
 * frdmmcxa156_flexio_mculcd_edma_transfer.
 *
 * @author    Alejandro Lara Doña - alejandro.lara(at)uca.es
 * @date      2026
 * @version   1.0
 *
 * @copyright GNU General Public License version 3 or later
 */

#include "board.h"
#include "lcd.h"
#include "lcd_tactil.h"
#include "tipos.h"

// ===== Ejemplo_Tactil - Constantes =====
/**
 * @brief   Geometría de la pantalla del ejemplo.
 */
enum ejemplo_tactil_geometria {
  kEjemploTactilBarraAlto = 40,   //!< Alto de la barra superior (coordenadas y botón).
  kEjemploTactilBotonX0   = 380,  //!< Esquina superior izquierda del botón "Borrar".
  kEjemploTactilBotonX1   = 479,  //!< Esquina inferior derecha del botón.
  kEjemploTactilBotonY1   = 39,
  kEjemploTactilRadio     = 3  //!< Radio del punto que se dibuja.
};

// ===== Ejemplo_Tactil - Prototipos =====
/**
 * @brief   Inicializa pines, relojes y consola de depuración. Está en el hardware_init.c del
 * proyecto, y su prototipo, en app.h.
 */
void BOARD_InitHardware(void);

// ===== Ejemplo_Tactil - Funciones Privadas =====
/**
 * @brief   Dibuja la barra superior con el botón "Borrar" y deja el lienzo en negro.
 */
static void dibujar_pantalla(void) {

  lcd_borrar(NEGRO);
  lcd_rectangulo_relleno(0, 0, kLCDXMaximo, kEjemploTactilBarraAlto - 1, AZUL_OSCURO);
  lcd_rectangulo_relleno(kEjemploTactilBotonX0, 0, kEjemploTactilBotonX1, kEjemploTactilBotonY1,
                         ROJO);
  lcd_xprintf(kEjemploTactilBotonX0 + 14, 8, BLANCO, ROJO, FUENTE12X24, "Borrar");
}

// ===== Ejemplo_Tactil - Programa Principal =====
/**
 * @brief   Bucle: lee el táctil y, si hay un dedo, o borra (si está sobre el botón) o dibuja un
 * punto en esa posición.
 */
int main(void) {

  uint16_t x, y;

  BOARD_InitHardware();
  lcd_inicializar();

  if (!lcd_tactil_inicializar()) {
    lcd_xprintf(0, 0, ROJO, NEGRO, FUENTE12X24, "El GT911 no responde");
    while (1) {
      ;
    }
  }

  dibujar_pantalla();

  while (1) {
    if (lcd_tactil_leer(&x, &y)) {
      lcd_xprintf(4, 8, BLANCO, AZUL_OSCURO, FUENTE12X24, "x = %3u  y = %3u", (unsigned) x,
                  (unsigned) y);

      if (x >= kEjemploTactilBotonX0 && y <= kEjemploTactilBotonY1) {
        dibujar_pantalla();
        /* Espera a que se levante el dedo, para no borrar veinte veces seguidas. */
        while (lcd_tactil_pulsado()) {
          ;
        }
      } else if (y >= kEjemploTactilBarraAlto + kEjemploTactilRadio) {
        lcd_circulo(x, y, kEjemploTactilRadio, AMARILLO);
      }
    }
  }
}
