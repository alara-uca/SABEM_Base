# Módulo lcd_panel. Los drivers del SDK que necesita los activa su Kconfig con select.
# Las rutas se toman desde la carpeta del módulo (la padre de mcux/).
if(CONFIG_MCUX_COMPONENT_LCD_PANEL)
    mcux_add_source(
        BASE_PATH ${CMAKE_CURRENT_LIST_DIR}/..
        SOURCES src/lcd_panel.c
                inc/lcd_panel.h
    )
    mcux_add_include(
        BASE_PATH ${CMAKE_CURRENT_LIST_DIR}/..
        INCLUDES inc
    )
endif()
