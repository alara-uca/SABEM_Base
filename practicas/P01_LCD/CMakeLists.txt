
# Copyright 2026 NXP
#
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.22.0)

cmake_path(APPEND conf_file_path ${CMAKE_CURRENT_LIST_DIR} ${board} prj.conf)
set(CONF_FILE ${conf_file_path} ${CONF_FILE})

# Módulos de la asignatura: cada carpeta de modulos/ con mcux/module.yml, a cualquier profundidad
# (p. ej. modulos/lcd/lcd_panel), se registra como módulo externo del SDK
# (develop/sdk/integration.md). Su Kconfig entra solo en el menú "External Modules" y sólo se
# compila si se activa en prj.conf (CONFIG_MCUX_COMPONENT_<NOMBRE>=y).
# Tiene que ir antes de project(), que es donde el SDK procesa EXTRA_MCUX_MODULES.
file(GLOB_RECURSE modulos_yml CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/modulos/module.yml)
foreach(modulo_yml ${modulos_yml})
    cmake_path(GET modulo_yml PARENT_PATH carpeta_mcux)     # .../<módulo>/mcux
    cmake_path(GET carpeta_mcux FILENAME nombre_carpeta)
    if(nombre_carpeta STREQUAL "mcux")
        cmake_path(GET carpeta_mcux PARENT_PATH modulo)     # .../<módulo>
        list(APPEND EXTRA_MCUX_MODULES ${modulo})
    endif()
endforeach()

include(${SdkRootDirPath}/cmake/extension/mcux.cmake)

project(P01_LCD LANGUAGES C CXX ASM PROJECT_BOARD_PORT_PATH ${board_root}/${board}/driver_examples/flexio/mculcd/edma_transfer)

include(${SdkRootDirPath}/CMakeLists.txt)

include(${board}/board_files.cmake)

# Carpeta con el .mex de MCUXpresso Config Tools (sin ella, Config Tools no reconoce el proyecto West).
# Está en la raíz para que Config Tools genere pin_mux, clock_config y peripherals en board/.
mcux_add_config_mex_path(PATH .)

mcux_add_source(
    SOURCES main.c
            hardware_init.c
            app.h
)

mcux_add_macro(
    CC "-DBOARD_LCD_S035=1"
)

mcux_convert_binary(BINARY ${APPLICATION_BINARY_DIR}/${MCUX_SDK_PROJECT_NAME}.bin)

mcux_add_include(INCLUDES .)
