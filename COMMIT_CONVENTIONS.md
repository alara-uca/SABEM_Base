# Git Workflow

Guía rápida para generar mensajes de *commit* con GitHub Copilot (o similar) de forma consistente en
este repositorio. Complementa, no sustituye, lo visto en clase sobre *commits* atómicos: el formato de
abajo es la **primera línea** del mensaje; el cuerpo (el "por qué", si hace falta) se sigue escribiendo
como siempre, en una línea en blanco después.

Commits siempre en **español**. Formato: `:emoji:tipo(scope): descripción` (max 72 chars).

| Emoji | Código | Tipo | Uso |
| :-: | - | - | - |
| :star: | `:star:` | `nuevo` | nueva funcionalidad o módulo |
| :bug: | `:bug:` | `fix` | corrección de bug |
| :memo: | `:memo:` | `docs` | solo documentación |
| :art: | `:art:` | `estilo` | formato, sin cambio lógico |
| :construction: | `:construction:` | `reestructura` | reestructura del código sin cambio de comportamiento |
| :crossed_fingers: | `:crossed_fingers:` | `test` | tests de hardware |
| :wrench: | `:wrench:` | `tarea` | build, toolchain, ficheros de proyecto |
| :bookmark: | `:bookmark:` | `hito` | commit de fusión (título del *Pull Request* fusionado en GitHub) que cierra un hito H1-H10 en `main` |

* **emoji**: el emoji que se ve en GitHub (y en `git log --oneline --graph --decorate`) para identificar
  visualmente el tipo de commit.
* **tipo**: el tipo de commit, que puede ser `nuevo`, `fix`, `docs`, `estilo`, `reestructura`, `test`, `tarea` o `hito`.
* **scope**: nombre del módulo o funcionalidad afectada (`sensor`, `fsm`, `motor`, `pwm`...). Es
**opcional**: si el cambio no encaja en un módulo concreto (toca varios ficheros a la vez, o es algo
general del proyecto), se omite el scope y el paréntesis entero:

```text
:emoji:tipo(scope): descripción
```

## Ejemplos (de este mismo repositorio)

| Commit real del historial | Con el nuevo formato |
| --- | --- |
| `Anadir lectura simulada del sensor de guiado` | `:star:nuevo(sensor): anadir lectura simulada del sensor de guiado` |
| `Simular oscilacion en la lectura para probar el filtro` | `:crossed_fingers:test(sensor): simular oscilacion en la lectura para probar el filtro` |
| `Anadir esqueleto de la maquina de estados` | `:star:nuevo(fsm): anadir esqueleto de la maquina de estados` |
| `Implementar las transiciones basicas de la FSM` | `:star:nuevo(fsm): implementar las transiciones basicas de la fsm` |
| `Anadir control de velocidad por PWM (esqueleto)` | `:star:nuevo(motor): anadir control de velocidad por pwm, esqueleto` |
| `Anadir caso por defecto a la FSM, en main` | `:x:fix(fsm): anadir caso por defecto que faltaba en el switch` |
| `Anadir gitignore y README inicial del equipo` | `:wrench:tarea: anadir gitignore, readme, convenciones de commit y plantillas del equipo` |
| `Fusionar h1-sensor: hito H1 verificado` | `:bookmark:hito(sensor): fusionar h1-sensor, hito H1 verificado` |
| `Fusionar h2-fsm: hito H2 verificado` | `:bookmark:hito(fsm): fusionar h2-fsm, hito H2 verificado` |

El ejemplo de `tarea` (sin scope) es el caso típico de "toca varios ficheros a la vez, no hay un
módulo concreto" — en este repositorio es, de hecho, el primer commit: `.gitignore`, `README.md`,
este mismo fichero y toda la carpeta `plantillas/` juntos. Los ejemplos de `hito` son el mensaje que
lleva el commit de fusión (el título al fusionar el Pull Request) con el que se cierra cada hito en `main` (el que luego se etiqueta con
`git tag hN`) — así se distingue de un `nuevo(...)` normal con un vistazo al `git log`.

## Con el cuerpo del mensaje (cuando hace falta el "por qué")

```text
:bug:fix(sensor): filtrar la lectura con media movil de 8

La lectura en bruto oscilaba +-40 mV con los motores en marcha, lo que
son unos 0,8 mm de posicion falsa. Con M=8 el ruido baja a +-14 mV.
```

La primera línea sigue el formato `tipo(scope): descripción`; el resto de la norma sobre *commits*
atómicos y el "por qué" en el cuerpo es la misma que ya se vio en clase.
