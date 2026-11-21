#include <assert.h>
#include <stdio.h>
#include "game.h"

static void playing(Game *game)
{
    game_reset(game);
    game->state = STATE_PLAYING;
}

static void two_bricks(Game *game)
{
    int i;

    playing(game);
    for (i = 0; i < BRICK_COUNT; ++i)
    {
        game->bricks[i] = 0;
    }
    game->bricks[0] = 1;
    game->bricks[BRICK_COUNT - 1] = 1;
    game->bricks_left = 2;
}

static void test_reset_and_launch(void)
{
    Game game;
    int i;

    game_reset(&game);
    assert(game.state == STATE_READY);
    assert(game.lives == 3 && game.score == 0);
    assert(game.bricks_left == BRICK_COUNT);
    for (i = 0; i < BRICK_COUNT; ++i)
    {
        assert(game.bricks[i] == 1);
    }

    game_tick(&game, 1, 0);
    assert(game.paddle_x == 107);
    assert(game.ball_x == 127 && game.ball_y == 192);
    game_tick(&game, 0, 1);
    assert(game.state == STATE_PLAYING);
    game_tick(&game, 0, 0);
    assert(game.ball_x == 128 && game.ball_y == 189);
}

static void test_paddle_limits(void)
{
    Game game;
    int i;

    game_reset(&game);
    for (i = 0; i < 100; ++i)
    {
        game_tick(&game, -1, 0);
    }
    assert(game.paddle_x == FIELD_LEFT);
    assert(game.ball_x == FIELD_LEFT + 20);
    for (i = 0; i < 100; ++i)
    {
        game_tick(&game, 1, 0);
    }
    assert(game.paddle_x == FIELD_RIGHT - PADDLE_WIDTH);
}

static void test_wall_bounces(void)
{
    Game game;

    playing(&game);
    game.ball_x = FIELD_LEFT;
    game.ball_y = 150;
    game.vx = -3;
    game.vy = 0;
    game_tick(&game, 0, 0);
    assert(game.vx == 3 && game.ball_x >= FIELD_LEFT);
    game.ball_x = FIELD_RIGHT - BALL_SIZE;
    game.vx = 3;
    game_tick(&game, 0, 0);
    assert(game.vx == -3 && game.ball_x + BALL_SIZE <= FIELD_RIGHT);
    game.ball_x = 100;
    game.ball_y = FIELD_TOP;
    game.vx = 0;
    game.vy = -3;
    game_tick(&game, 0, 0);
    assert(game.vy == 3 && game.ball_y >= FIELD_TOP);
}

static void test_brick_faces_and_score(void)
{
    Game game;

    two_bricks(&game);
    game.ball_x = BRICK_X + 4;
    game.ball_y = BRICK_Y + BRICK_HEIGHT;
    game.vx = 0;
    game.vy = -3;
    game_tick(&game, 0, 0);
    assert(game.bricks[0] == 0 && game.bricks_left == 1);
    assert(game.score == 10 && game.vy == 3);
    assert(game.ball_y >= BRICK_Y + BRICK_HEIGHT);
    game_tick(&game, 0, 0);
    assert(game.score == 10);

    two_bricks(&game);
    game.ball_x = BRICK_X - BALL_SIZE;
    game.ball_y = BRICK_Y;
    game.vx = 3;
    game.vy = 0;
    game_tick(&game, 0, 0);
    assert(game.bricks[0] == 0 && game.score == 10);
    assert(game.vx == -3 && game.ball_x + BALL_SIZE <= BRICK_X);

    two_bricks(&game);
    game.ball_x = BRICK_X + BRICK_WIDTH;
    game.ball_y = BRICK_Y;
    game.vx = -3;
    game.vy = 0;
    game_tick(&game, 0, 0);
    assert(game.bricks[0] == 0 && game.vx == 3);

    two_bricks(&game);
    game.ball_x = BRICK_X + 4;
    game.ball_y = BRICK_Y - BALL_SIZE;
    game.vx = 0;
    game.vy = 3;
    game_tick(&game, 0, 0);
    assert(game.bricks[0] == 0 && game.vy == -3);
}

