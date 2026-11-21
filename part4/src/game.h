#ifndef BREAKOUT_GAME_H
#define BREAKOUT_GAME_H

#define FIELD_LEFT   16
#define FIELD_RIGHT  240
#define FIELD_TOP    32
#define FIELD_BOTTOM 224

#define BALL_SIZE         8
#define BALL_SUBSTEPS     3
#define BALL_UNIT         16
#define PADDLE_WIDTH      48
#define WIDE_PADDLE_WIDTH 64
#define WIDE_FRAMES       600
#define PADDLE_HEIGHT     8
#define PADDLE_Y          200
#define PADDLE_SPEED      3

#define BRICK_COLS   14
#define BRICK_ROWS   5
#define BRICK_COUNT  (BRICK_COLS * BRICK_ROWS)
#define BRICK_X      16
#define BRICK_Y      48
#define BRICK_WIDTH  16
#define BRICK_HEIGHT 8

#define HIT_WALL         1
#define HIT_PADDLE       2
#define HIT_BRICK        4
#define LEVEL_COUNT      3
#define HIT_CAPSULE      8
#define CAPSULE_COUNT    3
#define CAPSULE_SIZE     8
#define CAPSULE_SPEED    2
#define CAPSULE_INTERVAL 4

enum GameState
{
    STATE_READY,
    STATE_PLAYING,
    STATE_GAME_OVER,
    STATE_WON,
    STATE_LEVEL_CLEAR
};

typedef struct Capsule
{
    int x;
    int y;
    int active;
} Capsule;

typedef struct Game
{
    int paddle_x;
    int ball_x;
    int ball_y;
    int vx;
    int vy;
    unsigned int score;
    int lives;
    int bricks_left;
    int state;
    int hits;
    int level;
    int ball_fraction_x;
    int ball_fraction_y;
    int drops;
    int paddle_width;
    int wide_ticks;
    Capsule capsules[CAPSULE_COUNT];
    unsigned char bricks[BRICK_COUNT];
} Game;

void game_reset(Game *game);
void game_next_level(Game *game);
void game_move_paddle(Game *game, int dx);
void game_tick(Game *game, int movement, int action_pressed);

#endif
