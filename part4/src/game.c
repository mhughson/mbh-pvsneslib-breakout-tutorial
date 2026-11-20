#include "game.h"

static const char levels[LEVEL_COUNT][BRICK_ROWS][BRICK_COLS + 1] = {
    {
        "22222222222222",
        "11001100110011",
        "11111111111111",
        "00110011001100",
        "11111111111111"
    },
    {
        "00000111100000",
        "00011111111000",
        "01111111111110",
        "00011111111000",
        "00000111100000"
    },
    {
        "33333333333333",
        "10010011001001",
        "11111111111111",
        "00110100101100",
        "11111111111111"
    }
};

static int magnitude(int value)
{
    if (value < 0)
        return -value;
    return value;
}

static void attach_ball(Game *game)
{
    game->ball_x = game->paddle_x + (game->paddle_width - BALL_SIZE) / 2;
    game->ball_y = PADDLE_Y - BALL_SIZE;
    game->vx = BALL_UNIT;
    game->vy = -3 * BALL_UNIT;
    game->ball_fraction_x = 0;
    game->ball_fraction_y = 0;
}

static void clear_capsules(Game *game)
{
    int i;

    for (i = 0; i < CAPSULE_COUNT; ++i) {
        game->capsules[i].x = 0;
        game->capsules[i].y = 0;
        game->capsules[i].active = 0;
    }
}

static void load_level(Game *game)
{
    int row;
    int col;
    int i;

    game->paddle_x = (FIELD_LEFT + FIELD_RIGHT - PADDLE_WIDTH) / 2;
    game->paddle_width = PADDLE_WIDTH;
    game->wide_ticks = 0;
    game->bricks_left = 0;
    game->state = STATE_READY;
    game->hits = 0;
    game->drops = 0;
    clear_capsules(game);
    i = 0;
    for (row = 0; row < BRICK_ROWS; ++row) {
        for (col = 0; col < BRICK_COLS; ++col) {
            game->bricks[i] = levels[game->level][row][col] - '0';
            if (game->bricks[i])
                ++game->bricks_left;
            ++i;
        }
    }
    attach_ball(game);
}

void game_reset(Game *game)
{
    game->score = 0;
    game->lives = 3;
    game->level = 0;
    load_level(game);
}

void game_next_level(Game *game)
{
    if (game->state == STATE_LEVEL_CLEAR && game->level + 1 < LEVEL_COUNT) {
        ++game->level;
        load_level(game);
    }
}

static void drop_capsule(Game *game, int row, int col)
{
    int i;

    ++game->drops;
    if (game->drops % CAPSULE_INTERVAL != 0 || game->bricks_left == 0)
        return;
    for (i = 0; i < CAPSULE_COUNT; ++i) {
        if (!game->capsules[i].active) {
            game->capsules[i].x = BRICK_X + col * BRICK_WIDTH
                                 + (BRICK_WIDTH - CAPSULE_SIZE) / 2;
            game->capsules[i].y = BRICK_Y + row * BRICK_HEIGHT;
            game->capsules[i].active = 1;
            return;
        }
    }
}

static void resize_paddle(Game *game, int width)
{
    game->paddle_x -= (width - game->paddle_width) / 2;
    game->paddle_width = width;
    if (game->paddle_x < FIELD_LEFT)
        game->paddle_x = FIELD_LEFT;
    if (game->paddle_x + width > FIELD_RIGHT)
        game->paddle_x = FIELD_RIGHT - width;
}

static void update_capsules(Game *game)
{
    int i;
    Capsule *capsule;

    for (i = 0; i < CAPSULE_COUNT; ++i) {
        capsule = &game->capsules[i];
        if (!capsule->active)
            continue;
        capsule->y += CAPSULE_SPEED;
        if (capsule->y < PADDLE_Y + PADDLE_HEIGHT &&
            capsule->y + CAPSULE_SIZE > PADDLE_Y &&
            capsule->x < game->paddle_x + game->paddle_width &&
            capsule->x + CAPSULE_SIZE > game->paddle_x) {
            capsule->active = 0;
            game->hits |= HIT_CAPSULE;
            resize_paddle(game, WIDE_PADDLE_WIDTH);
            game->wide_ticks = WIDE_FRAMES;
        } else if (capsule->y >= FIELD_BOTTOM) {
            capsule->active = 0;
        }
    }
}

static int hit_brick(Game *game, int x, int y)
{
    int first_row;
    int last_row;
    int first_col;
    int last_col;
    int row;
    int col;
    int index;

    if (y + BALL_SIZE <= BRICK_Y ||
        y >= BRICK_Y + BRICK_ROWS * BRICK_HEIGHT ||
        x + BALL_SIZE <= BRICK_X ||
        x >= BRICK_X + BRICK_COLS * BRICK_WIDTH)
        return 0;

    /* The grid has no gaps, so only the bricks under the ball's corners can
       be hit. Working them out directly is much cheaper than a full scan. */
    first_row = y < BRICK_Y ? 0 : (unsigned int)(y - BRICK_Y) / BRICK_HEIGHT;
    last_row = (unsigned int)(y + BALL_SIZE - 1 - BRICK_Y) / BRICK_HEIGHT;
    if (last_row >= BRICK_ROWS)
        last_row = BRICK_ROWS - 1;
    first_col = x < BRICK_X ? 0 : (unsigned int)(x - BRICK_X) / BRICK_WIDTH;
    last_col = (unsigned int)(x + BALL_SIZE - 1 - BRICK_X) / BRICK_WIDTH;
    if (last_col >= BRICK_COLS)
        last_col = BRICK_COLS - 1;

    for (row = first_row; row <= last_row; ++row) {
        for (col = first_col; col <= last_col; ++col) {
            index = row * BRICK_COLS + col;
            if (game->bricks[index]) {
                game->hits |= HIT_BRICK;
                --game->bricks[index];
                if (game->bricks[index])
                    return 1;
                game->score += 10;
                --game->bricks_left;
                drop_capsule(game, row, col);
                if (game->bricks_left == 0) {
                    clear_capsules(game);
                    if (game->level + 1 == LEVEL_COUNT)
                        game->state = STATE_WON;
                    else
                        game->state = STATE_LEVEL_CLEAR;
                }
                return 1;
            }
        }
    }
    return 0;
}

