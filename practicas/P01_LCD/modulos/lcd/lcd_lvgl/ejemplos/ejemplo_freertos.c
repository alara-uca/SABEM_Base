/**
 * @file    ejemplo_freertos.c
 * @brief   Ejemplo de lcd_lvgl_* con FreeRTOS: una tarea de interfaz que mueve LVGL y otra tarea
 * que escribe en la pantalla con lcd_lvgl_xprintf() sin preocuparse de la primera.
 *
 * @details
 *  - tarea_interfaz: inicializa la pantalla y LVGL, crea un botón y llama periódicamente a
 *    lcd_lvgl_procesar(), que repinta, lee el táctil y atiende los eventos. Es la única tarea
 *    que "mueve" LVGL.
 *  - tarea_contador: cada 500 ms escribe un contador con lcd_lvgl_xprintf(). Puede hacerlo
 *    aunque la otra tarea esté a mitad de un repintado porque cada función lcd_lvgl_* toma
 *    lv_lock(), el mutex (recursivo) de LVGL: si LVGL está ocupado, la tarea espera.
 *  Lo que escribe tarea_contador aparece en pantalla en el siguiente lcd_lvgl_procesar() de
 *  tarea_interfaz: como mucho unos 30 ms después.
 *
 * LCD_PANEL_CON_FREERTOS debe valer 1 en TODO el proyecto (lcd_panel.c, lcd_lvgl_port.c, las
 * fuentes de LVGL a través de lv_conf.h...). Con los módulos no hay que hacer nada: el Kconfig de
 * lcd_panel la activa sola si el proyecto usa FreeRTOS. configUSE_RECURSIVE_MUTEXES (lo exige
 * lv_lock()) ya vale 1 en la configuración de FreeRTOS que genera el SDK.
 *
 * Proyecto: copiarlo como main.c de un proyecto con la carpeta modulos/ y activar en prj.conf el
 * módulo y FreeRTOS (ver modulos/lcd/README.md, apartado «FreeRTOS»):
 *     CONFIG_MCUX_COMPONENT_LCD_LVGL=y
 *     CONFIG_MCUX_COMPONENT_middleware.freertos-kernel=y
 *     CONFIG_MCUX_COMPONENT_middleware.freertos-kernel.cm33_non_trustzone=y
 *     CONFIG_MCUX_COMPONENT_middleware.freertos-kernel.heap_4=y
 *     CONFIG_configTOTAL_HEAP_SIZE=24576
 * Probado en la placa con frdmmcxa156_flexio_mculcd_edma_transfer, táctil incluido.
 *
 * @author      Alejandro Lara Doña - alejandro.lara@uca.es
 * @date        2026
 * @version     1.0
 *
 * @copyright   GNU General Public License version 3 or later
 */

#ifndef LCD_PANEL_CON_FREERTOS
#define LCD_PANEL_CON_FREERTOS  1  // Ver la nota de la cabecera: debe definirse globalmente.
#endif
#if !LCD_PANEL_CON_FREERTOS
#error "ejemplo_freertos.c necesita LCD_PANEL_CON_FREERTOS a 1 en todo el proyecto."
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "board.h"
#include "tipos.h"
#include "lvgl.h"
#include "lcd_lvgl.h"

// ===== EJEMPLO - Macros Privadas =====
/** Pilas en palabras de 32 bits. Medidas con uxTaskGetStackHighWaterMark(): tarea_interfaz usa
 * unas 960 palabras (con 1024 se desbordaba) y tarea_contador unas 370. Salen del montón de
 * FreeRTOS, igual que los 8 KiB de pila de la tarea de dibujo que crea LVGL ("swdraw",
 * LV_DRAW_THREAD_STACK_SIZE): en total se usan unos 19 KiB, de ahí configTOTAL_HEAP_SIZE=24576. */
#define EJEMPLO_PILA_INTERFAZ   1536u
#define EJEMPLO_PILA_CONTADOR   512u
#define EJEMPLO_PERIODO_MAX_MS  30u   //!< Espera máxima entre dos lcd_lvgl_procesar().

// ===== EJEMPLO - Prototipos Externos =====
/** Inicialización de relojes y pines de la placa: hardware_init.c del proyecto,
 * declarada en su app.h. */
void BOARD_InitHardware(void);

