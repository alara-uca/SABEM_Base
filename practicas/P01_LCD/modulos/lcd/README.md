# Pantalla LCD-PAR-S035 — biblioteca común de las dos asignaturas

Pantalla de 3,5" y 480 × 320 píxeles de la **FRDM-MCXA156**: controlador **ST7796S** por un bus
paralelo **8080 de 8 bits fabricado con el FlexIO**, y panel táctil capacitivo **GT911** en el
bus **LPI2C2** (compartido con el RTC y el sensor de distancia de SABEM). La decisión de diseño
está en `00_Gestion/DECISIONES_TOMADAS.md`, sección "Pantalla — CERRADA (21/09/2026)".

La biblioteca se entrega como **cuatro módulos** del sistema de construcción del SDK, agrupados
en esta carpeta. Cómo funcionan los módulos en general (estructura, cómo se añaden a un proyecto,
cómo se crea uno nuevo) está en [../README.md](../README.md).

## 1. Las cuatro capas

```text
   Informática Industrial                     SABEM (4.º)
   ----------------------          -----------------------------------------
   programa                        programa
      |                               |                      |
      |                          lcd_lvgl_*  (modo      widgets de LVGL
      |                          inmediato emulado:     (lv_button, lv_slider...)
      |                          un objeto por llamada)      |
      |                               |                      |
      |                               +------ LVGL 9.4 ------+
      |                                          |
   lcd_*  (modo inmediato directo,        lcd_lvgl_port  (reloj, display
   mismas firmas que glcd_*)              con búferes parciales, indev)
      |                                    |                  |
      +---------------+--------------------+                  |
                      |                                       |
                 lcd_panel                               lcd_tactil
   (ÚNICA dueña del ST7796S y del FlexIO;         (GT911 en LPI2C2, modo inmediato;
    ventana + píxeles, síncrono y asíncrono;       la usa el programa directamente
    mutex del bus con FreeRTOS)                    y la usa LVGL como indev)
                      |                                       |
              FlexIO 8080 + ST7796S                     LPI2C2 + GT911
```

| Módulo (carpeta `modulos/lcd/<módulo>`) | Ficheros | Qué es | Quién lo usa |
| --- | --- | --- | --- |
| `lcd_panel` | `lcd_panel.h/.c` | Capa más baja: inicializa FlexIO y ST7796S, selecciona ventanas y vuelca píxeles (bloqueante o asíncrono con aviso). Con FreeRTOS, protege el bus con un mutex. | `lcd`, `lcd_tactil` y `lcd_lvgl`; nunca el programa. |
| `lcd` | `lcd.h/.c`, `lcd_colores.h`, `fuente.h/.c` | **Modo inmediato directo**, con las mismas firmas que la antigua `glcd_*` (prefijo `lcd_`) y sus tres fuentes de mapa de bits (8×16, 12×24, 16×32). No hay *framebuffer* en el micro: cada llamada escribe su ventana en la memoria de imagen del ST7796S. | Informática Industrial, y cualquiera que necesite dibujo masivo. |
| `lcd_lvgl` | `lcd_lvgl.h/.c`, `lcd_lvgl_port.h/.c`, `lv_conf.h` | **Modo inmediato emulado sobre LVGL**: mismas firmas que `glcd_*` con prefijo `lcd_lvgl_`, pero cada llamada crea (o actualiza) un objeto de LVGL. Se mezcla libremente con los widgets de LVGL. | SABEM. |
| `lcd_tactil` | `lcd_tactil.h/.c` | Táctil en modo inmediato: `lcd_tactil_leer(&x, &y)`. Un solo driver: también es el que lee LVGL. | Las dos asignaturas. |

Todos usan además el módulo `comun` (`tipos.h`), que está fuera de esta carpeta. El módulo
`error` (`ERROR()`, `ASSERT()`) muestra los errores en la pantalla si el proyecto usa `lcd` o
`lcd_lvgl` (ver [../README.md](../README.md)).

### Modo inmediato frente a modo retenido (lo que hay que entender de `lcd_lvgl_*`)