static void move_ball_x(Game *game)
{
    int next_x;

    next_x = game->ball_x + (game->vx > 0 ? 1 : -1);
    if (next_x < FIELD_LEFT || next_x + BALL_SIZE > FIELD_RIGHT) {
        game->hits |= HIT_WALL;
        game->vx = -game->vx;
    } else if (hit_brick(game, next_x, game->ball_y)) {
        game->vx = -game->vx;
    } else {
        game->ball_x = next_x;
    }
}

static void paddle_bounce(Game *game)
{
    int hit_position;

    hit_position = game->ball_x + BALL_SIZE / 2 - game->paddle_x;
    if (hit_position < game->paddle_width / 5) {
        game->vx = -40;
        game->vy = -24;
    } else if (hit_position < 2 * game->paddle_width / 5) {
        game->vx = -24;
        game->vy = -40;
    } else if (hit_position < 3 * game->paddle_width / 5) {
        game->vx = game->vx < 0 ? -8 : 8;
        game->vy = -48;
    } else if (hit_position < 4 * game->paddle_width / 5) {
        game->vx = 24;
        game->vy = -40;
    } else {
        game->vx = 40;
        game->vy = -24;
    }
    game->ball_fraction_x = 0;
    game->ball_fraction_y = 0;
}

static void move_ball_y(Game *game)
{
    int next_y;

    next_y = game->ball_y + (game->vy > 0 ? 1 : -1);
    if (next_y < FIELD_TOP) {
        game->hits |= HIT_WALL;
        game->vy = -game->vy;
    } else if (game->vy > 0 &&
               game->ball_y + BALL_SIZE <= PADDLE_Y &&
               next_y + BALL_SIZE > PADDLE_Y &&
               game->ball_x + BALL_SIZE > game->paddle_x &&
               game->ball_x < game->paddle_x + game->paddle_width) {
        game->hits |= HIT_PADDLE;
        game->ball_y = PADDLE_Y - BALL_SIZE;
        paddle_bounce(game);
    } else if (hit_brick(game, game->ball_x, next_y)) {
        game->vy = -game->vy;
    } else {
        game->ball_y = next_y;
    }
}

static void lose_life(Game *game)
{
    clear_capsules(game);
    resize_paddle(game, PADDLE_WIDTH);
    game->wide_ticks = 0;
    --game->lives;
    if (game->lives == 0) {
        game->state = STATE_GAME_OVER;
    } else {
        game->state = STATE_READY;
        attach_ball(game);
    }
}

void game_move_paddle(Game *game, int dx)
{
    if (game->state != STATE_READY && game->state != STATE_PLAYING)
        return;
    game->paddle_x += dx;
    if (game->paddle_x < FIELD_LEFT)
        game->paddle_x = FIELD_LEFT;
    if (game->paddle_x + game->paddle_width > FIELD_RIGHT)
        game->paddle_x = FIELD_RIGHT - game->paddle_width;
}

void game_tick(Game *game, int movement, int action_pressed)
{
    int step;

    game->hits = 0;
    if (game->state == STATE_LEVEL_CLEAR) {
        if (action_pressed)
            game_next_level(game);
        return;
    }
    if (game->state == STATE_GAME_OVER || game->state == STATE_WON) {
        if (action_pressed)
            game_reset(game);
        return;
    }

    game_move_paddle(game, movement * PADDLE_SPEED);

    if (game->state == STATE_READY) {
        attach_ball(game);
        if (action_pressed)
            game->state = STATE_PLAYING;
        return;
    }

    if (game->wide_ticks > 0) {
        --game->wide_ticks;
        if (game->wide_ticks == 0)
            resize_paddle(game, PADDLE_WIDTH);
    }
    update_capsules(game);
    game->ball_fraction_x += magnitude(game->vx);
    game->ball_fraction_y += magnitude(game->vy);
    /* Spend sixteenths one pixel at a time, retaining the fractional part. */
    for (step = 0; step < BALL_SUBSTEPS; ++step) {
        if (game->ball_fraction_x >= BALL_UNIT) {
            game->ball_fraction_x -= BALL_UNIT;
            move_ball_x(game);
        }
        if (game->state != STATE_PLAYING)
            return;
        if (game->ball_fraction_y >= BALL_UNIT) {
            game->ball_fraction_y -= BALL_UNIT;
            move_ball_y(game);
        }
        if (game->state != STATE_PLAYING)
            return;
        if (game->ball_y >= FIELD_BOTTOM) {
            lose_life(game);
            return;
        }
    }
}
