# Declaración de uso de Inteligencia Artificial Generativa

> Plantilla obligatoria. Debe incluirse en el `README.md` del repositorio del equipo,
> en la sección **«Agradecimientos y Herramientas»**, conforme a la Guía Docente 2026-27.

## Agradecimientos y Herramientas

En el desarrollo de este trabajo se han empleado las siguientes herramientas de
Inteligencia Artificial Generativa:

| Modelo / herramienta | Función concreta realizada | Ficheros afectados |
|---|---|---|
| *(p. ej. Claude Opus 4.6)* | *(p. ej. depuración de la máquina de estados de navegación)* | `agv_fsm.c` |
| | | |

Declaramos que:

- Entendemos y somos capaces de explicar y modificar todo el código entregado.
- Cada función o bloque generado de forma íntegra o sustancial por una IAG va precedido en
  el firmware de un comentario de bloque que indica el prompt empleado o la asistencia recibida.

Firmado: *(nombres de los integrantes del equipo)*

---

## Formato del comentario en el código

```c
/* ===== Bloque asistido por IAG =====
 * Modelo:      <modelo y versión>
 * Asistencia:  <qué se pidió>
 * Prompt:      "<prompt literal empleado>"
 * Revisión:    <qué se ha modificado o verificado a mano>
 */
```
