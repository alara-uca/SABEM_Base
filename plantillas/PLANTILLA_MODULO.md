# Guía de los módulos: cómo se activan y cómo se crea uno

En la asignatura, cada biblioteca (la pantalla, el tratamiento de errores y, más adelante, los
sensores, los motores...) es un **módulo**: una carpeta dentro de `modulos/` que se activa con
**una línea en `prj.conf`**. Esta guía explica qué hay detrás de esa línea, cómo usar un módulo
que ya existe y cómo crear uno nuevo. Tenla a mano en todas las prácticas.

---

## 1. Qué es un módulo y por qué se trabaja así

Un proyecto del SDK de NXP no compila todo lo que hay en sus carpetas: compila lo que **está
activado**. Los drivers del propio SDK (GPIO, FlexIO, I2C...) funcionan así, y nuestros módulos
también. Hay tres piezas:

*Figura 1 – Cómo se decide qué se compila*

```text
   prj.conf                      Kconfig del módulo                  CMakeLists.txt del módulo
   (lo que pides)         →      (qué más necesita)           →      (qué ficheros se compilan)

   CONFIG_MCUX_COMPONENT_LCD=y   LCD activa LCD_PANEL y COMUN,       si LCD está activo:
                                 y los drivers del SDK               compila lcd.c y fuente.c
                                 que usan
```

1. **`prj.conf`** dice qué módulos quieres. Es lo único que tocas para usar un módulo.
2. El **`Kconfig`** de cada módulo dice qué otros módulos y qué drivers del SDK necesita, y los
   activa solos. Por eso basta con pedir `LCD`: `lcd_panel`, `comun`, el driver del FlexIO, etc.
   vienen detrás. El resultado completo queda escrito en `debug/.config`.
3. El **`CMakeLists.txt`** de cada módulo dice qué ficheros `.c` se compilan y dónde están sus
   cabeceras, pero sólo si el módulo quedó activado.

Ventajas: el `CMakeLists.txt` del proyecto no se toca nunca, un módulo que no usas no ocupa
memoria, y no hay que buscar a mano qué drivers del SDK hacen falta.

> [!NOTE]
> El `CMakeLists.txt` del proyecto base ya trae preparado el registro de la carpeta `modulos/`:
> busca dentro de ella toda carpeta que tenga `mcux/module.yml` y la da de alta como módulo.

---

## 2. Usar un módulo que ya existe

1. **Copia su carpeta** dentro de `modulos/` del proyecto, completa (con su carpeta `mcux/`). Si
   el módulo necesita otros módulos (lo dice su `README.md`), cópialos también.
2. **Actívalo** en `prj.conf`, en una línea propia:

    ```text
    CONFIG_MCUX_COMPONENT_LCD=y
    ```

    El nombre es `CONFIG_MCUX_COMPONENT_` seguido del nombre del módulo **en mayúsculas**
    (`lcd_tactil` → `CONFIG_MCUX_COMPONENT_LCD_TACTIL`).
3. **Borra la caché** y vuelve a configurar: en VS Code, paleta de comandos (`Ctrl+Mayús+P`) →
   **CMake: Delete Cache and Reconfigure**.
4. **Compila** y carga.

> [!WARNING]
> **¡Sin borrar la caché no se entera!** El proyecto está configurado para no repetir la
> configuración si ya existe, así que un cambio en `prj.conf` o un módulo nuevo no se notan
> hasta que se borra. Ante cualquier comportamiento raro tras tocar `prj.conf`, lo primero es
> borrar la caché.

> [!WARNING]
> **En `prj.conf` los comentarios van en su propia línea.** `CONFIG_MCUX_COMPONENT_LCD=y  # la
> pantalla` no funciona: el comentario estropea la opción.

Para **desactivar** un módulo, pon la línea a `n` (`CONFIG_MCUX_COMPONENT_LCD=n`) o bórrala.

### 2.1 Cómo comprobar que funciona

