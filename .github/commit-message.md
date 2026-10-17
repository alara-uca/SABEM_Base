# Mensajes de commit

Escribe el mensaje de commit en español con este formato en la primera línea:

`:emoji:tipo(scope): descripción`

- Máximo 72 caracteres, en minúsculas, en imperativo, sin punto final y sin tildes ni ñ.
- Tipos y emojis: `:star:nuevo` (funcionalidad nueva), `:bug:fix` (corrección de un fallo), `:memo:docs` (solo documentación), `:construction:reestructura` (renombrar, mover o reindentar sin cambiar el comportamiento), `:crossed_fingers:test` (programa que solo comprueba algo), `:wrench:tarea` (compilación, configuración, ficheros de proyecto).
- El scope es el módulo afectado (`sensor`, `fsm`, `motor`...). Omítelo si el cambio no es de un módulo concreto.
- Elige el tipo por lo que el cambio hace al programa, no por el nombre del fichero. `tarea` es solo para ficheros que no son código ni documentación (`Makefile`, `.gitignore`, configuración) y nunca para ficheros `.c` o `.h`.
- Añadir un caso o una rama que faltaba (por ejemplo un `default`) o corregir un valor es `fix`. Una función o un módulo que no existía es `nuevo`.
- Describe solo lo que el diff añade o quita realmente. No inventes cambios que no aparezcan en él. Incluye siempre el scope del módulo (por ejemplo `fsm`) cuando el cambio sea de un módulo.
- No uses `:bookmark:hito`.
- Añade un cuerpo tras una línea en blanco solo si el porqué no es obvio.
- Termina siempre con una línea en blanco y esta línea final: `Assisted-by: GitHub Copilot (<modelo>)`. Sustituye `<modelo>` por el modelo que estás usando si lo conoces; si no lo sabes, escribe solo `Assisted-by: GitHub Copilot`. No inventes el modelo.

Ejemplos:

```text
:star:nuevo(sensor): anadir lectura simulada del sensor de guiado

Assisted-by: GitHub Copilot (GPT-5)
```

```text
:bug:fix(fsm): devolver 0 en los estados no previstos de fsm_step

Assisted-by: GitHub Copilot
```
