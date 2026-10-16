# Mensajes de commit

Escribe el mensaje de commit en español con este formato en la primera línea:

`:emoji:tipo(scope): descripción`

- Máximo 72 caracteres, en minúsculas, en imperativo, sin punto final y sin tildes ni ñ.
- Tipos y emojis: `:star:nuevo` (funcionalidad nueva), `:bug:fix` (corrección de un fallo), `:memo:docs` (solo documentación), `:construction:reestructura` (renombrar, mover o reindentar sin cambiar el comportamiento), `:crossed_fingers:test` (programa que solo comprueba algo), `:wrench:tarea` (compilación, configuración, ficheros de proyecto).
- El scope es el módulo afectado (`sensor`, `fsm`, `motor`...). Omítelo si el cambio no es de un módulo concreto.
- Describe solo lo que hay en staging. No uses `:bookmark:hito`.
- Añade un cuerpo tras una línea en blanco solo si el porqué no es obvio.
- Termina siempre con una línea en blanco y esta línea final: `Assisted-by: GitHub Copilot (<modelo>)`. Sustituye `<modelo>` por el modelo que estás usando si lo conoces; si no lo sabes, escribe solo `Assisted-by: GitHub Copilot`. No inventes el modelo.

Ejemplo:

```text
:star:nuevo(sensor): anadir lectura simulada del sensor de guiado

Assisted-by: GitHub Copilot (GPT-5)
```
