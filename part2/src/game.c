#include "game.h"

static int magnitude(int value)
{
    if (value < 0)
        return -value;
    return value;
}

static void attach_ball(Game *game)
{
    game->ball_x = game->paddle_x + (PADDLE_WIDTH - BALL_SIZE) / 2;
    game->ball_y = PADDLE_Y - BALL_SIZE;
    game->vx = 1;
    game->vy = -3;
}

void game_reset(Game *game)
{
    int i;

    game->paddle_x = (FIELD_LEFT + FIELD_RIGHT - PADDLE_WIDTH) / 2;
    game->score = 0;
    game->lives = 3;
    game->bricks_left = BRICK_COUNT;
    game->state = STATE_READY;
    game->hits = 0;
    for (i = 0; i < BRICK_COUNT; ++i)
        game->bricks[i] = 1;
    attach_ball(game);
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
                game->bricks[index] = 0;
                game->score += 10;
                --game->bricks_left;
                if (game->bricks_left == 0)
                    game->state = STATE_WON;
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
    if (hit_position < PADDLE_WIDTH / 3) {
        game->vx = -2;
        game->vy = -2;
    } else if (hit_position >= 2 * PADDLE_WIDTH / 3) {
        game->vx = 2;
        game->vy = -2;
    } else {
        game->vx = game->vx < 0 ? -1 : 1;
        game->vy = -3;
    }
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
               game->ball_x < game->paddle_x + PADDLE_WIDTH) {
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
    --game->lives;
    if (game->lives == 0) {
        game->state = STATE_GAME_OVER;
    } else {
        game->state = STATE_READY;
        attach_ball(game);
    }
}

void game_tick(Game *game, int movement, int action_pressed)
{
    int step;

    game->hits = 0;
    if (game->state == STATE_GAME_OVER || game->state == STATE_WON) {
        if (action_pressed)
            game_reset(game);
        return;
    }

    game->paddle_x += movement * PADDLE_SPEED;
    if (game->paddle_x < FIELD_LEFT)
        game->paddle_x = FIELD_LEFT;
    if (game->paddle_x + PADDLE_WIDTH > FIELD_RIGHT)
        game->paddle_x = FIELD_RIGHT - PADDLE_WIDTH;

    if (game->state == STATE_READY) {
        attach_ball(game);
        if (action_pressed)
            game->state = STATE_PLAYING;
        return;
    }

    /* Check each pixel, rather than jumping across a thin brick. */
    for (step = 0; step < BALL_SUBSTEPS; ++step) {
        if (step < magnitude(game->vx))
            move_ball_x(game);
        if (game->state != STATE_PLAYING)
            return;
        if (step < magnitude(game->vy))
            move_ball_y(game);
        if (game->state != STATE_PLAYING)
            return;
        if (game->ball_y >= FIELD_BOTTOM) {
            lose_life(game);
            return;
        }
    }
}
