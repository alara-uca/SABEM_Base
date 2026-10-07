/**
 * @file    tipos.h
 * @brief   Definiciones de tipos para el compilador ARM.
 *
 * @author    Alejandro Lara Doña
 * @date      2026
 * @version   2.1
 *
 * @copyright GNU General Public License version 3 or later
 */

#ifndef TIPOS_H
#define TIPOS_H

#include "fsl_common.h"   // Cabecera del MCX A156, a través del SDK de NXP
#include <math.h>         // Necesario para pow() en ROUND

// ===== Tipos - Tipos estándar =====
/**
 * @brief Incluir stddef.h, que contiene definiciones para NULL, size_t, ptrdiff_t y wchar_t.
 */
#include <stddef.h>

/**
 * @brief Si el compilador es C99 o mayor estarán disponible stdint.h y stdbool.h así que los
 * aprovechamos. El tipo bool_t de define en base al _Bool de stdbool.h.
 */
#if defined ( __STDC_VERSION__ ) && ( __STDC_VERSION__ >= 199901L ) // C99 o superior

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Si el compilador es clang ignoramos errores de codificación de caracteres ANSI
 * en vez de UTF-8.
 */
#ifdef __clang__
#pragma clang diagnostic ignored "-Winvalid-source-encoding"
#endif

typedef _Bool bool_t;

#else
/**
 * @brief Si no estamos en un compilador C99 y no está definido el símbolo __stdint_h,
 * definir los tipos enteros estandar.
 */
#ifndef __stdint_h

typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef signed long long    int64_t;

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

#endif  // __stdint_h

/**
 * @brief Si no está definido el símbolo __bool_true_false_are_defined definir el tipo bool_t como
 * unsigned char, en caso contrario definir a partir de bool.
 */
#ifndef __bool_true_false_are_defined
typedef unsigned char   bool_t;
#else
typedef bool            bool_t;
#endif

#endif  // defined ( __STDC_VERSION__ ) && ( __STDC_VERSION__ >= 199901L )

// Si no están definidos los símbolos FALSE y TRUE, definirlos
#ifndef FALSE
#define FALSE ((bool_t) 0)  //!< Definir símbolo FALSE en caso de no estar definido.
#endif

#ifndef TRUE
#define TRUE  ((bool_t) 1)  //!< Definir símbolo TRUE en caso de no estar definido.
#endif

// Definir tipos para datos flotantes que indiquen claramente su tamaño
typedef float   float32_t;  //!< Tipo flotante de 32 bits.
typedef double  float64_t;  //!< Tipo flotante de 64 bits.


/**
 * @name Tipos - Funciones Macro Públicas
 * @details Funciones de propósito general definidas como macros.
 * @{
 */
/**
 * @brief   _BIT(n) establece a 1 el bit en la posicion "n".
 * @ingroup Tipos_Funciones
 *
 * @param[in] n   Posición del bit.
 *
 * @details _BIT(n) está pensado para utilizarse en expresiones lógicas tipo "OR" o "AND"
 * ej. "(_BIT(3) | _BIT(7))".
 */
#undef  _BIT
#define _BIT(n) (1u << n)

/**
 * @brief   _CAMPO(f, v) establece a el campo que empieza en la posicion 'f' al valor 'v'.
 * @ingroup Tipos_Funciones
 *
 * @param[in] f   Bit de inicio del campo de bits.
 * @param[in] v   Valor del campo de bits.
 *
 * @details _CAMPO(f, v) está pensado para utilizarse en expresiones lógicas tipo "OR" o "AND".
 * ej. "(_CAMPO(5, 3) | _CAMPO(12, 7))".
 */
#undef  _CAMPO
#define _CAMPO(f, v) (v << f)

/**
 * @brief   Redondea un número 'x' hacia +infinito (techo).
 * @ingroup Tipos_Funciones
 *
 * @param[in] x   El número a redondear (puede ser cualquier tipo numérico).
 *
 * @return El valor redondeado.
 */
#if !defined(CEIL)
#define CEIL(x)           \
({                        \
  __typeof__(x) _x = (x); \
  /* La última expresión es el valor de retorno de la macro */     \
  (_x > 0) ? (long long)(_x + 0.9999999999999999) : (long long)_x; \
})
#endif

/**
 * @brief   Redondea un número 'x' hacia -infinito (suelo).
 * @ingroup Tipos_Funciones
 *
 * @param[in] x   El número a redondear (puede ser cualquier tipo numérico).
 *
 * @return El valor redondeado.
 */
#if !defined(FLOOR)
#define FLOOR(x)            \
({                          \
  __typeof__(x) _x = (x);   \
  _x < 0 ? (long long)(_x - 0.9999999999999999) : (long long) _x; \
})
#endif

