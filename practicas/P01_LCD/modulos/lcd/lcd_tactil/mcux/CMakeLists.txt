# Módulo lcd_tactil. lcd_panel y los drivers del SDK los activa su Kconfig con select.
# Las rutas se toman desde la carpeta del módulo (la padre de mcux/).
if(CONFIG_MCUX_COMPONENT_LCD_TACTIL)
    mcux_add_source(
        BASE_PATH ${CMAKE_CURRENT_LIST_DIR}/..
        SOURCES src/lcd_tactil.c
                inc/lcd_tactil.h
    )
    mcux_add_include(
        BASE_PATH ${CMAKE_CURRENT_LIST_DIR}/..
        INCLUDES inc
    )
endif()
