# Módulos del proyecto (bibliotecas activables desde `prj.conf`)

Cada módulo de `modulos/` es una biblioteca (pantalla, un periférico, código común...) que se
añade al proyecto **copiando su carpeta** y se activa con **una línea en `prj.conf`**. No hace
falta tocar el `CMakeLists.txt` ni el `Kconfig` del proyecto. Los módulos relacionados se pueden
agrupar en subcarpetas, como los de la pantalla en `lcd/`.

Se apoya en el mecanismo oficial de **módulos externos** del MCUXpresso SDK
(`EXTRA_MCUX_MODULES`), descrito en
[develop/sdk/integration.md](../../../develop/sdk/integration.md) (apartado *Integrate Other
CMake Build System*) y en
[develop/build_system/Software_Componentization.md](../../../develop/build_system/Software_Componentization.md).

Probado con el SDK 26.06 (`<ruta-del-SDK>/mcuxsdk`), toolchain `armgcc` y los presets de
la extensión MCUXpresso para VS Code.

## 1. Módulos disponibles

```text
modulos/
├── README.md             ← este fichero: cómo funcionan los módulos
├── comun/                ← tipos.h y macros de PRINTF (activar siempre)
├── error/                ← ERROR(), ASSERT()
└── lcd/                  ← biblioteca de la pantalla LCD-PAR-S035 (ver lcd/README.md)
    ├── README.md
    ├── lcd_panel/
    ├── lcd/
    ├── lcd_tactil/
    └── lcd_lvgl/
```

| Módulo | Carpeta | Símbolo | Contenido | Depende de |
| --- | --- | --- | --- | --- |
| `comun` | `comun/` | `MCUX_COMPONENT_COMUN` | `tipos.h`; macros `PRINTF_FLOAT_ENABLE=1` y `PRINTF_ADVANCED_ENABLE=1` (siempre juntas, ver [comun/README.md](comun/README.md)) | `driver.common` |
| `error` | `error/` | `MCUX_COMPONENT_ERROR` | `ERROR()`, `ASSERT()`, `parar_con_error()` | `comun`. Usa `lcd` o `lcd_lvgl` si están activos, pero no los necesita |
| `lcd_panel` | `lcd/lcd_panel/` | `MCUX_COMPONENT_LCD_PANEL` | Capa baja de la pantalla (ST7796S por FlexIO) | `comun`; drivers del SDK |
| `lcd` | `lcd/lcd/` | `MCUX_COMPONENT_LCD` | `lcd_*`: modo inmediato | `comun`, `lcd_panel` |
| `lcd_tactil` | `lcd/lcd_tactil/` | `MCUX_COMPONENT_LCD_TACTIL` | `lcd_tactil_*`: GT911 | `comun`, `lcd_panel`; drivers del SDK |
| `lcd_lvgl` | `lcd/lcd_lvgl/` | `MCUX_COMPONENT_LCD_LVGL` | `lcd_lvgl_*` sobre LVGL 9.4 | `comun`, `lcd_panel`, `lcd_tactil`; LVGL |

En `prj.conf` sólo se activa lo que usa el programa: las dependencias (`comun`, `lcd_panel`, los
drivers del SDK, LVGL) se activan solas. Sus carpetas, eso sí, tienen que estar copiadas.

Todo lo específico de la pantalla (capas, LVGL, FreeRTOS, memoria, opciones y ejemplos) está en
[lcd/README.md](lcd/README.md).

### 1.1. `error`

Opciones (se activan solas; se pueden desactivar en `prj.conf` para no mostrar el mensaje):

| Opción | Por defecto | Efecto |
| --- | --- | --- |
| `ERROR_EN_LCD` | `y` si está activo el módulo `lcd` | El error se muestra con `lcd_*`. |
| `ERROR_EN_LCD_LVGL` | `y` si está activo `lcd_lvgl` y no `lcd` | Se muestra con `lcd_lvgl_*` y se fuerza el repintado. |