/**
 * @brief Redondea un número 'x' hacia cero (trunca la parte decimal).
 * @ingroup Tipos_Funciones
 *
 * @param[in] x   El número a redondear (puede ser cualquier tipo numérico).
 *
 * @return El valor redondeado.
 */
#if !defined(FIX)
#define FIX(x) ((long long)(x))
#endif

/**
 * @brief   Redondea un número 'x' al entero más cercano, con 'n' decimales.
 * @ingroup Tipos_Funciones
 *
 * @param[in] x   El número a redondear (puede ser cualquier tipo numérico).
 * @param[in] n   El número de decimales a los que redondear.
 *
 * @return El valor redondeado.
 */
#if !defined(ROUND)
#define ROUND(x, n) (FLOOR(((x) * pow(10, (n))) + 0.5) / pow(10, (n)))
#endif

/**
 * @brief   Devuelve el mayor de dos valores.
 * @ingroup Tipos_Funciones
 *
 * @param[in] a   El primer valor a comparar.
 * @param[in] b   El segundo valor a comparar.
 *
 * @return El valor más grande entre 'a' y 'b'.
 *
 * @note  Los argumentos se evalúan una sola vez, por lo que es seguro usar expresiones con
 * efectos secundarios (ej. MAX(i++, j--)).
 */
#if !defined(MAX)
#define MAX(a, b)           \
({                          \
  __typeof__(a) _a = (a);   \
  __typeof__(b) _b = (b);   \
  _a > _b ? _a : _b;        \
})
#endif

/**
 * @brief   Devuelve el menor de dos valores.
 * @ingroup Tipos_Funciones
 *
 * @param[in] a   El primer valor a comparar.
 * @param[in] b   El segundo valor a comparar.
 *
 * @return El valor más pequeño entre 'a' y 'b'.
 */
#if !defined(MIN)
#define MIN(a, b)           \
({                          \
  __typeof__(a) _a = (a);   \
  __typeof__(b) _b = (b);   \
  _a < _b ? _a : _b;        \
})
#endif

/**
 * @brief   Calcula el valor absoluto de un número.
 * @ingroup Tipos_Funciones
 *
 * @param[in] x   El número de entrada (positivo o negativo).
 *
 * @return El valor absoluto de 'x'.
 */
#if !defined(ABS)
#define ABS(x)              \
({                          \
  __typeof__(x) _x = (x);   \
  _x > 0 ? _x : -_x;        \
})
#endif

/**
 * @brief   Limita un valor a un rango específico.
 * @ingroup Tipos_Funciones
 *
 * @param[in] amt   El valor a limitar.
 * @param[in] low   El límite inferior del rango.
 * @param[in] high  El límite superior del rango.
 *
 * @return 'amt' si está dentro de [low, high]. 'low' si 'amt' es menor, 'high' si 'amt' es mayor.
 */
#if !defined(CONSTRAIN)
#define CONSTRAIN(amt, low, high)   \
({                                  \
  __typeof__(amt) _amt = (amt);     \
  __typeof__(low) _low = (low);     \
  __typeof__(high) _high = (high);  \
  _amt < _low ? _low : (_amt > _high ? _high : _amt);  \
})
#endif

/**
 * @brief   Calcula el cuadrado de un número (x*x).
 * @ingroup Tipos_Funciones
 *
 * @param[in] x   El número a elevar al cuadrado.
 *
 * @return El resultado de 'x' multiplicado por sí mismo.
 */
#if !defined(SQ)
#define SQ(x)               \
({                          \
  __typeof__(x) _x = (x);   \
  _x * _x;                  \
})
#endif

/**
 * @brief   Mapea un número de un rango de entrada a un rango de salida.
 * @ingroup Tipos_Funciones
 *
 * @details Realiza una interpolación lineal para reescalar un valor de un rango [in_min, in_max]
 * a un nuevo rango [out_min, out_max]. No limita el valor si se sale de los rangos.
 *
 * @param[in] x         El valor a mapear.
 * @param[in] in_min    El límite inferior del rango de entrada.
 * @param[in] in_max    El límite superior del rango de entrada.
 * @param[in] out_min   El límite inferior del rango de salida.
 * @param[in] out_max   El límite superior del rango de salida.
 *
 * @return El valor `x` reescalado al rango de salida.
 *
 * @note Esta versión es más segura que la tradicional, ya que evalúa cada argumento una sola vez
 * y preserva los tipos de datos.
 */
#if !defined(MAP)
#define MAP(x, in_min, in_max, out_min, out_max)  \
({                                                \
  __typeof__(x) _x = (x);                         \
  __typeof__(in_min) _in_min = (in_min);          \
  __typeof__(in_max) _in_max = (in_max);          \
  __typeof__(out_min) _out_min = (out_min);       \
  __typeof__(out_max) _out_max = (out_max);       \
  (_x - _in_min) * (_out_max - _out_min) / (_in_max - _in_min) + _out_min;        \
})
#endif
//!@}

#endif  // TIPOS_H
