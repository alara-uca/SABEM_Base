# Módulo `comun`

Tipos y macros comunes a todos los módulos: `tipos.h` (`bool_t`, `TRUE`/`FALSE`, `float32_t`,
`MIN`, `MAX`, `CONSTRAIN`, `MAP`...).

- **Activación:** `CONFIG_MCUX_COMPONENT_COMUN=y` en el `prj.conf` de **todos** los proyectos, aunque
  ningún otro módulo lo pida (los que lo usan también lo activan solos con `select`).
- **Depende de:** `driver.common` del SDK.
- **Macros de compilación de la consola** (`debug_console_lite`, LPUART0):
  `PRINTF_FLOAT_ENABLE=1` y `PRINTF_ADVANCED_ENABLE=1`. Con ellas, `PRINTF("%f")` y las banderas
  (`%-5d`, `%05d`, `%+d`) funcionan. Cuestan unos 9-10 KB de Flash y unos 0,4 KB de RAM (medido).
  **Las dos van siempre juntas**: solo con `FLOAT` los `float` negativos se imprimen **sin signo**
  (`-0.5f` sale como `0.500000`), y solo con `ADVANCED` `%f` sale vacío. Si se cambia una, se
  revisa la otra. Van en **una sola** palabra `CC` de `mcux_add_macro`: si se repite `CC`, solo
  llega la última. Esta es la razón de que `comun` se active siempre y de que el alumnado no tenga
  que tocar ningún `CMakeLists.txt`.
  Probadas en `Practicas/P03_PWM_2627` (comprobado en `compile_commands.json` y en la placa).
- **Ejemplos:** ninguno.

Detalles: comentarios de `inc/tipos.h`. Cómo funcionan los módulos: [../README.md](../README.md).