Sin ninguna de las dos, `parar_con_error()` detiene el programa sin mensaje. Ejemplo:
`error/ejemplos/ejemplo_error.c` (con `LCD` y `ERROR` en `prj.conf`).

## 2. Estructura obligatoria de un módulo

```text
<módulo>/                     ← en modulos/ o en una subcarpeta suya (modulos/lcd/<módulo>)
├── README.md                 ← muy breve: qué es, activación, dependencias, opciones, ejemplos
├── inc/                      ← cabeceras públicas (.h)
├── src/                      ← fuentes (.c); puede no existir si sólo hay cabeceras
├── ejemplos/                 ← opcional: programas de ejemplo (no se compilan)
└── mcux/                     ← datos para el sistema de construcción del SDK
    ├── module.yml
    ├── Kconfig
    └── CMakeLists.txt
```

El sistema reconoce como módulo cualquier carpeta de `modulos/`, a cualquier profundidad, que
tenga `mcux/module.yml`. Una carpeta sin `mcux/` (como `lcd/`) sólo agrupa módulos. El nombre del
módulo es el `name:` de su `module.yml`, no la ruta: tiene que ser único en todo el proyecto.

### 2.1. `mcux/module.yml`

```yaml
name: <nombre>          # igual que la carpeta del módulo
build:
  cmake: mcux           # carpeta con el CMakeLists.txt (relativa a la del módulo)
  kconfig: mcux/Kconfig
  depends:              # opcional: módulos propios que deben cargarse antes
    - comun
```

### 2.2. `mcux/Kconfig`

```kconfig
config MCUX_COMPONENT_<NOMBRE>
    bool "<descripción corta>"
    select MCUX_COMPONENT_COMUN               # módulos propios que necesita
    select MCUX_COMPONENT_driver.<driver>     # drivers/componentes del SDK que necesita
    help
      <qué hace el módulo>
```

Reglas:

- El símbolo se llama `MCUX_COMPONENT_<NOMBRE>`, con `<NOMBRE>` en **mayúsculas** (`lcd_tactil`
  → `MCUX_COMPONENT_LCD_TACTIL`). El prefijo `MCUX_` hace que el SDK **no** lo meta como macro en
  `mcux_config.h`, y las mayúsculas evitan choques con los del SDK (`driver.gpio`...).
- Las dependencias se declaran con **`select`**, no con `depends on`: así, al activar el módulo,
  se activan solos sus drivers, incluidos los símbolos invisibles del SDK que no se pueden poner
  en `prj.conf` (por ejemplo `driver.dbi_flexio_edma`).
- Los nombres de los drivers están en los `Kconfig` del SDK (`mcuxsdk/drivers/*/Kconfig`,
  `mcuxsdk/components/**/Kconfig`), con la forma `MCUX_COMPONENT_driver.<nombre>`.
- No usar variables (`${...}`) en el Kconfig del módulo
  ([Variables.md](../../../develop/build_system/Variables.md)).

Opciones de configuración propias del módulo, dentro de `if MCUX_COMPONENT_<NOMBRE> ... endif`:

- Llegan a **todos** los fuentes del proyecto como macro, porque el SDK incluye `mcux_config.h`
  en cada compilación (`-include`). Con `No prefix in generated macro` en su `help`, la macro no
  lleva el prefijo `CONFIG_`; con `Macro value is in quotes`, un `string` sale entre comillas.
- Si el código define la macro con `#ifndef X ... #define X 1`, la opción tiene que ser `int` y
  no `bool`: una `bool` desactivada no genera ninguna macro, y el `#ifndef` volvería a poner el
  valor por defecto del código.
- Un módulo puede usar otro **sin depender de él** con una opción `depends on` sobre el símbolo
  del otro (así lo hace `error` con `lcd` y `lcd_lvgl`): si el otro no está copiado o no está
  activo, la opción queda a `n` y el código compila sin él. En ese caso el otro módulo no se pone
  en `depends` del `module.yml`.
- Las opciones también están disponibles en CMake como `CONFIG_<opción>` (así pasa `lcd_lvgl` el
  tamaño de la pila al enlazador).

