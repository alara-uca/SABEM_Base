/**
 * @file      Nombre del archivo.c
 * @author    Nombre del autor - Correo electrónico o medio de contacto
 * @date      [Fecha de creación/Fecha de la última modificación]
 * @version   Versión del archivo.
 * @copyright Licencia de aplicación al archivo.
 *
 * @brief   Resumen corto y directo de lo que hace el elemento.
 * @details Descripción más detallada sobre el funcionamiento, casos de uso, limitaciones...
 *
 * @param[in]     arg_in    Descripción del parámetro de entrada.
 * @param[out]    arg_out   Descripción del parámetro de salida.
 * @param[in,out] arg_inout Descripción del parámetro de entrada/salida.
 *
 * @retval  valor   Varios retval con los valores de retorno y el significado exacto de cada uno.
 * @return  Descripción del valor devuelto. Indicar si hay diferentes valores para éxito o error.
 *
 * @todo    Tarea pendiente de programar o realizar.
 * @note    Comentario importante a tener en cuenta.
 * @warning Advertencia sobre posibles efectos secundarios, riesgos o situaciones no obvias.
 *
 * @pre   Condiciones necesarias antes de la ejecución de la función.
 * @post  Condiciones o estado después de la ejecución de la función.
 *
 * @see   Referencias a otros elementos que pueden ser de utilidad. Ej: funcion_1(), variable
 */

bool_t variable; //!< Descripción en linea. Similar a brief

/**
 * @file    modulo_ejemplo.c
 * @brief   Ejemplo de documentación para un archivo fuente.
 * @details Este archivo contiene ejemplos de documentación para funciones,
 *          enumeraciones, estructuras y definiciones mediante Doxygen.
 *
 * @author  Nombre Apellido - correo@example.com
 * @date    2025-01-15
 * @version 1.0.0
 *
 * @copyright Copyright (c) 2025, Organización de ejemplo.
 *
 * @note    Sustituir estos datos de ejemplo por los del módulo real.
 */

/**
 * @brief   Suma dos números enteros.
 *
 * @details Recibe dos valores enteros y devuelve el resultado de sumarlos.
 *
 * @param[in] a Primer número entero que se sumará.
 * @param[in] b Segundo número entero que se sumará.
 *
 * @return La suma de los parámetros a y b.
 *
 * @note    Los parámetros de entrada no se modifican.
 */
int sumar(int a, int b);

/**
 * @brief Enumeración de estados de ejemplo.
 * @details Identifica el estado actual de una operación del módulo.
 */
typedef enum
{
	ESTADO_INACTIVO = 0, //!< La operación no está en curso.
	ESTADO_ACTIVO,       //!< La operación está en curso.
	ESTADO_ERROR         //!< Se produjo un error durante la operación.
} estado_t;

/**
 * @brief Estructura con los datos de una solicitud.
 * @details Agrupa los valores necesarios para procesar una solicitud.
 */
typedef struct
{
	int identificador; //!< Identificador de la solicitud.
	int prioridad;     //!< Prioridad asignada a la solicitud.
} solicitud_t;

/** @brief Número máximo de solicitudes almacenadas simultáneamente. */
#define MAX_SOLICITUDES 10

//!<
