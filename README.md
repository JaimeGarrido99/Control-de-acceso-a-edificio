# Control-de-acceso-a-edificio
## Descripción 📜
Este proyecto diseñado para una asignatura consiste en el diseño de un sistema de control de acceso para un edificio de oficina, implementado en un microcontrolador MSP430. El sistema permite la gestión de **16 usuarios**, incluyendo un **manager** y **15 empleados**. Cada usuario tiene un nombre de usuario de **6 caracteres** y un **PIN de 4 dígitos**, los cuales deben ser ingresados para acceder al edificio.

Los usuarios pueden estar en tres estados diferentes:
- **Trabajando**: El usuario entra al edificio y comienza su turno. 💼
- **Pausa**: El usuario hace una pausa (ej., para descansar). ☕️
- **Salir**: El usuario sale del edificio al finalizar su jornada. 🚪

Si un usuario introduce incorrectamente su PIN más de tres veces, será bloqueado, y solo el **manager** podrá desbloquearlo. Además, el manager puede consultar informes de actividad de los usuarios y desbloquear cuentas bloqueadas.
Además, el manager podrá consultar el tiempo que ha permanecido cada usuario en cada modo y el estado en el que se encuentra.

## Desarrollo 💻

### 1. **Escritura en FLASH** 💾
El programa almacena los nombres de los usuarios y sus respectivos PINs en la memoria **FLASH** del MSP430.

### 2. **Sistema Principal** ⚙️
El sistema principal gestiona los estados de los usuarios y las interacciones a través de una simple interfaz visual y un puerto serie. La pantalla LCD del BoosterPack MKII muestra información y permite al usuario seleccionar su estado (trabajando, pausa, salir). 

Los estados del sistema se gestionan mediante una **máquina de estados**:
- **ARRANQUE**: Inicialización del sistema, incluyendo la configuración de la fecha y hora. ⏰
- **SELEC_USER**: Selección de usuario mediante el joystick. 🎮
- **PIN_US**: Introducción del PIN para verificar la identidad del usuario mediante puerto serie. 🔑
- **MODO**: En este estado, los usuarios pueden elegir entre entrar, hacer una pausa o salir del edificio. 🚶‍♂️💼
- **MODO_ADMIN**: El manager puede desbloquear usuarios y consultar informes. 👨‍💼📊

#### **Interacciones con el Usuario**:
- **Pantalla LCD del BoosterPack MKII**: Muestra mensajes de estado y las opciones disponibles, como la solicitud de la fecha y hora, selección de usuario, y mensajes de error si el PIN es incorrecto.
- **Joystick y Botones**: Permiten la navegación entre opciones y la selección del usuario. 🎮🔘

#### **Temporizadores e Interrupciones**:
- **Interrupciones**: El sistema usa interrupciones para manejar la entrada de datos por puerto serie (para la fecha y el PIN) y la lectura del joystick mediante un ADC (Convertidor Analógico a Digital).
- **Temporizador de 10 ms**: Gestiona el tiempo en los estados y las actualizaciones de la pantalla (por ejemplo, para mostrar la hora y realizar la gestión de estados).
