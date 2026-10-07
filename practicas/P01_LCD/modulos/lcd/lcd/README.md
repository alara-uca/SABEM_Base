# Módulo `lcd`

Pantalla en **modo inmediato directo**, con las mismas funciones que la antigua `glcd_*`
(prefijo `lcd_`): texto con tres fuentes (8×16, 12×24, 16×32), `lcd_printf()`/`lcd_xprintf()`,
puntos, líneas, rectángulos y círculos. Cada llamada pinta en el acto en la memoria del ST7796S.

- **Activación:** `CONFIG_MCUX_COMPONENT_LCD=y` en `prj.conf`.
- **Depende de:** `comun` y `lcd_panel` (se activan solos; sus carpetas tienen que estar
  copiadas).
- **Uso:** `lcd_inicializar()` una vez y después cualquier función `lcd_*`.
- **Ejemplo:** `ejemplos/ejemplo_lcd.c` (con `LCD`).

Si se mezcla con LVGL, sólo en zonas distintas de la pantalla. Detalles: comentarios de
`inc/lcd.h`. Biblioteca completa: [../README.md](../README.md).
