# 🕹️ Pong ESP32 – Doble Consola con UART

Implementación del clásico **Pong** usando **dos ESP32**, cada uno con su **propia pantalla TFT**, comunicados entre sí mediante **UART**.  
Un ESP32 actúa como **host autoritativo** (lógica, física y marcador) y el otro como **cliente/jugador remoto**, que envía sus entradas y muestra el estado sincronizado del juego.

---

## 📌 Características principales

- 🎮 Juego Pong completo (bola, palas, colisiones y marcador)
- 📺 Pantalla TFT en ambos jugadores
- 🔁 Comunicación **UART** entre dos ESP32
- 🧠 Arquitectura **host–cliente**
- ⚡ Renderizado fluido usando **sprites (framebuffer)**
- ⏱️ Física basada en **delta time (dt)** independiente del framerate
- 🔧 Implementado en **C/C++ (Arduino framework)**

---

## 🧩 Arquitectura del sistema

### ESP32 #1 – Host (Consola principal)
- Conectado a una pantalla TFT
- Lee los botones del **Jugador 1**
- Recibe las entradas del **Jugador 2** por UART
- Ejecuta:
  - Lógica del juego
  - Física y colisiones
  - Marcador
- Renderiza el juego localmente
- Envía el **estado del juego** al ESP32 cliente

### ESP32 #2 – Cliente (Jugador 2)
- Conectado a su propia pantalla TFT
- Lee los botones del **Jugador 2**
- Envía las entradas al host por UART
- Recibe el estado del juego
- Renderiza el juego **sin calcular física**

📡 La sincronización se realiza enviando únicamente el **estado mínimo necesario**, evitando desincronizaciones.

---

## 📟 Comunicación UART

- Comunicación serie TTL a **3.3 V**
- Uso de **UART2 (Serial2)**
- Conexión cruzada:
  - TX Host → RX Cliente
  - TX Cliente → RX Host
  - **GND común obligatorio**

### Protocolo de estado (Host → Cliente)

Paquete binario compacto:


- Cabecera de sincronización
- Checksum XOR para validación
- Muy bajo consumo de ancho de banda

---

## 🖥️ Hardware utilizado

- 2 × **ESP32 DevKit / ESP32-WROOM**
- 2 × **Pantallas TFT SPI** (ST7735 / ST7732S)
  - Resolución típica: **160×128**
- Pulsadores para cada jugador
- Cables Dupont / soldadura directa
- Conexión UART entre placas

---

## 🔌 Conexión de botones

Cada botón se conecta de la siguiente forma:


Configuración por software:
```cpp
pinMode(GPIO, INPUT_PULLUP);

lib_deps = bodmer/TFT_eSPI

build_flags =
  -DST7735_DRIVER
  -DLOAD_GLCD
  -DLOAD_FONT2

pong-esp32/
├── host/
│   └── src/main.cpp      # Host: lógica, render y UART
├── client/
│   └── src/main.cpp      # Cliente: input + render
├── README.md
└── platformio.ini





