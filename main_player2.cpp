// main_player2.cpp
// This is the main.cpp file for Player 2 (Slave Node/Remote Controller)
// Este es el archivo main.cpp para el Jugador 2 (Nodo Esclavo/Controlador Remoto)
//
// IMPORTANT: This file is NOT compiled in this project. It belongs to the pong_p2 project.
// IMPORTANTE: Este archivo NO se compila en este proyecto. Pertenece al proyecto pong_p2.
//
// This file is included here for reference and to be shared alongside the main project.
// Este archivo se incluye aquí como referencia y para compartirse junto al proyecto principal.

#include <Arduino.h>
#include <TFT_eSPI.h>

#define BTN_UP    32
#define BTN_DOWN  33

struct StatePkt {
  uint8_t a, b;
  uint8_t ballX, ballY;
  uint8_t p1Y, p2Y;
  uint8_t sL, sR;
  uint8_t cs;
};

static inline uint8_t pkt_cs(const StatePkt &p) {
  return p.ballX ^ p.ballY ^ p.p1Y ^ p.p2Y ^ p.sL ^ p.sR;
}

// Lee un paquete buscando AA 55 y luego el resto.
// Devuelve true si recibió un paquete válido.
bool recv_state(StatePkt &out) {
  static uint8_t sync = 0;

  while (Serial.available()) {
    uint8_t c = (uint8_t)Serial.read();

    if (sync == 0) {
      if (c == 0xAA) sync = 1;
    } else if (sync == 1) {
      if (c == 0x55) {
        sync = 2;
      } else {
        sync = 0;
      }
    } else {
      // Ya tenemos AA 55, este 'c' es el primer byte del payload (ballX)
      uint8_t *ptr = (uint8_t*)&out;
      ptr[0] = 0xAA;
      ptr[1] = 0x55;
      ptr[2] = c;

      int remaining = (int)sizeof(StatePkt) - 3;
      size_t got = Serial.readBytes(ptr + 3, remaining);

      sync = 0;

      if (got == (size_t)remaining && out.cs == pkt_cs(out)) {
        return true;
      }
    }
  }
  return false;
}


static inline bool pressed(int pin) {
  return digitalRead(pin) == LOW;
}


TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

static inline int iround_u8(uint8_t v) { return (int)v; }

void render_state(const StatePkt &st) {
  spr.fillSprite(TFT_BLACK);

  // línea central (opcional, empieza más abajo para no tapar marcador)
  for (int y = 20; y < 128; y += 6) {
    spr.fillRect(160/2 - 1, y, 2, 3, TFT_WHITE);
  }

  // marcador fijo (como ya lo dejaste perfecto)
  char L[8], R[8];
  snprintf(L, sizeof(L), "%02u", st.sL);
  snprintf(R, sizeof(R), "%02u", st.sR);
  spr.drawString(L, 160/2 - 40, 2, 1);
  spr.drawString(R, 160/2 + 20, 2, 1);

  // palas (mismo tamaño que tu juego: w=4 h=20, x=6 y x=160-6-4)
  const int pW = 4, pH = 20;
  spr.fillRect(6,              iround_u8(st.p1Y), pW, pH, TFT_WHITE);
  spr.fillRect(160 - 6 - pW,   iround_u8(st.p2Y), pW, pH, TFT_WHITE);

  // bola (size=3)
  spr.fillRect(iround_u8(st.ballX), iround_u8(st.ballY), 3, 3, TFT_WHITE);

  spr.pushSprite(0, 0);
}


void setup() {
  // botones
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);

  // UART0
  Serial.begin(115200);

  // TFT
  tft.init();
  tft.setRotation(3);
  spr.createSprite(160, 128);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);

  randomSeed(esp_random()); // opcional
}

void loop() {
  // 1) enviar input a P1 (igual que ya lo tienes)
  char cmd = 'N';
  if (pressed(BTN_UP)) cmd = 'U';
  else if (pressed(BTN_DOWN)) cmd = 'D';

  Serial.write(cmd);
  Serial.write('\n');

  // 2) recibir estado y renderizar cuando llegue
  static StatePkt st;
  if (recv_state(st)) {
    render_state(st);
  }

  delay(10);
}
