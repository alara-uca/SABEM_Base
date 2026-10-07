# Módulo `lcd_lvgl`

Pantalla sobre **LVGL 9.4**: las funciones de `lcd` con prefijo `lcd_lvgl_`, pero cada llamada
crea o actualiza un objeto de LVGL, así que se mezclan con sus widgets (`lv_button`,
`lv_slider`...). Incluye el *port* de LVGL (display con búferes parciales y táctil) y su
`lv_conf.h`.

- **Activación:** `CONFIG_MCUX_COMPONENT_LCD_LVGL=y` en `prj.conf`.
- **Depende de:** `comun`, `lcd_panel` y `lcd_tactil`, y LVGL del SDK (se activan solos).
- **Uso:** `lcd_lvgl_inicializar()` una vez y `lcd_lvgl_procesar()` periódicamente: sin él no
  se ve nada ni se lee el táctil.
- **Opciones:** `LCD_LVGL_PILA_PRINCIPAL` (pila de `main()`, `0x1800`),
  `LCD_LVGL_PORT_SYSTICK_PROPIO` y `LCD_LVGL_PORT_INTERCAMBIAR_BYTES`.
- **Ejemplos:** `ejemplos/ejemplo_lcd_lvgl.c` (sin RTOS) y `ejemplos/ejemplo_freertos.c` (con
  FreeRTOS; necesita las líneas de FreeRTOS del README de la biblioteca).

Con FreeRTOS, el montón de FreeRTOS tiene que ser de 24 KiB. Detalles: comentarios de
`inc/lcd_lvgl.h` y `inc/lv_conf.h`. LVGL, FreeRTOS y memoria: [../README.md](../README.md).
