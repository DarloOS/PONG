// src/main.cpp
// Pong para ESP32 + TFT_eSPI (ST7732S/ST7735) usando SPRITE (sin parpadeo)
// Mantiene tu lógica casi intacta: structs, init_game(), update_game()
// Cambios mínimos: time (micros), input (GPIO), render (TFT sprite)

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <stdio.h>

// ===================== STRUCTS =====================

typedef struct
{
    float size;   /* tamaño de la bola */
    float x;      /* posición x */
    float y;      /* posición y */
    float speedx; /* velocidad x */
    float speedy; /* velocidad y */
} Ball;

typedef struct
{
    float h;     /* alto de la pala */
    float w;     /* ancho de la pala */
    float x;     /* posición x */
    float y;     /* posición y */
    float speed; /* velocidad de la pala */
} Paddle;

typedef struct
{
    /* Medidas del mundo */
    int w;
    int h;

    /* Marcador */
    int scoreL;
    int scoreR;

    Ball ball;
    Paddle p1;
    Paddle p2;

} Game;

// ===================== TIEMPO (Arduino) =====================

static inline double now_seconds(void)
{
    return (double)micros() * 1e-6;
}

// ===================== INIT GAME =====================

void init_game(Game *g)
{
    /* iniciar mundo */
    g->w = 160;
    g->h = 128;
    g->scoreL = 0;
    g->scoreR = 0;

    /* iniciar bola */
    g->ball.size = 3;
    g->ball.x = (g->w - g->ball.size) / 2;
    g->ball.y = (g->h - g->ball.size) / 2;
    g->ball.speedx = 120;
    g->ball.speedy = 0;

    /* iniciar p1 */
    g->p1.w = 4;
    g->p1.h = 20;
    g->p1.speed = 60;
    g->p1.x = 6;
    g->p1.y = (g->h - g->p1.h) / 2;

    /* iniciar p2 */
    g->p2.w = 4;
    g->p2.h = 20;
    g->p2.speed = 60;
    g->p2.x = g->w - 6 - g->p2.w;
    g->p2.y = (g->h - g->p2.h) / 2;
}

// ===================== UPDATE GAME =====================

void update_game(Game *g, float dt)
{
    g->ball.x += g->ball.speedx * dt;
    g->ball.y += g->ball.speedy * dt;

    if (g->ball.y <= 0)
    {
        g->ball.y = 0;
        g->ball.speedy = -g->ball.speedy;
    }
    if (g->ball.y >= g->h - g->ball.size)
    {
        g->ball.y = g->h - g->ball.size;
        g->ball.speedy = -g->ball.speedy;
    }

    if (g->ball.x <= 0)
    {
        g->scoreR++;
        g->ball.x = (g->w - g->ball.size) / 2;
        g->ball.y = (g->h - g->ball.size) / 2;
        g->ball.speedx = 120;
        g->ball.speedy = 0;
    }

    // Colisión con pala izquierda (p1) solo si la bola va hacia la izquierda
    if (g->ball.speedx < 0 &&
        g->ball.x < g->p1.x + g->p1.w &&
        g->ball.x + g->ball.size > g->p1.x &&
        g->ball.y < g->p1.y + g->p1.h &&
        g->ball.y + g->ball.size > g->p1.y)
    {
        // Rebotar en X
        g->ball.speedx = -g->ball.speedx;

        // Sacar la bola fuera de la pala para evitar “pegado”
        g->ball.x = g->p1.x + g->p1.w;

        // Ángulo según punto de impacto
        float ballCenter = g->ball.y + g->ball.size * 0.5f;
        float paddleCenter = g->p1.y + g->p1.h * 0.5f;
        float hit = ballCenter - paddleCenter;
        g->ball.speedy = hit * 2.0f;
    }

    // Colisión con pala derecha (p2) solo si la bola va hacia la derecha
    if (g->ball.speedx > 0 &&
        g->ball.x < g->p2.x + g->p2.w &&
        g->ball.x + g->ball.size > g->p2.x &&
        g->ball.y < g->p2.y + g->p2.h &&
        g->ball.y + g->ball.size > g->p2.y)
    {
        g->ball.speedx = -g->ball.speedx;

        // Sacar la bola fuera de la pala derecha
        g->ball.x = g->p2.x - g->ball.size;

        float ballCenter = g->ball.y + g->ball.size * 0.5f;
        float paddleCenter = g->p2.y + g->p2.h * 0.5f;
        float hit = ballCenter - paddleCenter;
        g->ball.speedy = hit * 2.0f;
    }

    if (g->ball.x >= g->w - g->ball.size)
    {
        g->scoreL++;
        g->ball.x = (g->w - g->ball.size) / 2;
        g->ball.y = (g->h - g->ball.size) / 2;
        g->ball.speedx = -120;
        g->ball.speedy = 0;
    }
}

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

static inline uint8_t clamp_u8(int v, int lo, int hi) {
  if (v < lo) v = lo;
  if (v > hi) v = hi;
  return (uint8_t)v;
}

