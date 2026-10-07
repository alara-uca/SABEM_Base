# Módulo `lcd_panel`

Capa más baja de la pantalla LCD-PAR-S035: inicializa el FlexIO (bus 8080) y el ST7796S,
selecciona ventanas y vuelca píxeles (`lcd_panel_ventana()`, `lcd_panel_escribir_pixeles()` y
su versión asíncrona). Es la única dueña del bus; **el programa no la usa directamente**.

- **Activación:** no hace falta. La activan `lcd`, `lcd_tactil` y `lcd_lvgl` con `select`
  (`MCUX_COMPONENT_LCD_PANEL`).
- **Depende de:** `comun`; drivers del SDK `flexio`, `flexio_mculcd`, `flexio_mculcd_edma`,
  `st7796s`, `dbi_flexio_edma`, `port`, `gpio`, `clock` y `reset`, que activa ella misma.
- **Opción:** `LCD_PANEL_CON_FREERTOS`, que se activa sola si el proyecto usa FreeRTOS y protege
  el bus con un mutex.
- **Ejemplos:** ninguno (se usa a través de los otros módulos).

Detalles: comentarios de `src/lcd_panel.c`. Biblioteca completa: [../README.md](../README.md).