- `lcd_*` (modo inmediato): `lcd_rectangulo(...)` pinta píxeles **ya** y los olvida.
- `lcd_lvgl_*` (modo retenido por debajo): `lcd_lvgl_rectangulo(...)` crea un **objeto**
  rectángulo en el árbol de objetos de LVGL y vuelve. **No se ve nada** hasta que se llama a
  `lcd_lvgl_procesar()` (en el bucle principal o en la tarea de interfaz), que repinta las zonas
  que han cambiado; `lcd_lvgl_refrescar_ya()` fuerza el repintado en el acto.
- Todos los objetos de `lcd_lvgl_*` cuelgan de un **lienzo** transparente de 480 × 320 que está
  al fondo de la pantalla activa, detrás de los widgets.
- El texto se **reutiliza**: escribir otra vez en la misma posición `(x, y)` cambia el texto de
  la etiqueta que ya había (hasta 32 etiquetas recordadas). Las figuras, no: cada llamada es un
  objeto nuevo que ocupa RAM hasta el siguiente `lcd_lvgl_borrar()`. `lcd_lvgl_punto()` crea un
  objeto por píxel: úsese con mesura.
- Fuentes: `FUENTE8X16` → UNSCII 16 (monoespaciada, 8 × 16, como la de glcd). `FUENTE12X24` y
  `FUENTE16X32` → Montserrat 24 y 32, **proporcionales**: las columnas no casan con las de glcd.
- Cada función `lcd_lvgl_*` toma `lv_lock()`: con FreeRTOS se puede llamar desde cualquier
  tarea. Para llamar a la API nativa de LVGL desde una tarea que no sea la de interfaz, hay que
  rodearla de `lv_lock()` / `lv_unlock()` (dentro de los eventos de LVGL no hace falta).

## 2. Regla de mezcla

- **`lcd_lvgl_*` + widgets de LVGL: libremente.** Todo son objetos del mismo árbol; LVGL sabe
  qué hay en cada sitio y repinta bien los solapes.
- **`lcd_*` + LVGL: sólo en zonas distintas de la pantalla.** `lcd_*` escribe directamente en la
  memoria de imagen del ST7796S y LVGL no se entera: la próxima vez que LVGL repinte una zona
  que considera suya (porque un objeto cambió o se movió), borrará lo que `lcd_*` dibujó allí.
  El bus sí está protegido: `lcd_panel` serializa los volcados de los dos.

## 3. Cómo usarla en un proyecto

1. Copiar esta carpeta (`modulos/lcd/`) y `comun/` dentro de la carpeta `modulos/` del proyecto.
   El proyecto tiene que estar preparado para módulos (ver [../README.md](../README.md),
   apartado 5.4). Se pueden copiar sólo los módulos que se vayan a usar, con sus dependencias:
   `lcd` necesita `lcd_panel`; `lcd_tactil`, también `lcd_panel`; `lcd_lvgl`, `lcd_panel` y
   `lcd_tactil`.
2. Activar en el `prj.conf` de la raíz del proyecto lo que use el programa:

    | Para usar | Línea en `prj.conf` |
    | --- | --- |
    | `lcd_*` | `CONFIG_MCUX_COMPONENT_LCD=y` |
    | `lcd_tactil_*` | `CONFIG_MCUX_COMPONENT_LCD_TACTIL=y` |
    | `lcd_lvgl_*` y LVGL | `CONFIG_MCUX_COMPONENT_LCD_LVGL=y` (incluye el táctil) |

    `lcd_panel`, `comun`, LVGL y los drivers del SDK (FlexIO, ST7796S, DBI, LPI2C, GT911...) se
    activan solos.
3. Reconfigurar (**CMake: Delete Cache and Reconfigure**) y compilar.

No hace falta partir de un ejemplo LVGL del SDK ni quitar su *port*: basta un proyecto de la
placa que compile (estos módulos se han probado sobre `frdmmcxa156_flexio_mculcd_edma_transfer`).
El *port* LVGL del SDK para esta placa (`lvgl_support/mcx_flexio`) sólo entra con el segmento
`module.board.lvgl` de los ejemplos LVGL de NXP; si se parte de uno de ellos, hay que
desactivarlo, porque también maneja el ST7796S y el GT911 y sólo puede haber un dueño.

