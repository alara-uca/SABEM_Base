# Documentación Doxygen

- `//` para comentarios no-documentación
- Variables entre backticks: `` `ptr` nunca puede ser NULL ``
- `/** ... */` para bloques de documentación

## Cabecera de archivo (va en `.c` y `.h`)

```c
/**
 * @file    mi_modulo.c
 * @brief   Descripción breve del archivo.
 *
 * @author  Nombre [email@uca.es]
 * @date    yyyy-mm-dd
 * @version v1.0
 *
 * @copyright GNU General Public License version 3 or later
 *
 * @note    Dependencias de hardware o configuración requerida.
 */
```

## Cabecera de función (va en `.h`, junto a la declaración)

```c
/**
 * @brief   Lee el sensor y devuelve el valor calibrado.
 *
 * @param[in]     sensor        Puntero al sensor. No puede ser NULL.
 * @param[out]    output_value  Resultado calibrado almacenado aquí.
 *
 * @retval  0   Lectura exitosa.
 * @retval -1   Puntero NULL.
 * @retval -2   Timeout del sensor.
 *
 * @note    Puede tardar hasta 5 ms.
 */
int32_t sensor_leer_calibrado(const sensor_t *sensor, float32_t *output_value);
```

## Macro

```c
/**
 * @def     CONEXIONES_MAX
 * @brief   Número máximo de conexiones simultáneas.
 */
#define CONEXIONES_MAX 16

#define PANTALLA_ANCHO_PX 480u  //!< Píxeles horizontales de la pantalla

/**
 * @brief   Calcula el cuadrado de un número.
 * @param[in]  x  Número a elevar al cuadrado.
 * @return  Valor de x * x.
 * @warning El argumento `x` se evalúa dos veces. No usar `SQUARE(i++)`.
 */
#define SQUARE(x) ((x) * (x))
```

## Typedef

```c
/**
 * @brief   Identificador único de usuario.
 */
typedef uint32_t user_id_t;
```

## Enum

```c
/**
 * @brief   Códigos de estado para operaciones de dispositivo.
 */
typedef enum {
  DEVICE_OK      = 0,  //!< Operación completada con éxito
  DEVICE_BUSY,         //!< Dispositivo ocupado
  DEVICE_ERROR,        //!< Error no especificado
  DEVICE_TIMEOUT,      /*!< La operación no finalizó en el tiempo
                            esperado */
} device_status_t;
```

## Struct

```c
/**
 * @brief   Configuración de un puerto serie (UART).
 */
typedef struct {
  uint32_t baud_rate;      //!< Velocidad en baudios
  uint8_t  data_bits;      //!< Número de bits de datos
  bool_t   enable_parity;  //!< Habilita el bit de paridad
} uart_config_t;
```

## Referencia de etiquetas

| Etiqueta | Uso |
| --- | --- |
| `@brief` | Resumen de una línea (verbo en 3ª persona) |
| `@details` | Descripción larga: algoritmo, limitaciones, referencias |
| `@param[in]` | Parámetro de entrada |
| `@param[out]` | Parámetro de salida (puntero modificado por la función) |
| `@param[in,out]` | Parámetro que se lee y también se modifica |
| `@return` | Descripción del valor de retorno |
| `@retval` | Un código de retorno concreto (preferido para errores) |
| `@note` | Nota importante (rendimiento, thread-safety, plataforma) |
| `@warning` | Advertencia crítica (efectos secundarios, mal uso) |
| `@pre` | Precondición que debe cumplirse antes de llamar |
| `@post` | Estado del sistema tras la ejecución exitosa |
| `@todo` | Tarea pendiente (Doxygen genera lista global) |
| `@see` / `@ref` | Referencia cruzada — usar `#nombre_funcion` |
