/*
 * Copyright 2024 NXP
 * Modificado para SABEM (Universidad de Cádiz), 2026.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file    app.h
 * @brief   Prototipo de la inicialización de la placa (hardware_init.c).
 */
#ifndef APP_H_
#define APP_H_

/**
 * @brief   Inicializa pines, relojes, periféricos y consola de depuración con lo configurado en
 * MCUXpresso Config Tools. Se llama una vez, al principio de main().
 */
void BOARD_InitHardware(void);

#endif /* APP_H_ */
