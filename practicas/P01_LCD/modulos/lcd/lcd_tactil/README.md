# Módulo `lcd_tactil`

Panel táctil GT911 de la pantalla (bus LPI2C2) en modo inmediato: `lcd_tactil_leer(&x, &y)`
devuelve si hay un dedo y dónde, en coordenadas de la pantalla (480 × 320), y
`lcd_tactil_pulsado()` sólo si hay un dedo. También es el que usa `lcd_lvgl` para LVGL.

- **Activación:** `CONFIG_MCUX_COMPONENT_LCD_TACTIL=y` en `prj.conf` (con `lcd_lvgl` se activa
  solo).
- **Depende de:** `comun` y `lcd_panel`; drivers del SDK `lpi2c` y `gt911`, que activa él mismo.
- **Uso:** `lcd_tactil_inicializar()` (devuelve `FALSE` si el GT911 no responde) y después
  `lcd_tactil_leer()` periódicamente.
- **Ejemplo:** `ejemplos/ejemplo_tactil.c` (con `LCD` y `LCD_TACTIL`).

Las coordenadas se giran y se escalan con la resolución que tiene guardada el GT911. Detalles:
comentarios de `src/lcd_tactil.c`. Biblioteca completa: [../README.md](../README.md).
