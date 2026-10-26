#ifndef BREAKOUT_GAME_H
#define BREAKOUT_GAME_H

#define FIELD_LEFT 16
#define FIELD_RIGHT 240
#define FIELD_TOP 32
#define FIELD_BOTTOM 224

#define BALL_SIZE 8
#define BALL_SUBSTEPS 3
#define PADDLE_WIDTH 48
#define PADDLE_HEIGHT 8
#define PADDLE_Y 200
#define PADDLE_SPEED 3

#define BRICK_COLS 10
#define BRICK_ROWS 5
#define BRICK_COUNT (BRICK_COLS * BRICK_ROWS)
#define BRICK_X 30
#define BRICK_Y 48
#define BRICK_WIDTH 16
#define BRICK_HEIGHT 8
#define BRICK_PITCH_X 20
#define BRICK_PITCH_Y 16

enum GameState {
    STATE_READY,
    STATE_PLAYING,
    STATE_GAME_OVER,
    STATE_WON
};

typedef struct Game {
    int paddle_x;
    int ball_x;
    int ball_y;
    int vx;
    int vy;
    unsigned int score;
    int lives;
    int bricks_left;
    int state;
    unsigned char bricks[BRICK_COUNT];
} Game;

void game_reset(Game *game);
void game_tick(Game *game, int movement, int action_pressed);

#endif