### 2.3. `mcux/CMakeLists.txt`

```cmake
if(CONFIG_MCUX_COMPONENT_<NOMBRE>)
    mcux_add_source(
        BASE_PATH ${CMAKE_CURRENT_LIST_DIR}/..
        SOURCES src/<fichero>.c
                inc/<fichero>.h
    )
    mcux_add_include(
        BASE_PATH ${CMAKE_CURRENT_LIST_DIR}/..
        INCLUDES inc
    )
endif()
```

Reglas:

- **Todo** dentro de un único `if(CONFIG_MCUX_COMPONENT_<NOMBRE>) ... endif()`: sin `if`
  anidados y sin `&&` / `||` en la condición (regla del SDK).
- `BASE_PATH ${CMAKE_CURRENT_LIST_DIR}/..` apunta a la carpeta del módulo (la padre de `mcux/`),
  esté donde esté.
- Se usan `mcux_add_source` / `mcux_add_include`, no `add_library`: así los fuentes se compilan
  dentro de la aplicación, con las rutas de las cabeceras `fsl_*` y las opciones del SDK.
- Macros de compilación: `mcux_add_macro(CC "-DNOMBRE=valor")`. Opciones del enlazador:
  `mcux_add_armgcc_configuration(LD "...")`. Lo que NXP pone en el `reconfig.cmake` de sus
  ejemplos no llega a los módulos: hay que ponerlo aquí.

## 3. Cómo lo integra el proyecto

En el `CMakeLists.txt` del proyecto, **antes de `include(.../mcux.cmake)` y de `project()`**:

```cmake
file(GLOB_RECURSE modulos_yml CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/modulos/module.yml)
foreach(modulo_yml ${modulos_yml})
    cmake_path(GET modulo_yml PARENT_PATH carpeta_mcux)     # .../<módulo>/mcux
    cmake_path(GET carpeta_mcux FILENAME nombre_carpeta)
    if(nombre_carpeta STREQUAL "mcux")
        cmake_path(GET carpeta_mcux PARENT_PATH modulo)     # .../<módulo>
        list(APPEND EXTRA_MCUX_MODULES ${modulo})
    endif()
endforeach()
```

Busca todos los `module.yml` que estén dentro de una carpeta `mcux/`, a cualquier profundidad, y
registra la carpeta padre como módulo.

Qué hace el SDK con eso (comprobado en su código, no sólo en la documentación):

1. Dentro de `project()` se incluye `mcuxsdk/cmake/extension/mcux_module.cmake`, que lee
   `EXTRA_MCUX_MODULES`. Por eso el bloque tiene que ir **antes** de `project()`. Sólo actúa si
   la variable `WEST` está definida; los presets de la extensión ya la definen (`"WEST": "TRUE"`).
2. El script `mcuxsdk/scripts/misc/mcux_module.py` genera `debug/Kconfig/Kconfig.modules` con un
   `osource` al `Kconfig` de cada módulo (menú *External Modules*, cargado desde
   `Kconfig.mcuxpresso`). Por eso los `select` del módulo pueden activar drivers del SDK.
3. Ese script crea además un símbolo `MCUX_COMPONENT_<NOMBRE>_MODULE`, siempre a `y`, que sólo
   indica que la carpeta está presente. **Lo que decide si se compila es
   `MCUX_COMPONENT_<NOMBRE>`**, el de `prj.conf`.
4. `mcux_load_extra_module()` (en `mcux_module.cmake`) hace `add_subdirectory` del
   `mcux/CMakeLists.txt` de cada módulo, respetando `depends`. `mcux_add_source` y
   `mcux_add_include` añaden directamente al ejecutable del proyecto, que ya existe en ese
   momento.

## 4. Cómo se ha hecho y comprobado

1. La biblioteca de la pantalla (antes en `pantalla/`) y `tipos.h` se convirtieron en los módulos
   `comun`, `lcd_panel`, `lcd`, `lcd_tactil` y `lcd_lvgl`; después se añadió `error` y los de la
   pantalla se agruparon en `lcd/`.
