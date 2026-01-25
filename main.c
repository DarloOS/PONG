#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

/*
Bola:
    izquierda → x
    derecha → x + ballS
    arriba → y
    abajo → y + ballS

Pala:
    izquierda → paddleX
    derecha → paddleX + paddleW
    arriba → paddleY
    abajo → paddleY + paddleH

*/

double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static struct termios old_term;

void enable_raw_mode(void) {
    struct termios new_term;
    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;
    new_term.c_lflag &= ~(ICANON | ECHO);     // sin modo línea y sin eco
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    // stdin no bloqueante
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
}

int read_key(void) {
    unsigned char c;
    int n = read(STDIN_FILENO, &c, 1);
    if (n == 1) return (int)c;
    return -1;
}

// ---- RENDER ASCII (terminal) ----
void render_frame(
    int W, int H,
    int puntosL, int puntosR,
    float x, float y, float ballS,
    float paddleX, float paddleY, float paddleW, float paddleH,
    float paddle2X, float paddle2Y, float paddle2W, float paddle2H,
    int sx, int sy
) {
    int CW = W / sx;
    int CH = H / sy;

    // NUEVO: render en buffer + una sola escritura para evitar tirones del terminal
    static char *frame = NULL;
    static size_t frame_cap = 0;

    size_t needed = 64 + (size_t)(CH * (CW + 1)) + 128;
    if (needed > frame_cap) {
        free(frame);
        frame = malloc(needed);
        frame_cap = needed;
        if (!frame) return;
    }

    size_t len = 0;

    len += snprintf(frame + len, frame_cap - len,
                    "\x1b[H""L=%d  R=%d\n\n", puntosL, puntosR);

    for (int cy = 0; cy < CH; cy++) {
        for (int cx = 0; cx < CW; cx++) {

            // Centro de la celda (evita “desapariciones” con sy grande)
            float px = cx * sx + sx * 0.5f;
            float py = cy * sy + sy * 0.5f;

            char c = '.';

            // Línea central
            if (cx == CW / 2) c = ':';

            // Pala izquierda
            if (px >= paddleX && px < paddleX + paddleW &&
                py >= paddleY && py < paddleY + paddleH)
                c = '|';

            // Pala derecha
            if (px >= paddle2X && px < paddle2X + paddle2W &&
                py >= paddle2Y && py < paddle2Y + paddle2H)
                c = '|';

            float cellL = cx * sx;
            float cellR = cellL + sx;
            float cellT = cy * sy;
            float cellB = cellT + sy;

            float ballL = x;
            float ballR = x + ballS;
            float ballT = y;
            float ballB = y + ballS;

            // Bola (la última para que “tape” lo anterior)
            if (cellR > ballL && cellL < ballR &&
                cellB > ballT && cellT < ballB)
                c = 'O';

            frame[len++] = c;
        }
        frame[len++] = '\n';
    }

    // Antes: putchar/fflush → ahora: una sola escritura
    write(STDOUT_FILENO, frame, len);
}

int main(void){
    /*Medidas del mundo*/
    int W = 160;
    int H = 128;

    /*Bola*/
    float ballS = 3; /*tamaño de la bola s de size*/
    float x = (W - ballS) / 2; /*posición x*/
    float y = (H - ballS) / 2; /*posición y*/

    float vx = 1; /*velocidad x de la bola*/
    float vy = 0.5; /*velocidad y de la bola*/

    /*pala*/
    float paddleH = 20; /*alto de la pala*/

    float paddleY = (H - paddleH)/2; /*posición relativa de la pala*/
    float paddle2Y = (H - paddleH)/2;

    float paddleW = 4; /*ancho de la pala*/

    float paddleX = 6; /*posición realativa de la pala*/
    float paddle2X = 140;

    float paddleV = 60; /*velocidad de la pala*/

    /*Marcador de player 1 y 2*/
    int puntosL = 0;
    int puntosR = 0;

    double last = now_seconds();
    float acc = 0.0f;

    enable_raw_mode();

    printf("\x1b[2J\x1b[H\x1b[?25l");
    fflush(stdout);

    float render_acc = 0.0f;
    int sx = 4, sy = 8;   // prueba 4/8 (se ve apaisado)

    while (1){

        double now = now_seconds();
        float dt = now - last;
        last = now;

        render_acc += dt;

        x += vx * dt;
        y += vy * dt;

        /*Si la bola toca el techo rebota pa bajo si toca suelo parriba*/
        if (y <= 0) vy = -vy;
        if (y >= H) vy = -vy;

        /*colisiones*/
        if (x + ballS >= paddleX && x <= paddleX + paddleW &&
            y + ballS >= paddleY && y <= paddleY + paddleH)
        {
            vx = -vx;
            x = paddleX + paddleW;

            float hit = ((y + ballS/2) - (paddleY + paddleH/2));
            vy = hit * 2;
        }

        if (vx > 0 && x + ballS >= paddle2X && x <= paddle2X + paddleW &&
            y + ballS >= paddle2Y && y <= paddle2Y + paddleH)
        {
            vx = -vx;
            x = paddle2X - ballS;

            float hit = ((y + ballS/2) - (paddle2Y + paddleH/2));
            vy = hit * 2;
        }

        /*sitema de puntos*/
        if (x < 0) {
            puntosR++;
            x = W/2;
            y = H/2;
            vx = -1;
            vy = 0;
        }

        if (x > W) {
            puntosL++;
            x = W/2;
            y = H/2;
            vx = 1;
            vy = 0;
        }

        int up = 0, down = 0;
        int up2 = 0, down2 = 0;   // pala derecha (flechas)
        int k;

        while ((k = read_key()) != -1)
        {
            if (k == 'w') up = 1;
            if (k == 's') down = 1;

            if (k == 27){
                int k2 = read_key();
                int k3 = read_key();
                if (k2 == '[' && k3 == 'A') up2 = 1;
                if (k2 == '[' && k3 == 'B') down2 = 1;
            }

            if (k == 'q'){ // para salir
                printf("\x1b[?25h");
                fflush(stdout);
                disable_raw_mode();
                return 0;
            }
        }

        if (up) paddleY -= paddleV * dt;
        if (down) paddleY += paddleV * dt;
        if (up2) paddle2Y -= paddleV * dt;
        if (down2) paddle2Y += paddleV * dt;

        if (paddleY < 0) paddleY = 0;  /*evitar que la pala se vaya de los bordes*/
        if (paddleY > H - paddleH) paddleY = H - paddleH;

        if (paddle2Y < 0) paddle2Y = 0;  /*evitar que la pala se vaya de los bordes*/
        if (paddle2Y > H - paddleH) paddle2Y = H - paddleH;

        acc += dt;
        if (acc >= 1.0f)
            acc = 0.0f;

        if (render_acc >= 0.01f) {  // 0.1s => 10 FPS
            render_frame(W, H, puntosL, puntosR,
                x, y, ballS,
                paddleX, paddleY, paddleW, paddleH,
                paddle2X, paddle2Y, paddleW, paddleH,
                sx, sy);
            render_acc = 0.0f;
        }

        usleep(16000); // ~16 ms → ~60 FPS
    }

    return -1;
}