### Opciones de configuración

Se cambian en `prj.conf` con `CONFIG_<opción>=<valor>`:

| Módulo | Opción | Por defecto | Para qué |
| --- | --- | --- | --- |
| `lcd_panel` | `LCD_PANEL_CON_FREERTOS` | `y` con FreeRTOS, `n` sin él | Mutex del bus. Llega a todo el proyecto como macro (la leen `lcd_panel.c`, `lcd_lvgl_port.c` y `lv_conf.h`); no hay que definirla a mano. |
| `lcd_lvgl` | `LCD_LVGL_PILA_PRINCIPAL` | `0x1800` | Tamaño de la pila de `main()` (ver apartado 4). |
| `lcd_lvgl` | `LCD_LVGL_PORT_SYSTICK_PROPIO` | `1` | Sin FreeRTOS, el *port* usa el SysTick como reloj de LVGL y define `SysTick_Handler()`. Si el programa ya lo usa, poner `0` y llamar a `lcd_lvgl_port_contar_1ms()` cada milisegundo. Con FreeRTOS, el reloj es `xTaskGetTickCount()`. |
| `lcd_lvgl` | `LCD_LVGL_PORT_INTERCAMBIAR_BYTES` | `0` | Intercambia los bytes de cada píxel RGB565 antes de enviarlos. |

## 4. LVGL

- **Configuración (`lv_conf.h`).** En el SDK 26.06, LVGL se configura por Kconfig
  (`CONFIG_LV_CONF_SKIP=y`) y no lee ningún `lv_conf.h`. El módulo `lcd_lvgl` define
  `LV_CONF_PATH` para que LVGL incluya igualmente el suyo (`lcd_lvgl/inc/lv_conf.h`): lo que se
  define ahí manda, y el resto de opciones salen de Kconfig (se pueden cambiar en `prj.conf` con
  `CONFIG_LV_...`). Para cambiar la configuración de la asignatura (fuentes, `LV_MEM_SIZE`...),
  editar ese `lv_conf.h`. Sin esto faltan las fuentes Montserrat 24/32 y UNSCII 16.
- **Pila principal.** Sin RTOS, LVGL dibuja en la pila de `main()`, y los 2 KiB (`0x800`) que
  pone el enlazador por defecto no bastan: se desborda, salta un HardFault y la pantalla se queda
  en negro. El módulo la amplía a `0x1800` (lo mismo que los ejemplos LVGL sin RTOS de NXP para
  esta placa); se cambia con `CONFIG_LCD_LVGL_PILA_PRINCIPAL`.
- **Rutas largas en Windows.** LVGL genera nombres de objeto muy largos: si la carpeta del
  proyecto tiene una ruta larga, se supera el límite de 260 caracteres y la compilación falla con
  `opening dependency file ... No such file or directory`.

## 5. FreeRTOS

Añadir en `prj.conf`, además de los módulos:

```text
CONFIG_MCUX_COMPONENT_middleware.freertos-kernel=y
CONFIG_MCUX_COMPONENT_middleware.freertos-kernel.cm33_non_trustzone=y
CONFIG_MCUX_COMPONENT_middleware.freertos-kernel.heap_4=y
CONFIG_configTOTAL_HEAP_SIZE=24576
```

- `LCD_PANEL_CON_FREERTOS` y `configUSE_RECURSIVE_MUTEXES` (lo exige `lv_lock()`) quedan a 1
  solos.
- El SDK no pone asignador de memoria por defecto: sin `heap_4` fallan `xTaskCreate()` y los
  mutex.
- **Con LVGL, el montón de FreeRTOS tiene que ser grande**: LVGL crea su propia tarea de dibujo
  (`swdraw`) con 8 KiB de pila (`LV_DRAW_THREAD_STACK_SIZE`), que sale de ese montón. Con
  `ejemplo_freertos.c` se usan unos 19 KiB; con 16 KiB no cabía y LVGL funcionaba mal sin avisar.