| Dónde mirar | Qué tiene que aparecer |
|---|---|
| Salida de la configuración (panel *Output* → *CMake*) | Una línea `Add external module <nombre> ...` por cada módulo que hay en `modulos/` (esté activado o no). Si falta, la carpeta no está o no tiene `mcux/module.yml`. |
| `debug/.config` | `CONFIG_MCUX_COMPONENT_<NOMBRE>=y` para el módulo que has pedido y para los que ha activado él solo. |
| Salida de la compilación | Sin errores ni avisos. Al final, el tamaño del programa (*text*, *data*, *bss*). |
| La placa | Lo que diga el ejemplo del módulo. |

---

## 3. Cómo es la carpeta de un módulo

*Figura 2 – Carpeta de un módulo*

```text
modulos/
└── leds/                       ← la carpeta se llama como el módulo
    ├── README.md               ← qué es, cómo se activa, de qué depende
    ├── inc/
    │   └── leds.h              ← cabecera pública: lo que puede usar el programa
    ├── src/
    │   └── leds.c              ← el código
    ├── ejemplos/
    │   └── ejemplo_leds.c      ← un main.c de prueba (no se compila: se copia como main.c)
    └── mcux/                   ← lo que necesita el sistema de compilación
        ├── module.yml
        ├── Kconfig
        └── CMakeLists.txt
```

Una carpeta **sin** `mcux/` no es un módulo: sólo agrupa otros. Es el caso de `modulos/lcd/`,
que contiene los módulos `lcd_panel`, `lcd`, `lcd_tactil` y `lcd_lvgl`.

---

## 4. Crear un módulo nuevo

Lo que distingue un módulo de una carpeta con código son los tres ficheros de `mcux/`. Los
explicamos con un módulo **inventado** para la ocasión, `leds`, que enciende y apaga los LED de
la placa: tiene `src/leds.c`, `inc/leds.h`, usa `tipos.h` (del módulo `comun`) e incluye
`fsl_gpio.h` y `fsl_port.h` (drivers GPIO y PORT del SDK).

### 4.1 `mcux/module.yml`: el módulo existe

Le dice al sistema cómo se llama el módulo y dónde están sus otros dos ficheros.

```yaml
name: leds
build:
  cmake: mcux
  kconfig: mcux/Kconfig
  depends:
    - comun
```

| Línea | Qué significa |
|---|---|
| `name: leds` | Nombre del módulo. Igual que su carpeta, en minúsculas y **único en todo el proyecto**. |
| `cmake: mcux` | Carpeta donde está su `CMakeLists.txt`, contada desde la carpeta del módulo. |
| `kconfig: mcux/Kconfig` | Fichero `Kconfig` del módulo, contado desde la carpeta del módulo. |
| `depends:` | Otros **módulos de la asignatura** que tienen que cargarse antes que este. Aquí, `comun`, porque `leds.c` usa `tipos.h`. Si no depende de ninguno, se quita todo el bloque `depends`. Los drivers del SDK **no** van aquí: van en el `Kconfig`. |

### 4.2 `mcux/Kconfig`: qué se activa con él

Define la opción que se escribe en `prj.conf` y qué más se activa con ella.

```kconfig
config MCUX_COMPONENT_LEDS
    bool "LED de la placa"
    select MCUX_COMPONENT_COMUN
    select MCUX_COMPONENT_driver.common
    select MCUX_COMPONENT_driver.gpio
    select MCUX_COMPONENT_driver.port
    help
      Funciones para encender, apagar y conmutar los LED de la placa.
```

| Línea | Qué significa |
|---|---|
| `config MCUX_COMPONENT_LEDS` | Nombre de la opción: `MCUX_COMPONENT_` + nombre del módulo **en mayúsculas**. En `prj.conf` se escribe con `CONFIG_` delante: `CONFIG_MCUX_COMPONENT_LEDS=y`. |
| `bool "..."` | Es una opción de sí o no (`y`/`n`), con una descripción corta. |
| `select MCUX_COMPONENT_COMUN` | Al activar `leds`, se activa solo el módulo `comun`. |
| `select MCUX_COMPONENT_driver.xxx` | Al activar `leds`, se activan solos los drivers del SDK que usa. Hay que poner uno por cada `#include "fsl_xxx.h"` de sus fuentes, más `driver.common`, que necesitan todos. |
| `help` | Explicación más larga. Las líneas de texto van con más sangría que `help`. |

