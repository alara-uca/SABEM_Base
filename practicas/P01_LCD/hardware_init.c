/*
 * Copyright 2024 NXP
 * Modificado para SABEM (Universidad de Cádiz), 2026.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file    hardware_init.c
 * @brief   Inicialización de la placa: aplica lo que se configura en MCUXpresso Config Tools.
 *
 * @details Las funciones BOARD_InitBoot*() las genera Config Tools en board/ a partir del .mex
 * del proyecto: no se escriben a mano. Cada una llama a las funciones de su herramienta que estén
 * marcadas como «Called from default initialization function».
 *
 * Los módulos de modulos/ (por ejemplo, la LCD) configuran ellos mismos los pines, el reloj y el
 * periférico que usan, así que no aparecen aquí: si un proyecto no usa un módulo, no se inicializa
 * nada de él.
 */
#include "app.h"
#include "board.h"
#include "clock_config.h"
#include "peripherals.h"
#include "pin_mux.h"

void BOARD_InitHardware(void) {

  BOARD_InitBootPins();         // Herramienta Pins.
  BOARD_InitBootClocks();       // Herramienta Clocks: la configuración marcada por defecto.
  BOARD_InitBootPeripherals();  // Herramienta Peripherals.
  BOARD_InitDebugConsole();     // Consola de depuración (PRINTF) por el LPUART0; está en board.c.

  /* El LPUART0 de la consola no se configura en Peripherals: ya lo inicializa
   * BOARD_InitDebugConsole(), y hacerlo en los dos sitios lo dejaría con dos configuraciones.
   */
}
