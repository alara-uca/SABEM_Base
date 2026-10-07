/**
 * @file    error.h
 * @brief   Librería para el tratamiento de errores.
 *
 * @author      Alejandro Lara Doña - alejandro.lara@uca.es | Eduardo Romero
 * @date        2014/2025
 * @version     2.0
 *
 * @copyright   GNU General Public License version 3 or later
 */

#ifndef ERROR_H
#define ERROR_H

#include "tipos.h"

/* Si el mensaje se muestra en la pantalla ya no se decide aquí (antes, con EA_GLCD): lo deciden
 * las opciones ERROR_EN_LCD y ERROR_EN_LCD_LVGL del Kconfig del módulo, que se activan solas
 * cuando el proyecto usa el módulo lcd o el módulo lcd_lvgl. Ver error.c.
 */

/**
 * @brief   Macro para habilitar o deshabilitar la función ASSERT.
 * @ingroup ERROR
 */
#define HABILITAR_ASSERT 1

// ===== Error - Funciones Macro Publicas =====
/**
 * @brief     Función para generar un mensaje de error.
 * @ingroup   ERROR
 *
 * @details   Muestra un mensaje de error indicando el archivo, función y linea donde se llamó
 * a la función junto con un mensaje.
 *
 * @param[in]   mensaje   Mensaje breve y descriptivo del "error".
 */
#define ERROR(mensaje)  parar_con_error(__FILE__, __FUNCTION__, __LINE__, mensaje)

#if HABILITAR_ASSERT != 0
/**
 * @brief     Función de comprobación.
 * @ingroup   ERROR
 *
 * @details     Verifica si la condición `expr` es falsa y lanza un error indicando el archivo,
 * función y linea donde se llamó a la función junto con un mensaje. Principalmente se utiliza
 * para verificar los parámetros de entrada de las funciones para comprobar que se encuentran
 * en los rangos correctos o se corresponden con los tipos adecuados (por ejemplo, punteros a
 * periféricos).
 *
 * @param[in]   expr      Expresión booleana cuyo resultado sea Verdadero si los parámetros
 *                        comprobados son correctos.
 * @param[in]   mensaje   Mensaje breve y descriptivo de qué ha causado el "error".
 */
#define ASSERT(expr, mensaje)               \
do {                                        \
  if (expr) {                               \
    ;                                       \
  } else {                                  \
    parar_con_error(__FILE__, __FUNCTION__, \
                    __LINE__, mensaje);     \
  }                                         \
} while (0)

#else
#define ASSERT(expr, mensaje)  //!< Función ASSERT vacía para cuando está deshabilitada.
#endif

// ===== Error - Funciones Publicas =====
void parar_con_error(const char *fichero, const char *funcion,
                     const uint32_t linea, const char *mensaje);

#endif // ERROR_H