2. En el `CMakeLists.txt` del proyecto se añadió el bloque del apartado 3, se quitaron los
   fuentes de la biblioteca y el `fsl_dbi_flexio_edma.c` que se añadía a mano (ahora lo activa un
   `select`), y se volvió a rutas relativas al propio fichero, sin `PrjRootDirPath` ni `APP_DIR`
   (NXP anuncia que `PrjRootDirPath` desaparecerá, y `APP_DIR` es de *sysbuild*).
3. En `prj.conf` se añadió `CONFIG_MCUX_COMPONENT_LCD=y`.

Pruebas, desde una caché limpia (`debug/` borrada) con `cmake --preset debug` y
`cmake --build --preset debug` (las de la pantalla, en [lcd/README.md](lcd/README.md)):

| Prueba | Resultado |
| --- | --- |
| Configurar el proyecto | CMake informa `Add external module <nombre> ...` por cada módulo. |
| Compilar y enlazar con sólo `LCD` activo | Sin errores ni avisos del compilador. Sólo se compilan `lcd`, `lcd_panel` y `comun`, más `fsl_dbi_flexio_edma.c` del SDK. Mismo tamaño que la versión sin módulos: 68 256 B de código y 5 888 B de datos. |
| `debug/.config` | `CONFIG_MCUX_COMPONENT_LCD=y`, y por `select` `LCD_PANEL`, `COMUN` y `driver.dbi_flexio_edma`. |
| `debug/mcux_config.h` | No aparece ningún símbolo `MCUX_COMPONENT_*`. |
| Módulo copiado pero desactivado (`-DCONFIG_MCUX_COMPONENT_LCD=n`) | Quedan sin activar él y sus dependencias, y `build.ninja` no contiene ninguno de sus fuentes (sólo su `Kconfig` y `CMakeLists.txt` como dependencias de configuración). |
| Módulo activado en `prj.conf` pero sin su carpeta | Kconfig avisa de `undefined symbol` y la configuración continúa. Ver apartado 6. |
| `error` con `lcd` y con `lcd_lvgl` (un `ERROR()` en el ejemplo de cada uno) | En la placa aparece la pantalla de error con el mensaje, el fichero, la función y la línea. `ERROR_EN_LCD` / `ERROR_EN_LCD_LVGL` se activaron solas, cada una en su caso. |
| Módulos agrupados en una subcarpeta (`modulos/lcd/`) | Se registran los seis módulos (`comun`, `error` y los cuatro de `lcd/`) y compila con el mismo tamaño que antes de agruparlos. |
| `ejemplo_error.c`, con `LCD` y `ERROR` | Compila y, en la placa, al llegar a `100 / 0` salta el `ASSERT()`: pantalla de error con el mensaje, `main.c`, la función `dividir` y la línea del `ASSERT()`. |

**No probado:** la compilación con `west` fuera de VS Code y la generación de proyectos para IDE
(*standalone project generation*), que NXP no garantiza con módulos externos.

## 5. Instrucciones para el usuario

### 5.1. Añadir un módulo existente a un proyecto que ya usa módulos

1. Copiar la carpeta completa del módulo (con su `mcux/`) en `modulos/`, o la carpeta que agrupa
   varios (por ejemplo `lcd/`). Copiar también los módulos de los que depende (los que aparecen en
   `depends` de su `module.yml`; por ejemplo `comun`).
2. Añadir en el `prj.conf` de la raíz del proyecto:

    ```text
    CONFIG_MCUX_COMPONENT_<NOMBRE>=y
    ```

3. En VS Code: paleta de comandos (`Ctrl+Mayús+P`) → **CMake: Delete Cache and Reconfigure**.
   Es necesario porque el proyecto tiene `cmake.skipConfigureIfCachePresent: true` y, si no,
   puede no detectar la carpeta nueva ni el cambio de `prj.conf`.
4. Compilar. En la salida de la configuración debe aparecer `Add external module <nombre> ...`.
5. Incluir sus cabeceras en el código (`#include "lcd.h"`).