static void test_brick_gap_and_corner(void)
{
    Game game;

    playing(&game);
    game.ball_x = BRICK_X + 4;
    game.ball_y = BRICK_Y + BRICK_HEIGHT;
    game.vx = 2;
    game.vy = 0;
    game_tick(&game, 0, 0);
    assert(game.bricks_left == BRICK_COUNT && game.score == 0);

    two_bricks(&game);
    game.ball_x = BRICK_X - BALL_SIZE;
    game.ball_y = BRICK_Y - BALL_SIZE;
    game.vx = 3;
    game.vy = 3;
    game_tick(&game, 0, 0);
    assert(game.bricks[0] == 0 && game.score == 10);
    assert(game.ball_y + BALL_SIZE <= BRICK_Y);
}

static void test_paddle_angles(void)
{
    Game game;
    int positions[3] = {1, 20, 39};
    int expected_vx[3] = {-2, 1, 2};
    int i;

    for (i = 0; i < 3; ++i)
    {
        playing(&game);
        game.ball_x = game.paddle_x + positions[i];
        game.ball_y = PADDLE_Y - BALL_SIZE;
        game.vx = 0;
        game.vy = 3;
        game_tick(&game, 0, 0);
        assert(game.vx == expected_vx[i]);
        assert(game.vy < 0);
        assert(game.ball_y + BALL_SIZE <= PADDLE_Y);
    }

    playing(&game);
    game.ball_y = PADDLE_Y + 1;
    game.vx = 0;
    game.vy = 3;
    game_tick(&game, 0, 0);
    assert(game.vy == 3);
}

static void test_lives_and_restart(void)
{
    Game game;

    playing(&game);
    game.score = 40;
    game.bricks[0] = 0;
    game.bricks_left = BRICK_COUNT - 1;
    game.ball_x = FIELD_LEFT;
    game.ball_y = FIELD_BOTTOM - 1;
    game.vx = 0;
    game.vy = 3;
    game_tick(&game, 0, 0);
    assert(game.lives == 2 && game.state == STATE_READY);
    assert(game.score == 40 && game.bricks[0] == 0);
    assert(game.ball_y == PADDLE_Y - BALL_SIZE);

    game.state = STATE_PLAYING;
    game.lives = 1;
    game.ball_x = FIELD_LEFT;
    game.ball_y = FIELD_BOTTOM - 1;
    game.vy = 3;
    game_tick(&game, 0, 0);
    assert(game.lives == 0 && game.state == STATE_GAME_OVER);
    game_tick(&game, 1, 0);
    assert(game.lives == 0 && game.score == 40);
    game_tick(&game, 0, 1);
    assert(game.state == STATE_READY && game.lives == 3);
    assert(game.score == 0 && game.bricks_left == BRICK_COUNT);
}

static void test_win(void)
{
    Game game;

    two_bricks(&game);
    game.bricks[BRICK_COUNT - 1] = 0;
    game.bricks_left = 1;
    game.score = 490;
    game.ball_x = BRICK_X + 4;
    game.ball_y = BRICK_Y + BRICK_HEIGHT;
    game.vx = 0;
    game.vy = -3;
    game_tick(&game, 0, 0);
    assert(game.state == STATE_WON);
    assert(game.bricks_left == 0 && game.score == 500);
    game_tick(&game, 0, 0);
    assert(game.score == 500);
    game_tick(&game, 0, 1);
    assert(game.state == STATE_READY && game.score == 0);
    assert(game.bricks_left == BRICK_COUNT);
}

int main(void)
{
    test_reset_and_launch();
    test_paddle_limits();
    test_wall_bounces();
    test_brick_faces_and_score();
    test_brick_gap_and_corner();
    test_paddle_angles();
    test_lives_and_restart();
    test_win();
    puts("PASS: 8 gameplay test groups");
    return 0;
}
