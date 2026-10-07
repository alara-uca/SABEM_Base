# Módulo lcd_lvgl. LVGL, lcd_panel, lcd_tactil y sus drivers los activa su Kconfig con select.
# Las rutas se toman desde la carpeta del módulo (la padre de mcux/).
#
# lv_conf.h: LVGL lo incluye gracias a LV_CONF_PATH (ver el Kconfig del módulo). La plantilla del
# SDK (middleware.lvgl.template) trae otro lv_conf.h, pero este módulo se carga antes y su ruta de
# include va delante, así que se usa el de aquí.
if(CONFIG_MCUX_COMPONENT_LCD_LVGL)
    mcux_add_source(
        BASE_PATH ${CMAKE_CURRENT_LIST_DIR}/..
        SOURCES src/lcd_lvgl.c
                src/lcd_lvgl_port.c
                inc/lcd_lvgl.h
                inc/lcd_lvgl_port.h
                inc/lv_conf.h
    )
    mcux_add_include(
        BASE_PATH ${CMAKE_CURRENT_LIST_DIR}/..
        INCLUDES inc
    )

    # Pila principal (ver LCD_LVGL_PILA_PRINCIPAL en el Kconfig), igual que los ejemplos LVGL de
    # NXP para esta placa. El enlazador toma __stack_size__ (MCXA156_flash.ld); si no, 0x800.
    mcux_add_armgcc_configuration(
        LD "-Xlinker --defsym=__stack_size__=${CONFIG_LCD_LVGL_PILA_PRINCIPAL}"
    )
    mcux_add_iar_configuration(
        LD "--config_def=__stack_size__=${CONFIG_LCD_LVGL_PILA_PRINCIPAL}"
    )
    mcux_add_mdk_configuration(
        LD "--predefine=\"-D__stack_size__=${CONFIG_LCD_LVGL_PILA_PRINCIPAL}\""
    )
endif()
