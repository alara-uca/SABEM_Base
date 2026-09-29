# Firmware del AGV

Aquí va el **proyecto de MCUXpresso for VS Code** del equipo: el que se importa desde el SDK de la
FRDM-MCXA156 y en el que se escribe el firmware del AGV.

## Cómo crearlo

1. Seguid la guía de instalación (`../docs/Guia_instalacion_VSCode_MCUXpresso.md`) hasta tener el
   SDK importado.
2. Importad el proyecto de partida **dentro de esta carpeta** (`firmware/`), no en la raíz del
   repositorio.
3. Antes del primer *commit* con el proyecto dentro, comprobad que el `.gitignore` de la raíz
   deja fuera lo que genera la extensión (`debug/`, `release/`, `__repo__/`). Un fichero que ya
   se ha subido no deja de estar seguido por Git porque después lo añadáis al `.gitignore`.
4. `mcux_include.json` y `.vscode/mcuxpresso-tools.json` **sí se suben**: la extensión los
   necesita y da error si faltan. Contienen rutas de cada PC, pero los reescribe sola al abrir
   el proyecto. Si un `git status` los muestra modificados sólo por rutas, no los incluyáis
   en vuestro commit (`git restore <fichero>`).

## Qué se versiona

Sí: vuestros `*.c` y `*.h`, `CMakeLists.txt`, `prj.conf`, `Kconfig`, `CMakePresets.json`,
`mcux_include.json`, `.vscode/mcuxpresso-tools.json` y los ficheros de la placa. No: la salida de
compilar (`debug/`, `release/`) ni `__repo__/`. La explicación
completa está en la lección L1.1.
