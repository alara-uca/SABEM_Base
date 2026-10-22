/**
 * @file    <modulo>.h
 * @brief   <Descripción de una línea de lo que hace el módulo.>
 *
 * @details <Descripción más detallada de lo que hace el módulo.>
 *
 * @author  <Nombre del autor> - <correo del autor>
 * @date    [Fecha de creación del módulo/Fecha de la última modificación]
 * @version 1.0
 *
 * @copyright GNU General Public License version 3 or later
 */

/**
 * @defgroup  <Modulo> <MODULO> - <Título corto del módulo>
 * @brief     <Descripción de una línea de lo que hace el módulo.>
 */

#ifndef <MODULO>_H
#define <MODULO>_H

//... librerías de C, de la más general a la más específica
//... librerías de la asignatura, de la más general a la más específica

// ===== <Modulo> - Constantes Publicas =====
/**
 * @brief <Qué parametriza este enumerado.>
 * @ingroup <Modulo>
 */
enum <modulo>_config {
  k<Modulo><Constante> = 0,   //!< <Descripción y unidades.>
};

// ===== <Modulo> - Tipos Publicos =====
/**
 * @brief <Qué representa este tipo.>
 * @ingroup <Modulo>
 */
typedef enum {
  k<Modulo><Valor>,           //!< <Descripción.>
} <modulo>_estado_t;

// ===== <Modulo> - Funciones Publicas =====
void <modulo>_inicializar(void);

#endif  // <MODULO>_H