### 5.2. Quitar un módulo

- Para no compilarlo: borrar su línea de `prj.conf` (o ponerla a `=n`) y reconfigurar.
- Para quitarlo del todo: además, borrar su carpeta de `modulos/`.

### 5.3. Crear un módulo nuevo (por ejemplo, de un periférico)

1. Crear la carpeta del módulo en `modulos/` (o en una subcarpeta que agrupe varios) con `inc/`,
   `src/` y `mcux/`, siguiendo el apartado 2.
2. Buscar en el SDK qué drivers usa (los `#include "fsl_*.h"` de sus fuentes) y el símbolo de
   cada uno en su `Kconfig`; ponerlos como `select` en `mcux/Kconfig`.
3. Si usa `tipos.h`: `select MCUX_COMPONENT_COMUN` en el `Kconfig` y `comun` en `depends` del
   `module.yml`.
4. Si tiene ejemplos, ponerlos en `ejemplos/`, indicando en la cabecera qué módulos activar.
5. Escribir su `README.md`, breve (el detalle va en los comentarios del código): qué es, línea de
   `prj.conf` que lo activa, de qué depende, sus opciones y sus ejemplos. Sirve de modelo el de
   cualquier módulo existente.
6. Seguir los pasos del apartado 5.1 para activarlo y comprobar que compila.

### 5.4. Preparar un proyecto nuevo para usar módulos

1. Crear la carpeta `modulos/` en la raíz del proyecto.
2. Copiar en su `CMakeLists.txt` el bloque del apartado 3, **antes** de
   `include(${SdkRootDirPath}/cmake/extension/mcux.cmake)`.
3. Comprobar que `CMakePresets.json` define `"WEST": "TRUE"` en `cacheVariables` (los proyectos
   importados con la extensión MCUXpresso ya lo traen).
4. Si el proyecto añadía a mano fuentes de una biblioteca que ahora es módulo, quitarlos del
   `CMakeLists.txt` para que no se compilen dos veces.

### 5.5. Probar un ejemplo

1. Copiar el ejemplo (`<módulo>/ejemplos/<ejemplo>.c`) como `main.c` del proyecto.
2. Activar en `prj.conf` los módulos que indica su cabecera.
3. Reconfigurar y compilar.

## 6. Avisos conocidos

- CMake avisa de que `PrjRootDirPath` no se usa. Viene de `CMakePresets.json` (lo genera la
  extensión) y no afecta a la compilación.
- Si un módulo está activado pero su carpeta no está copiada, la configuración **no se
  detiene**: sólo da este aviso, fácil de pasar por alto, y después falla el enlace con
  referencias sin definir. El aviso empieza por `prj.conf:<línea>:`:

    ```text
    warning: attempt to assign the value 'y' to the undefined symbol MCUX_COMPONENT_LCD
    ```

- Si `main.c` usa un módulo que está desactivado, el enlace falla con referencias sin definir.
- El formato de `prj.conf` no admite comentarios al final de una línea
  (`CONFIG_X=y  # comentario`): el comentario tiene que ir en su propia línea.
- **`%f` en `PRINTF`.** `debug_console_lite` lo ignora (sale vacío) si no está `PRINTF_FLOAT_ENABLE`,
  y con solo esa macro **imprime los `float` negativos sin signo**. Las dos macros
  (`PRINTF_FLOAT_ENABLE` y `PRINTF_ADVANCED_ENABLE`) las define el módulo `comun` y **tienen que ir
  siempre juntas**, en una sola palabra `CC` de `mcux_add_macro`. Por eso todos los proyectos llevan
  `CONFIG_MCUX_COMPONENT_COMUN=y` en `prj.conf`. Detalles en [comun/README.md](comun/README.md).
- **Rutas largas en Windows.** Si la carpeta del proyecto tiene una ruta larga, se puede superar
  el límite de 260 caracteres (con LVGL es fácil; ver [lcd/README.md](lcd/README.md)).