- Medido con `ejemplo_freertos.c`: la tarea que llama a `lcd_lvgl_procesar()` usa unas 960
  palabras de pila (con 1024 se desbordaba) y la que escribe con `lcd_lvgl_xprintf()`, unas 370.
- La configuración que genera el SDK trae **apagadas** la comprobación de desbordamiento de pila
  y el aviso de falta de memoria. Mientras se desarrolla, conviene activarlas (el programa tiene
  que definir `vApplicationStackOverflowHook()` y `vApplicationMallocFailedHook()`):

    ```text
    CONFIG_configCHECK_FOR_STACK_OVERFLOW=y
    CONFIG_configUSE_MALLOC_FAILED_HOOK=y
    ```

- Con FreeRTOS, `lcd_lvgl_inicializar()` se llama desde una tarea, con el planificador ya en
  marcha.

## 6. Memoria

El *port* reserva 2 búferes de 480 × 20 píxeles × 2 bytes = **37,5 KiB**, y `lv_conf.h` da a
LVGL un montón de **40 KiB** (`LV_MEM_SIZE`). Unos 80 KiB en total de los 120 KiB de SRAM, más
la pila principal de 6 KiB. Con FreeRTOS y un montón de 24 KiB, en la prueba quedaron libres unos
12 KiB. Las cuentas están comentadas en `lcd_lvgl_port.c` y `lv_conf.h`.

## 7. Ejemplos

Cada módulo trae sus ejemplos en `<módulo>/ejemplos/`. No se compilan: para probar uno, se copia
como `main.c` del proyecto y se activan los módulos que dice su cabecera.

| Fichero | Asignatura | Qué muestra | Probado en la placa |
| --- | --- | --- | --- |
| `lcd/ejemplos/ejemplo_lcd.c` | II | `lcd_*` en modo inmediato directo (texto, figuras). | Sí |
| `lcd_tactil/ejemplos/ejemplo_tactil.c` | II / SABEM | `lcd_tactil_leer()` sin LVGL. | Sí |
| `lcd_lvgl/ejemplos/ejemplo_lcd_lvgl.c` | SABEM | Sin RTOS: `lcd_lvgl_xprintf()`, `lcd_lvgl_printf()` y figuras mezclados con un `lv_slider` que se mueve con el táctil; bucle con `lcd_lvgl_procesar()`. | Sí, también el táctil |
| `lcd_lvgl/ejemplos/ejemplo_freertos.c` | SABEM | Con FreeRTOS: una tarea de interfaz llama a `lcd_lvgl_procesar()` y otra escribe un contador con `lcd_lvgl_xprintf()`, protegida por `lv_lock()`; más un `lv_button`. | Sí, también el táctil |

Los cuatro ejemplos están comprobados en la placa, táctil incluido: con `ejemplo_tactil.c` se
dibuja bajo el dedo en toda la pantalla, con `ejemplo_lcd_lvgl.c` el deslizador se mueve con el
dedo y con `ejemplo_freertos.c` el botón cuenta las pulsaciones.

## 8. Cómo se ha comprobado

Sobre el proyecto `frdmmcxa156_flexio_mculcd_edma_transfer`, copiando cada ejemplo como
`main.c`, desde una caché limpia:

