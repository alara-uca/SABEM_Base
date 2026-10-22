# Proyecto base (FRDM-MCXA156 + pantalla LCD-PAR-S035)

Proyecto de partida de la asignatura. Ya trae preparado todo lo que suele dar problemas al
empezar: la configuración de la placa hecha en MCUXpresso Config Tools, el núcleo a 96 MHz y la
carpeta `modulos/` registrada en el sistema de compilación. Vosotros sólo tenéis que **activar en
`prj.conf` los módulos que use el programa** y escribir vuestro código.

## Qué hay en la carpeta

```text
P01_LCD/
├── main.c                ← el programa (al principio, el ejemplo de la pantalla: ejemplo_lcd.c)
├── prj.conf              ← QUÉ MÓDULOS SE ACTIVAN (vacío al principio)
├── modulos/              ← las bibliotecas de la asignatura: comun/ y lcd/
├── P01_LCD.mex           ← configuración de Config Tools: pines, relojes y periféricos
├── board/                ← código GENERADO por Config Tools a partir del .mex (no editar a mano)
├── hardware_init.c       ← BOARD_InitHardware(): aplica lo de Config Tools y arranca la consola
├── app.h                 ← prototipo de BOARD_InitHardware()
├── error.c, error.h,
│   ejemplo_error.c       ← código del módulo error, para el ejercicio 4 de la P01
├── CMakeLists.txt        ← no hace falta tocarlo
└── frdmmcxa156/          ← ficheros de la placa del SDK (no hace falta tocarlos)
```

## Primeros pasos

1. El proyecto va en `Practicas/P01_LCD/` de vuestro repositorio. Si vuestro repositorio ya
   existía antes de que se añadiera, descargad el ZIP del Campus Virtual y copiad su contenido
   en esa carpeta. Abridlo con **Import Project** del panel de MCUXpresso y, cuando lo pida,
   elegid vuestro SDK de la FRDM-MCXA156. Si ya lo abristeis directamente como carpeta, la
   extensión no pide el SDK: en el panel *Projects*, clic derecho sobre el proyecto →
   **Configure** → **Associate Repository**, y elegid vuestro SDK.
2. **El proyecto y el SDK tienen que estar en la misma unidad de disco** (los dos en `C:`, por
   ejemplo). Si no, Config Tools no abre el proyecto.
3. Usad una ruta corta y sin espacios ni acentos (por ejemplo, `C:\sabem\...`): LVGL genera
   nombres de fichero muy largos y Windows no admite rutas de más de 260 caracteres.

## Cómo se usa

- **Activar un módulo:** una línea en `prj.conf` (`CONFIG_MCUX_COMPONENT_LCD=y`), borrar la caché
  (*CMake: Delete Cache and Reconfigure*) y compilar. La guía completa, con cómo crear un módulo
  nuevo, está en `Plantillas/PLANTILLA_MODULO.md` del repositorio de la asignatura.
- **Cambiar pines, relojes o periféricos:** en Config Tools (clic derecho sobre el proyecto →
  *Open with MCUXpresso Config Tools*), y después *Update Code*. Si VS Code propone cambios en el
  proyecto, aceptadlos. No editéis a mano los ficheros de `board/`: se pierden al regenerarlos.
- **Los módulos se configuran solos.** La pantalla y el táctil configuran sus propios pines,
  relojes y periféricos: no los asignéis en Config Tools.

> [!WARNING]
> **No uséis el LED azul.** Su pin (`P3_0`) es también el reset de la pantalla y del táctil:
> conmutarlo reinicia la pantalla. En este proyecto está configurado como entrada para que no
> moleste.

> [!WARNING]
> **No configuréis el LPUART0 en Peripherals.** Es la consola de depuración (`PRINTF`) y ya la
> inicializa `BOARD_InitDebugConsole()` en `hardware_init.c`.