// ===== EJEMPLO - Variables Privadas =====
static uint32_t pulsaciones = 0;  //!< Veces que se ha pulsado el botón (sólo tarea_interfaz).

// ===== EJEMPLO - Funciones Privadas =====
/**
 * @brief   Evento LV_EVENT_CLICKED del botón. Se ejecuta en tarea_interfaz, dentro de
 * lcd_lvgl_procesar().
 */
static void ejemplo_al_pulsar_boton(lv_event_t *evento) {
  (void) evento;
  pulsaciones++;
  lcd_lvgl_xprintf(10, 150, AMARILLO, NEGRO, FUENTE12X24, "Pulsaciones: %lu",
                   (unsigned long) pulsaciones);
}

/**
 * @brief   Tarea que escribe un contador cada 500 ms con lcd_lvgl_xprintf().
 */
static void tarea_contador(void *parametros) {
  uint32_t contador = 0;
  TickType_t instante = xTaskGetTickCount();

  (void) parametros;
  for (;;) {
    contador++;
    lcd_lvgl_xprintf(10, 100, BLANCO, AZUL_OSCURO, FUENTE12X24, "Contador: %lu",
                     (unsigned long) contador);
    vTaskDelayUntil(&instante, pdMS_TO_TICKS(500));
  }
}

/**
 * @brief   Tarea de interfaz: inicializa la pantalla y LVGL y los atiende periódicamente.
 */
static void tarea_interfaz(void *parametros) {
  lv_obj_t *boton;
  lv_obj_t *texto_boton;
  uint32_t espera;

  (void) parametros;

  // Con FreeRTOS, la inicialización se hace ya dentro de una tarea (usa el mutex del bus).
  lcd_lvgl_inicializar();
  lcd_lvgl_xprintf(10, 10, BLANCO, NEGRO, FUENTE16X32, "SABM - FreeRTOS + LVGL");
  lcd_lvgl_linea(10, 50, 470, 50, GRIS_CLARO);

  // Widget nativo: un botón con su texto.
  lv_lock();
  boton = lv_button_create(lv_screen_active());
  lv_obj_set_size(boton, 160, 60);
  lv_obj_align(boton, LV_ALIGN_BOTTOM_MID, 0, -30);
  lv_obj_add_event_cb(boton, ejemplo_al_pulsar_boton, LV_EVENT_CLICKED, NULL);
  texto_boton = lv_label_create(boton);
  lv_label_set_text(texto_boton, "Pulsa");
  lv_obj_center(texto_boton);
  lv_unlock();

  // La otra tarea se crea cuando LVGL ya está listo para recibir llamadas.
  (void) xTaskCreate(tarea_contador, "contador", EJEMPLO_PILA_CONTADOR, NULL,
                     tskIDLE_PRIORITY + 2u, NULL);

  for (;;) {
    espera = lcd_lvgl_procesar();
    if (espera > EJEMPLO_PERIODO_MAX_MS) {
      espera = EJEMPLO_PERIODO_MAX_MS;
    }
    vTaskDelay(pdMS_TO_TICKS(espera) + 1u);  // +1: ceder siempre la CPU al menos un tick.
  }
}

// ===== EJEMPLO - Programa Principal =====
int main(void) {
  BOARD_InitHardware();

  (void) xTaskCreate(tarea_interfaz, "interfaz", EJEMPLO_PILA_INTERFAZ, NULL,
                     tskIDLE_PRIORITY + 1u, NULL);
  vTaskStartScheduler();

  for (;;) {
    // Sólo se llega aquí si no hay memoria para la tarea inactiva.
  }
}

// ===== EJEMPLO - Ganchos de FreeRTOS =====
/**
 * @brief   Desbordamiento de pila. Sólo se llama si prj.conf activa
 * CONFIG_configCHECK_FOR_STACK_OVERFLOW=y (la configuración del SDK la trae apagada).
 */
void vApplicationStackOverflowHook(TaskHandle_t tarea, char *nombre) {
  (void) tarea;
  (void) nombre;
  for (;;) {
  }
}

/**
 * @brief   Sin memoria en el montón de FreeRTOS. Sólo se llama si prj.conf activa
 * CONFIG_configUSE_MALLOC_FAILED_HOOK=y (la configuración del SDK la trae apagada).
 */
void vApplicationMallocFailedHook(void) {
  for (;;) {
  }
}
