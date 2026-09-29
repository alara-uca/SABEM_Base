# AGV Filoguiado — Equipo <nº>

> ## ⚠️ PRIMEROS PASOS DEL EQUIPO — BORRAD ESTE BLOQUE cuando los hayáis hecho
>
> Este repositorio es una **plantilla**. El detalle está en la lección L1.1 (Git II a IV).
>
> **Primer integrante (una sola vez):**
> 1. Cread una cuenta personal en GitHub y solicitad las [ventajas para estudiantes](https://education.github.com/students) con el correo de la UCA.
> 2. En esta página, **Use this template → Create a new repository**. Propietario: vuestro usuario. **Visibilidad: Private** (el repositorio se hará público en la entrega final, según indique la tarea del Campus Virtual).
> 3. **Settings → Collaborators → Add people**: invitad al otro integrante y al profesor (`ale87jan`).
> 4. **Settings → Rules → Rulesets** (o **Branches**): proteged `main` exigiendo un *Pull Request* antes de fusionar. Nadie hace *push* directo a `main`.
> 5. Sustituid los marcadores `<...>` de este `README.md` y de `AUTHORS.md`.
>
> **Segundo integrante:** aceptad la invitación y clonad el repositorio (`git clone https://github.com/<usuario>/<repositorio>.git`).
>
> **Después:** instalad el entorno con `docs/Guia_instalacion_VSCode_MCUXpresso.md`, importad el proyecto en `firmware/` (ver `firmware/README.md`) y trabajad con una rama por hito y *Pull Request* (`COMMIT_CONVENTIONS.md`). Si os atascáis, abrid un *issue* con la plantilla de ayuda y mencionad a `@ale87jan`.
>
> **No hagáis *fork* de esta plantilla:** un *fork* de un repositorio público no se puede hacer privado.

---


Trabajo práctico de **Sistemas Automáticos Basados en Microcontroladores**
Grados en Ingeniería Electrónica Industrial y en Ingeniería en Tecnologías Industriales ·
Universidad de Cádiz · Curso 2026-27

**Integrantes:** <nombre 1>, <nombre 2> (y <nombre 3>, si el equipo es de tres)

## 1. Descripción del sistema

> [!TIP]
> Qué hace el AGV, en 5-10 líneas.

## 2. Arquitectura del firmware

> [!TIP]
> Diagrama de módulos y responsabilidades. Máquina de estados principal.

## 3. Mapa de conexiones

| Pin | Señal | Dispositivo |
| --- | ----- | ----------- |

## 4. Caracterización del sensor de guiado

> [!TIP]
> Curva tensión frente a distancia al hilo, procedimiento de calibración y filtrado aplicado.

## 5. Lazo de control

> [!TIP]
> Estructura del PID discreto, periodo de muestreo, método de sintonización y resultados.

## 6. Telemetría MQTT

| Topic | Sentido | Carga útil | Periodo |
| ----- | ------- | ---------- | ------- |

## 7. Cómo compilar y cargar

> [!TIP]
> Pasos reproducibles: cómo importar el proyecto de `firmware/`, compilar y cargarlo en la
> tarjeta. Comprobad que alguien de otro equipo puede repetirlos sin preguntaros.

## 8. Resultados de validación en pista

> [!TIP]
> Tiempos de vuelta, error de seguimiento, comportamiento ante parada de emergencia.

## 9. Agradecimientos y Herramientas

> [!TIP]
> Declaración obligatoria de uso de IAG: usad la plantilla `plantillas/DECLARACION_IAG.md`.

## 10. Licencia

Todos los materiales y diseños de hardware, el firmware y la documentación proporcionados aquí están
licenciados bajo las siguientes licencias:

- Hardware: CERN Open Hardware Licence Version 2 - Strongly Reciprocal [^1] ([`LICENSE-HW.md`](LICENSE-HW.md))
- Código fuente (firmware): GNU General Public License version 3 or later [^2] ([`LICENSE.md`](LICENSE.md))
- Documentación: GNU Free Documentation License, Version 1.3 or later [^3] ([`LICENSE-DOCS.md`](LICENSE-DOCS.md))

[^1]: El hardware del AGV (PCB, esquemático, planos de mecanizado) está licenciado bajo la licencia
CERN OHL v2.0, que permite el uso, modificación y distribución del hardware siempre que se mantenga
la misma licencia y se reconozca a los autores originales.

[^2]: El firmware del AGV está licenciado bajo la GNU GPL v3 o posterior, lo que permite el uso,
modificación y distribución del código fuente siempre que se mantenga la misma licencia y se
reconozca a los autores originales.

[^3]: La documentación del AGV (manuales, guías, diagramas) está licenciada bajo la GNU FDL v1.3 o
posterior, lo que permite el uso, modificación y distribución de la documentación siempre que se
mantenga la misma licencia y se reconozca a los autores originales.

## 11. Referencias y recursos de aprendizaje

- **Github**
    - [Github para estudiantes](https://docs.github.com/es/education/about-github-education/github-education-for-students/about-github-education-for-students)
    - [GitHub Docs](https://docs.github.com/es)
    - [Guía de sintaxis de escritura y formato en GitHub](https://docs.github.com/es/get-started/writing-on-github/getting-started-with-writing-and-formatting-on-github/basic-writing-and-formatting-syntax)
- **Curso de introducción a Git y GitHub** (GitHub Skills)
    - [Introducción a Github](https://github.com/skills/introduction-to-github)
    - [Comunicación con Markdown](https://github.com/skills/communicate-using-markdown)
    - [Introducción a Git](https://github.com/skills/introduction-to-git)
    - [Introducción a la gestión de repositorios](https://github.com/skills/introduction-to-repository-management)
- **FRDM-MCXA156**
    - [Página oficial de la tarjeta](https://www.nxp.com/design/development-boards/freedom-development-boards/freedom-development-platform-for-mcxa156:FRDM-MCXA156)
    - [Guía de inicio rápido](https://www.nxp.com/document/guide/getting-started-with-frdm-mcxa156:GS-FRDM-MCXA156)
    - [Manual de usuario](https://www.nxp.com/docs/en/user-manual/UM12121.pdf)
    - [Página oficial del MCXA156](https://www.nxp.com/products/MCX-A13X-A14X-A15X)
    - Manual de referencia del MCXA156 disponible en el Campus Virtual
- [Documentación de MCUXpresso](https://mcuxpresso.nxp.com/mcux-vscode/latest/)
