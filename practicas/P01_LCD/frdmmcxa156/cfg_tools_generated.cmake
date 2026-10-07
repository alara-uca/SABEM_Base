# Lo rellena cfg_resolve (MCUXpresso for VS Code) con los ficheros que genera Config Tools en ../board (relativo a esta carpeta)

# Added by cfg_resolve west command
mcux_add_include(
	BASE_PATH ${CMAKE_CURRENT_LIST_DIR}
	INCLUDES ../board
)

# Added by cfg_resolve west command
mcux_add_source(
	BASE_PATH ${CMAKE_CURRENT_LIST_DIR}
	SOURCES ../board/clock_config.c
			../board/clock_config.h
			../board/peripherals.c
			../board/peripherals.h
			../board/pin_mux.c
			../board/pin_mux.h
)
