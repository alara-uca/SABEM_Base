# Estilo de C de la asignatura

Resumen de las normas de la lección **L1.1** (partes C y D). Si algo de aquí contradice la lección,
manda la lección. Las herramientas que lo comprueban son `.editorconfig`, `.clang-format` y
`.clang-tidy` (raíz del repositorio): no las modifiquéis.

## Reglas generales

- C estándar (GNU11 o GNU17) y tipos de `<stdint.h>` (`uint8_t`, `int32_t`…).
- Variables y parámetros con tipo completo. Funciones sin argumentos: `void f(void)`, nunca `f()`.
- Las expresiones de un `#define` van entre paréntesis.
- Todo lo que no sea público, **`static`**. Nunca `static` en un `.h`.
- Nombres, comentarios y mensajes **en español**, sin tildes ni eñes en los identificadores.

## Nomenclatura

| Elemento | Convención | Ejemplo |
|---|---|---|
| Ficheros | `snake_case.c` / `.h` | `servo_pwm.c` |
| Funciones públicas | `prefijo_verbo_complemento()` | `motor_dc_ajustar_velocidad()` |
| Variables locales y parámetros | `snake_case` descriptivo | `porcentaje_velocidad` |
| Variables globales (también `static` de fichero) | `g_` + `snake_case` | `g_contador_pulsos` |
| Parámetros que son punteros | `ptr_` + `snake_case` | `ptr_buffer` |
| Macros | `MAYUSCULAS_CON_GUION_BAJO` | `MAX_CONEXIONES` |
| Tipos | `snake_case` + `_t` | `pieza_tipo_t` |
| Constantes (enum, `const`) | `k` + `CamelCase` | `kPiezaCuadrado` |

```c
#define MAX_CONEXIONES (100u)

int32_t g_numero_pulsos = 0;
void tec4x4_inicializar(uint8_t tipo);
void uart_transmitir(const uint8_t *ptr_datos, uint32_t num_bytes);

typedef enum {
  kTec4x4Pulsado,
  kTec4x4Liberado,
} tec4x4_estado_t;

typedef struct {
  uint8_t hora;
  uint8_t minuto;
} hora_t;
```

- **Constantes: `enum`, no `#define`**, salvo si tiene que verlas el preprocesador.
- **Prefijo único por módulo** en todo símbolo público (macros, variables, tipos y funciones).
  Sirve de *namespace*: `tec4x4_`, `motor_dc_`, `encoder_`…
- Las macros siguen en mayúsculas (`MAX_CONEXIONES`); la `k` distingue las constantes de ellas.

## Formato

Lo aplica el editor con `.editorconfig` y `.clang-format`; no lo hagáis a mano.

- **2 espacios**, nunca tabuladores. **100 caracteres** por línea. Una sentencia por línea.
- Llaves estilo K&R (la de apertura en la misma línea), **siempre**, incluso con una sola
  sentencia.
- Asterisco junto al nombre: `int16_t *ptr;`.
- Continuación de línea alineada con el primer parámetro; si no cabe, a 4 espacios.
- `case` indentado dentro del `switch`.
- Una línea en blanco entre bloques lógicos; dos entre funciones.
- Bucle infinito: `while (1)`.

```c
void funcion_ejemplo(uint32_t parametro) {
  if (parametro > 0u) {
    // código
  } else {
    // código
  }
}
```

### Espaciado horizontal

```c
int32_t resultado = (a + b) * c;  // espacio en operadores binarios; ninguno dentro de ()
v = w*x + y/z;                    // se pueden comprimir * y /, sin mezclar en la expresión
mi_funcion(arg1, arg2);           // sin espacio antes de '(' en una llamada; sí tras la coma
for (i = 0; i < 5; i++) {         // con espacio tras for/if/while/switch y tras cada ';'
```

Comentario al final de línea: dos espacios antes y uno tras `//`.

### Separadores de sección

```c
// ===== Servo_PWM - Constantes Privadas =====
// ===== Servo_PWM - Variables Privadas =====
// ===== Servo_PWM - Funciones Privadas =====
// ===== Servo_PWM - Funciones Publicas =====
// ===== Servo_PWM - Rutina de Servicio de Interrupcion =====
```

## Orden de los `#include`

1. El `.h` propio del `.c` (garantiza que la cabecera es autosuficiente).
2. Cabecera global del microcontrolador y HAL (`fsl_device_registers.h`, `fsl_<periférico>.h`).
3. Librería estándar de C.
4. Cabeceras propias, **de la más general a la más específica**.

`clang-format` no reordena los `#include`: el orden es responsabilidad de quien escribe.

## Estructuras y macros

Inicializad `struct` y `union` con inicializadores designados:

```c
pieza_t mi_pieza = {
  .tipo   = kPiezaPalo,
  .tamano = 4,
  .color  = kColorRojo,
};
```

Evitad las macros con argumentos. Si son inevitables: cada argumento entre paréntesis, sin `;`
final; `do { ... } while (0)` si no devuelven valor.

```c
#define ASSERT(expr, msg) \
do { \
  if (!(expr)) { \
    parar_con_error(__FILE__, __LINE__, msg); \
  } \
} while (0)
```

## Trampas habituales

1. **Nunca tabuladores**: configurad el editor a 2 espacios.
2. **El orden de los `#include` importa** (ver arriba).
3. Un fichero que ya sigue Git no deja de seguirse por añadirlo al `.gitignore`.
