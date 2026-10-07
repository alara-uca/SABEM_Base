# Módulo comun: sólo cabeceras. Las rutas se toman desde la carpeta del módulo (la padre de mcux/).
if(CONFIG_MCUX_COMPONENT_COMUN)
    mcux_add_source(
        BASE_PATH ${CMAKE_CURRENT_LIST_DIR}/..
        SOURCES inc/tipos.h
    )
    mcux_add_include(
        BASE_PATH ${CMAKE_CURRENT_LIST_DIR}/..
        INCLUDES inc
    )
endif()
