# Módulo lcd. La capa baja (lcd_panel) y sus drivers los activa su Kconfig con select.
# Las rutas se toman desde la carpeta del módulo (la padre de mcux/).
if(CONFIG_MCUX_COMPONENT_LCD)
    mcux_add_source(
        BASE_PATH ${CMAKE_CURRENT_LIST_DIR}/..
        SOURCES src/lcd.c
                src/fuente.c
                inc/lcd.h
                inc/lcd_colores.h
                inc/fuente.h
    )
    mcux_add_include(
        BASE_PATH ${CMAKE_CURRENT_LIST_DIR}/..
        INCLUDES inc
    )
endif()