void send_state_to_p2(const Game *g) {
  StatePkt p;
  p.a = 0xAA; p.b = 0x55;

  p.ballX = clamp_u8((int)(g->ball.x + 0.5f), 0, g->w - 1);
  p.ballY = clamp_u8((int)(g->ball.y + 0.5f), 0, g->h - 1);
  p.p1Y   = clamp_u8((int)(g->p1.y + 0.5f),   0, g->h - 1);
  p.p2Y   = clamp_u8((int)(g->p2.y + 0.5f),   0, g->h - 1);
  p.sL    = (uint8_t)g->scoreL;
  p.sR    = (uint8_t)g->scoreR;

  p.cs = pkt_cs(p);

  // Enviar binario (rápido)
  Serial.write((const uint8_t*)&p, sizeof(p));
}


// ===================== INPUT (ESP32) =====================
// Cambia estos GPIO a los que uses. Se asume botones a GND + INPUT_PULLUP (activo LOW).
static const int BTN_P1_UP = 32;
static const int BTN_P1_DOWN = 33;

char p2_cmd = 'N';
uint32_t last_rx = 0;

static inline bool pressed(int pin)
{
    return digitalRead(pin) == LOW;
}

void poll_uart_p2() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'U' || c == 'D' || c == 'N') {
      p2_cmd = c;
      last_rx = millis();
    }
  }

  // si se pierde conexión, pala quieta
  if (millis() - last_rx > 300) {
    p2_cmd = 'N';
  }
}

void handle_input_esp32(Game *g, float dt)
{
    bool up1 = pressed(BTN_P1_UP);
    bool down1 = pressed(BTN_P1_DOWN);

    if (up1)
        g->p1.y -= g->p1.speed * dt;
    if (down1)
        g->p1.y += g->p1.speed * dt;

    // INPUT REMOTO P2
    poll_uart_p2();

    if (p2_cmd == 'U') {
    g->p2.y -= g->p2.speed * dt;
    }
    else if (p2_cmd == 'D') {
    g->p2.y += g->p2.speed * dt;
    }

    // Clamp
    if (g->p2.y < 0) g->p2.y = 0;
    if (g->p2.y > g->h - g->p2.h) g->p2.y = g->h - g->p2.h;

    /* Clamp de las P1 */
    if (g->p1.y < 0)
        g->p1.y = 0;
    if (g->p1.y > g->h - g->p1.h)
        g->p1.y = g->h - g->p1.h;
    
}

// ===================== RENDER (Sprite) =====================

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

Game game;

static inline int iroundf(float x)
{
    return (int)(x + 0.5f);
}

void render_game(const Game *g)
{
    spr.fillSprite(TFT_BLACK);

    // Banda superior para que el marcador siempre se vea
    spr.fillRect(0, 0, g->w, 18, TFT_BLACK);
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);

    // Marcador fijo (no se mueve)
    spr.setTextColor(TFT_WHITE, TFT_BLACK);

    char L[8], R[8];
    snprintf(L, sizeof(L), "%02d", g->scoreL);
    snprintf(R, sizeof(R), "%02d", g->scoreR);

    // Posiciones fijas alrededor del centro
    spr.drawString(L, g->w/2 - 40, 2, 1);
    spr.drawString(R, g->w/2 + 20, 2, 1);



    // Línea central (empieza más abajo para no molestar al marcador)
    for (int y = 20; y < g->h; y += 6)
    {
        spr.fillRect(g->w / 2 - 1, y, 2, 3, TFT_WHITE);
    }

    // Palas
    spr.fillRect(iroundf(g->p1.x), iroundf(g->p1.y), iroundf(g->p1.w), iroundf(g->p1.h), TFT_WHITE);
    spr.fillRect(iroundf(g->p2.x), iroundf(g->p2.y), iroundf(g->p2.w), iroundf(g->p2.h), TFT_WHITE);

    // Bola
    spr.fillRect(iroundf(g->ball.x), iroundf(g->ball.y), iroundf(g->ball.size), iroundf(g->ball.size), TFT_WHITE);

    spr.pushSprite(0, 0);
}




// ===================== ARDUINO setup/loop =====================

void setup()
{
    Serial.begin(115200);  // UART0
    
    // Botones
    pinMode(BTN_P1_UP, INPUT_PULLUP);
    pinMode(BTN_P1_DOWN, INPUT_PULLUP);

    // TFT
    tft.init();
    tft.setRotation(3);

    // Sprite: profundidad 16-bit por defecto (va perfecto en 160x128)
    // Si quieres ahorrar RAM, descomenta:
    // spr.setColorDepth(8);

    spr.createSprite(160, 128);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);

    init_game(&game);
}

void loop()
{
    static double last = now_seconds();
    double t = now_seconds();
    float dt = (float)(t - last);
    last = t;

    // Evitar saltos si algo bloquea el loop
    if (dt > 0.05f)
        dt = 0.05f;

    handle_input_esp32(&game, dt);
    update_game(&game, dt);
    render_game(&game);

    static uint32_t lastSend = 0;
    uint32_t now = millis();
    if (now - lastSend >= 33) { // ~30 Hz
        lastSend = now;
        send_state_to_p2(&game);
    }

    // Pequeño cap para estabilidad (opcional)
    delay(1);
}