| Prueba | Resultado |
| --- | --- |
| `ejemplo_lcd.c`, con `LCD` | Compila (68 256 B de código, 5 888 B de datos) y funciona en la placa. |
| `ejemplo_tactil.c`, con `LCD` y `LCD_TACTIL` | Compila y enlaza; entran solos `fsl_lpi2c.c` y `fsl_gt911.c` (74 104 B / 6 040 B). En la placa, al principio sólo se podía tocar en una zona de 320 × 240: el GT911 de este panel tiene guardada una resolución menor que la de la pantalla y el giro copiado del ejemplo de NXP no escalaba. Resuelto en `lcd_tactil.c` escalando con la resolución que lee `GT911_Init()`; después responde en toda la pantalla, con el punto bajo el dedo. |
| `ejemplo_lcd_lvgl.c`, con `LCD_LVGL` (sin RTOS) | Compila (unos 500 objetos: LVGL completo). Al principio faltaban fuentes: LVGL no leía `lv_conf.h` (resuelto con `LV_CONF_PATH`). En la placa, pantalla negra: HardFault por desbordamiento de la pila de `main()`, localizado con la pila de llamadas del depurador y resuelto con `LCD_LVGL_PILA_PRINCIPAL=0x1800`. Después funciona. |
| `ejemplo_freertos.c`, con `LCD_LVGL` y FreeRTOS | No compilaba: `xSemaphoreCreateMutexStatic()` sin `configSUPPORT_STATIC_ALLOCATION` (resuelto en `lcd_panel.c`). Luego, pantalla negra al activar el aviso de falta de memoria: el montón de 16 KiB no bastaba por la tarea de dibujo de LVGL. Con 24 KiB funciona en la placa; pilas y montón medidos por el puerto serie. `LCD_PANEL_CON_FREERTOS` se activó sola. |
| Táctil con LVGL, después de escalar las coordenadas | Con `ejemplo_lcd_lvgl.c` el deslizador se mueve con el dedo y su valor se actualiza sin detener el contador. Con `ejemplo_freertos.c` (pilas de 1536 y 512 palabras, montón de 24 KiB y las comprobaciones de pila y memoria activadas) el botón cuenta las pulsaciones y el contador sigue avanzando. |
| Agrupación de los cuatro módulos en `modulos/lcd/` | Se registran todos y `ejemplo_lcd.c` compila con el mismo tamaño que antes. |
| Drivers de FlexIO quitados de los `prj.conf` del proyecto | `driver.flexio`, `driver.flexio_mculcd` y `driver.flexio_mculcd_edma` siguen a `y` en `.config` por el `select` de `lcd_panel`, sin avisos de Kconfig y con el mismo tamaño. |

## 9. Origen de la biblioteca y cambios hechos aquí

La biblioteca venía de la carpeta `Comun/pantalla` del repositorio de las asignaturas (con
`tipos.h` en `Trabajo_AGV/firmware_base/inc/`), pensada para copiarse a mano sobre un ejemplo LVGL
del SDK. Al convertirla en módulos se cambió también el código. **Si esa copia original se sigue
usando, hay que llevarle estos cambios:**

| Fichero | Cambio |
| --- | --- |
| `lcd_tactil/src/lcd_tactil.c` | Escala las coordenadas del GT911 con la resolución que tiene guardada (sin esto, en este panel sólo responde una zona de 320 × 240). |
| `lcd_panel/src/lcd_panel.c` | Con FreeRTOS, crea el mutex con `xSemaphoreCreateMutex()` si `configSUPPORT_STATIC_ALLOCATION` vale 0, que es lo que genera el SDK. |
| `lcd_lvgl/inc/lv_conf.h` | Sólo cambian los comentarios: cómo llega a LVGL (`LV_CONF_PATH`) y el presupuesto de memoria. |
| `ejemplos/*.c` | Cabeceras (cómo montar el proyecto), pilas medidas en `ejemplo_freertos.c` y comentarios de sus ganchos. |
| `error.c` / `error.h` | `EA_GLCD` sustituido por `ERROR_EN_LCD` / `ERROR_EN_LCD_LVGL`, y soporte de `lcd_lvgl`. |

`LCD_PANEL_CON_FREERTOS`, `LV_CONF_PATH` y el tamaño de la pila principal ya no se definen a mano:
los pone el Kconfig de los módulos.

## 10. Pendiente

- `lcd_desplazar()` borra la pantalla en lugar de desplazarla, porque se supuso que el
  desplazamiento por *hardware* del ST7796S mueve la imagen de lado con la orientación apaisada.
  No se ha comprobado en la placa (ver el comentario en `lcd/inc/lcd.h`).
- El tamaño de las variables globales de LVGL (1–2 KiB en el presupuesto de `lv_conf.h`) es una
  estimación: no se ha medido en el `.map`.
