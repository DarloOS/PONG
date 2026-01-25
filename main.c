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

typedef struct 
{
    float size; /*tamaño de la bola s de size*/
    float x; /*posición x*/
    float y; /*posición y*/

    float speedx; /*velocidad x de la bola*/
    float speedy; /*velocidad y de la bola*/
}Ball;

typedef struct 
{
    float h; /*alto de la pala*/
    float w; /*ancho de la pala*/
    float x; /*posición realativa de la pala*/
    float y; /*posición relativa de la pala*/
    float speed; /*velocidad de la pala*/

}Paddle;

typedef struct 
{
    /*Medidas del mundo*/
    int w;
    int h;

    /*Marcador*/
    int scoreL;
    int scoreR;

    Ball ball;
    Paddle p1;
    Paddle p2;

} Game;

void init_game(Game *g){
    /*iniciar mundo*/
    g->w = 160;
    g->h = 128;
    g->scoreL = 0;
    g->scoreR = 0;

    /*iniciar bola*/
    g->ball.size = 3;
    g->ball.x = (g->w - g->ball.size)/2;
    g->ball.y = (g->h - g->ball.size)/2;
    g->ball.speedx = 120;
    g->ball.speedy = 0;

    /*iniciar p1*/
    g->p1.w = 4;
    g->p1.h = 20;
    g->p1.speed = 60;
    g->p1.x = 6;
    g->p1.y = (g->h - g->p1.h)/2;
    

    /*iniciar p2*/
    g->p2.w = 4;
    g->p2.h = 20;
    g->p2.speed = 60;
    g->p2.x = g->w - 6 - g->p2.w;
    g->p2.y = (g->h - g->p2.h)/2;
}
 
void update_game(Game *g, float dt){

    g->ball.x += g->ball.speedx * dt;
    g->ball.y += g->ball.speedy * dt;

    if (g->ball.y <= 0) {
      g->ball.y = 0;
      g->ball.speedy = -g->ball.speedy;
    }
    if (g->ball.y >= g->h - g->ball.size) {
        g->ball.y = g->h - g->ball.size;
        g->ball.speedy = -g->ball.speedy;
    }


    if (g->ball.x <= 0) {
        g->scoreR++;
        g->ball.x = (g->w - g->ball.size)/2;
        g->ball.y = (g->h - g->ball.size)/2;
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


    if (g->ball.x >= g->w - g->ball.size) {
        g->scoreL++;
        g->ball.x = (g->w - g->ball.size)/2;
        g->ball.y = (g->h - g->ball.size)/2;
        g->ball.speedx = -120;
        g->ball.speedy = 0;
    }


}

void handle_input_pc(Game *g, float dt) {

    int up1 = 0, down1 = 0;
    int up2 = 0, down2 = 0;
    int k;

    while ((k = read_key()) != -1) {

        if (k == 'w') up1 = 1;
        if (k == 's') down1 = 1;

        if (k == 27) { // ESC sequence
            int k2 = read_key();
            int k3 = read_key();

            if (k2 == '[' && k3 == 'A') up2 = 1;    // flecha arriba
            if (k2 == '[' && k3 == 'B') down2 = 1;  // flecha abajo
        }

        if (k == 'q') {
            disable_raw_mode();
            printf("\x1b[?25h");
            fflush(stdout);
            exit(0);
        }
    }

    if (up1)   g->p1.y -= g->p1.speed * dt;
    if (down1) g->p1.y += g->p1.speed * dt;

    if (up2)   g->p2.y -= g->p2.speed * dt;
    if (down2) g->p2.y += g->p2.speed * dt;


    /*Clamp de las palas*/
    if (g->p1.y < 0){
        g->p1.y = 0;
    } 

    if (g->p1.y > g->h - g->p1.h){
        g->p1.y = g->h - g->p1.h;
    }

    if (g->p2.y < 0){
        g->p2.y = 0;
    } 

    if (g->p2.y > g->h - g->p2.h){
        g->p2.y = g->h - g->p2.h;
    }
}


int main(void){

}