> [!TIP]
> **¿Cómo se llama el driver del SDK?** Casi siempre como su fichero: `fsl_gpio.h` →
> `MCUX_COMPONENT_driver.gpio`, `fsl_lpi2c.h` → `MCUX_COMPONENT_driver.lpi2c`. Si no estás
> seguro, búscalo en el `Kconfig` de la carpeta del driver dentro del SDK
> (`mcuxsdk/drivers/<driver>/Kconfig`), o mira qué hay activado en `debug/.config` de un
> proyecto que ya lo use.

> [!IMPORTANT]
> **Las dependencias van con `select`, no con `depends on`.** `select` activa lo que haga
> falta; `depends on` sólo deja activar el módulo si lo otro ya estaba activado, y obligaría a
> escribir en `prj.conf` todos los drivers a mano.

**Opciones propias del módulo (avanzado).** Un módulo puede tener ajustes que llegan al código
como macros. Van entre `if MCUX_COMPONENT_<NOMBRE>` y `endif`, para que sólo existan si el
módulo está activado. Por ejemplo, si los LED de otra placa se encendieran con un 1:

```kconfig
if MCUX_COMPONENT_LEDS

config LEDS_ACTIVOS_A_NIVEL_ALTO
    bool "Los LED se encienden con un 1"
    default n
    help
      No prefix in generated macro

      Si está a y, leds.c enciende con un 1 en lugar de con un 0.

endif
```

- `default n` es el valor si nadie escribe la opción en `prj.conf`.
- La frase `No prefix in generated macro` **dentro del `help`** hace que la macro llegue al
  código como `LEDS_ACTIVOS_A_NIVEL_ALTO` y no como `CONFIG_LEDS_ACTIVOS_A_NIVEL_ALTO`. En el
  código se usa con `#if defined(LEDS_ACTIVOS_A_NIVEL_ALTO) && LEDS_ACTIVOS_A_NIVEL_ALTO`.
- Una opción puede depender de **otro módulo** sin obligar a usarlo: con
  `depends on MCUX_COMPONENT_LCD`, la opción sólo puede estar a `y` si el módulo `lcd` está
  activado; si no lo está, queda a `n` y el código compila sin la pantalla. Con `default y`,
  además, se activa sola en cuanto `lcd` está activado.

### 4.3 `mcux/CMakeLists.txt`: qué se compila

Dice qué ficheros del módulo se compilan y dónde están sus cabeceras.

```cmake
if(CONFIG_MCUX_COMPONENT_LEDS)
    mcux_add_source(
        BASE_PATH ${CMAKE_CURRENT_LIST_DIR}/..
        SOURCES src/leds.c
                inc/leds.h
    )
    mcux_add_include(
        BASE_PATH ${CMAKE_CURRENT_LIST_DIR}/..
        INCLUDES inc
    )
endif()
```

| Línea | Qué significa |
|---|---|
| `if(CONFIG_MCUX_COMPONENT_LEDS)` | Sólo se compila si el módulo está activado. Aquí **sí** se escribe `CONFIG_` delante. |
| `BASE_PATH ${CMAKE_CURRENT_LIST_DIR}/..` | Las rutas que siguen se cuentan desde la carpeta del módulo (la que contiene a `mcux/`). Así funciona aunque muevas el módulo de sitio. |
| `mcux_add_source(... SOURCES ...)` | Los ficheros del módulo que se añaden al proyecto: todos sus `.c` y `.h`, salvo los ejemplos. |
| `mcux_add_include(... INCLUDES inc)` | La carpeta de las cabeceras, para que `#include "leds.h"` funcione desde cualquier fichero. |

> [!WARNING]
> Todo va dentro de **un único** `if ... endif()`, sin `if` anidados y sin `&&` ni `||` en la
> condición: es una regla del SDK.

### 4.4 Pasos, en orden

1. Crea la carpeta del módulo en `modulos/` con `inc/`, `src/`, `ejemplos/` y `mcux/`, y pon cada
   fichero en su sitio.
2. Escribe los tres ficheros de `mcux/` (apartados 4.1 a 4.3).
3. Escribe el `README.md` del módulo (apartado 5).
4. Copia el ejemplo del módulo como `main.c` y activa el módulo en `prj.conf` con las líneas que
   diga el ejemplo en su cabecera.
5. Borra la caché, compila y comprueba lo del apartado 2.1.
6. **Prueba negativa:** pon el módulo a `n`, borra la caché y compila. Tiene que fallar: el
   programa ya no encuentra la cabecera del módulo (`fatal error: <módulo>.h: No such file or
   directory`). Así sabes que es tu `CMakeLists.txt` quien añade el módulo al proyecto, y no otra
   cosa. Si compila igual, queda una copia de la cabecera o del `.c` fuera de la carpeta del
   módulo.

---

## 5. El `README.md` del módulo

Corto. Sirve para que otra persona sepa en un minuto cómo usarlo:

```markdown
# Módulo `leds`

Encender, apagar y conmutar los LED de la placa.

- **Activación:** `CONFIG_MCUX_COMPONENT_LEDS=y` en `prj.conf`.
- **Depende de:** `comun`; drivers del SDK `gpio` y `port`.
- **Opciones:** `LEDS_ACTIVOS_A_NIVEL_ALTO` (por defecto, `n`).
- **Ejemplo:** `ejemplos/ejemplo_leds.c` (con `LEDS`).

Detalles: comentarios de `inc/leds.h`.
```

---

## 6. Errores frecuentes

*Tabla 1 – Síntomas, causa y solución*

| Síntoma | Causa probable | Solución |
|---|---|---|
| `fatal error: lcd.h: No such file or directory` (o la cabecera de otro módulo, o `tipos.h`) | El módulo no está activado en `prj.conf`, o se activó pero no se borró la caché: sus cabeceras sólo se añaden al proyecto si está activado | Añade la línea en `prj.conf`, borra la caché y compila |
| `undefined reference to '...'` (una función del módulo) al enlazar | La cabecera se encuentra pero el `.c` no se compila: falta en `SOURCES` del `CMakeLists.txt` del módulo, o hay una copia de la cabecera fuera del módulo | Revisa el apartado 4.3 y busca copias sueltas del `.h` |
| Al configurar sale `prj.conf:<línea>: warning: attempt to assign the value 'y' to the undefined symbol MCUX_COMPONENT_X` | La carpeta del módulo no está en `modulos/`, le falta `mcux/`, o el nombre no coincide con el del `Kconfig` | Copia la carpeta completa; revisa el nombre en mayúsculas |
| No sale la línea `Add external module <nombre>` | Falta `mcux/module.yml` o está mal escrito | Revisa la carpeta `mcux/` y el `module.yml` |
| `fatal error: xxx.h: No such file or directory` | Falta `mcux_add_include` en el `CMakeLists.txt` del módulo, o el `.h` no está en `inc/` | Revisa el apartado 4.3 |
| `fatal error: fsl_xxx.h: No such file or directory` | Falta el `select` del driver del SDK en el `Kconfig` | Añade `select MCUX_COMPONENT_driver.xxx` |
| `multiple definition of ...` | Un fichero se compila dos veces (por ejemplo, porque también está en la raíz del proyecto o añadido a mano en el `CMakeLists.txt` del proyecto) | Deja cada fichero en un único sitio |
| Un cambio en `prj.conf` no hace nada | No se borró la caché | *CMake: Delete Cache and Reconfigure* |
| Una opción de `prj.conf` deja de funcionar | Tiene un comentario al final de la línea | Pon el comentario en su propia línea |
| `opening dependency file ... No such file or directory` | Ruta del proyecto demasiado larga (Windows admite 260 caracteres) | Mueve el proyecto a una carpeta más corta |
