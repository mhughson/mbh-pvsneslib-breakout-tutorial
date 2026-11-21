#include <assert.h>
#include <stdio.h>
#include "game.h"

#define MID   (2 * BRICK_COLS + 6)
#define MID_X (BRICK_X + 6 * BRICK_WIDTH)
#define MID_Y (BRICK_Y + 2 * BRICK_HEIGHT)
#define FAR   (BRICK_COUNT - 1)

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
    game->bricks[MID] = 1;
    game->bricks[FAR] = 1;
    game->bricks_left = 2;
}

static void aim_up(Game *game)
{
    game->ball_x = MID_X + 4;
    game->ball_y = MID_Y + BRICK_HEIGHT;
    game->vx = 0;
    game->vy = -3 * BALL_UNIT;
    game->ball_fraction_x = 0;
    game->ball_fraction_y = 0;
}

static int live_bricks(const Game *game)
{
    int i;
    int count = 0;

    for (i = 0; i < BRICK_COUNT; ++i)
    {
        if (game->bricks[i])
        {
            ++count;
        }
    }
    return count;
}

static void test_reset_and_launch(void)
{
    Game game;

    game_reset(&game);
    assert(game.state == STATE_READY && game.level == 0);
    assert(game.lives == 3 && game.score == 0);
    assert(game.bricks_left == 56 && live_bricks(&game) == 56);
    assert(game.bricks[0] == 2 && game.bricks[BRICK_COLS + 2] == 0);
    game_tick(&game, 1, 0);
    assert(game.paddle_x == 107 && game.ball_x == 127);
    game_tick(&game, 0, 1);
    assert(game.state == STATE_PLAYING);
    game_tick(&game, 0, 0);
    assert(game.ball_x == 128 && game.ball_y == 189);
}

static void test_paddle_limits(void)
{
    Game game;

    game_reset(&game);
    game_move_paddle(&game, -127);
    assert(game.paddle_x == FIELD_LEFT);
    game_move_paddle(&game, 127);
    game_move_paddle(&game, 127);
    assert(game.paddle_x == FIELD_RIGHT - PADDLE_WIDTH);
    game.state = STATE_LEVEL_CLEAR;
    game_move_paddle(&game, -127);
    assert(game.paddle_x == FIELD_RIGHT - PADDLE_WIDTH);
    game.state = STATE_GAME_OVER;
    game_move_paddle(&game, -127);
    assert(game.paddle_x == FIELD_RIGHT - PADDLE_WIDTH);
}

static void test_wall_bounces(void)
{
    Game game;

    playing(&game);
    game.ball_x = FIELD_LEFT;
    game.ball_y = 150;
    game.vx = -3 * BALL_UNIT;
    game.vy = 0;
    game_tick(&game, 0, 0);
    assert(game.vx == 3 * BALL_UNIT && game.ball_x >= FIELD_LEFT);
    assert(game.hits == HIT_WALL);
    game_tick(&game, 0, 0);
    assert(game.hits == 0);
    game.ball_x = FIELD_RIGHT - BALL_SIZE;
    game.vx = 3 * BALL_UNIT;
    game_tick(&game, 0, 0);
    assert(game.vx == -3 * BALL_UNIT);
    assert(game.ball_x + BALL_SIZE <= FIELD_RIGHT);
    game.ball_x = 100;
    game.ball_y = FIELD_TOP;
    game.vx = 0;
    game.vy = -3 * BALL_UNIT;
    game_tick(&game, 0, 0);
    assert(game.vy == 3 * BALL_UNIT && game.ball_y >= FIELD_TOP);
}

static void test_damage_and_faces(void)
{
    Game game;
    int face;

    two_bricks(&game);
    game.bricks[MID] = 3;
    for (face = 2; face >= 0; --face)
    {
        aim_up(&game);
        game_tick(&game, 0, 0);
        assert(game.bricks[MID] == face);
        assert(game.hits == HIT_BRICK && game.vy > 0);
        assert(game.score == (face == 0 ? 10 : 0));
        assert(game.bricks_left == (face == 0 ? 1 : 2));
    }
    game_tick(&game, 0, 0);
    assert(game.score == 10 && game.hits == 0);

    for (face = 0; face < 3; ++face)
    {
        two_bricks(&game);
        game.vx = 0;
        game.vy = 0;
        game.ball_y = MID_Y;
        if (face == 0)
        {
            game.ball_x = MID_X - BALL_SIZE;
            game.vx = 3 * BALL_UNIT;
        }
        else if (face == 1)
        {
            game.ball_x = MID_X + BRICK_WIDTH;
            game.vx = -3 * BALL_UNIT;
        }
        else
        {
            game.ball_x = MID_X + 4;
            game.ball_y = MID_Y - BALL_SIZE;
            game.vy = 3 * BALL_UNIT;
        }
        game_tick(&game, 0, 0);
        assert(game.bricks[MID] == 0 && game.score == 10);
        if (face == 0)
        {
            assert(game.vx < 0 && game.ball_x + BALL_SIZE <= MID_X);
        }
        else if (face == 1)
        {
            assert(game.vx > 0 && game.ball_x >= MID_X + BRICK_WIDTH);
        }
        else
        {
            assert(game.vy < 0 && game.ball_y + BALL_SIZE <= MID_Y);
        }
    }
}

static void test_neighbours_and_corner(void)
{
    Game game;

    two_bricks(&game);
    game.bricks[MID + 1] = 1;
    game.bricks_left = 3;
    aim_up(&game);
    game.ball_x = MID_X + BRICK_WIDTH - 4;
    game_tick(&game, 0, 0);
    assert(game.bricks[MID] == 0 && game.bricks[MID + 1] == 1);
    assert(game.bricks_left == 2);

    two_bricks(&game);
    game.ball_x = MID_X - BALL_SIZE;
    game.ball_y = MID_Y - BALL_SIZE;
    game.vx = 3 * BALL_UNIT;
    game.vy = 3 * BALL_UNIT;
    game_tick(&game, 0, 0);
    assert(game.bricks[MID] == 0 && game.score == 10);
    assert(game.ball_y + BALL_SIZE <= MID_Y);
}

static void test_fractional_motion_and_angles(void)
{
    Game game;
    int i;
    int left;
    int positions[5] = {1, 11, 21, 31, 41};
    int speeds[5] = {-40, -24, 8, 24, 40};

    for (i = 0; i < 5; ++i)
    {
        playing(&game);
        game.ball_x = game.paddle_x + positions[i] - BALL_SIZE / 2;
        game.ball_y = PADDLE_Y - BALL_SIZE;
        game.vx = 0;
        game.vy = 3 * BALL_UNIT;
        game_tick(&game, 0, 0);
        assert(game.vx == speeds[i] && game.vy < 0);
        assert(game.ball_y + BALL_SIZE <= PADDLE_Y);
        assert(game.hits == HIT_PADDLE);
    }
    for (i = -1; i <= 1; i += 2)
    {
        playing(&game);
        game.ball_y = 140;
        game.vx = i * 8;
        game.vy = 0;
        left = game.ball_x;
        game_tick(&game, 0, 0);
        assert(game.ball_x == left);
        game_tick(&game, 0, 0);
        assert(game.ball_x == left + i);
    }
}

static void test_levels_and_final_win(void)
{
    Game game;
    int level;
    int count;

    playing(&game);
    game_next_level(&game);
    assert(game.level == 0);
    for (level = 0; level < LEVEL_COUNT; ++level)
    {
        assert(game.level == level);
        count = level == 1 ? 36 : (level == 0 ? 56 : 54);
        assert(game.bricks_left == count && live_bricks(&game) == count);
        assert(game.paddle_width == PADDLE_WIDTH && game.wide_ticks == 0);
        game.score = 1230;
        game.lives = 2;
        for (count = 0; count < BRICK_COUNT; ++count)
        {
            game.bricks[count] = 0;
        }
        game.bricks[MID] = 1;
        game.bricks_left = 1;
        game.state = STATE_PLAYING;
        aim_up(&game);
        game_tick(&game, 0, 0);
        assert(game.bricks_left == 0 && game.score == 1240);
        assert(game.state == (level == 2 ? STATE_WON : STATE_LEVEL_CLEAR));
        game_tick(&game, 1, 0);
        assert(game.score == 1240 && game.lives == 2);
        game_tick(&game, 0, 1);
        assert(game.state == STATE_READY);
        if (level != 2)
        {
            assert(game.score == 1240 && game.lives == 2);
        }
    }
    assert(game.level == 0 && game.score == 0 && game.lives == 3);
}

static void test_capsule_drops_and_pool(void)
{
    Game game;
    int i;

    two_bricks(&game);
    game.drops = CAPSULE_INTERVAL - 1;
    game.bricks[MID] = 2;
    aim_up(&game);
    game_tick(&game, 0, 0);
    assert(!game.capsules[0].active);
    assert(game.drops == CAPSULE_INTERVAL - 1);
    aim_up(&game);
    game_tick(&game, 0, 0);
    assert(game.capsules[0].active && !game.capsules[1].active);
    assert(game.capsules[0].x == MID_X + 4);
    for (i = 0; i < CAPSULE_COUNT; ++i)
    {
        game.capsules[i].active = 1;
        game.capsules[i].x = FIELD_LEFT;
        game.capsules[i].y = 120;
    }
    game.bricks[MID] = 1;
    game.bricks_left = 2;
    game.drops = 2 * CAPSULE_INTERVAL - 1;
    aim_up(&game);
    game_tick(&game, 0, 0);
    assert(game.bricks[MID] == 0 && game.bricks_left == 1);
    for (i = 0; i < CAPSULE_COUNT; ++i)
    {
        assert(game.capsules[i].active && game.capsules[i].x == FIELD_LEFT);
    }
    game.capsules[1].y = FIELD_BOTTOM - 1;
    game_tick(&game, 0, 0);
    assert(!game.capsules[1].active);
}

static void catch_capsule(Game *game)
{
    game->capsules[0].active = 1;
    game->capsules[0].x = game->paddle_x;
    game->capsules[0].y = PADDLE_Y - CAPSULE_SIZE;
    game_tick(game, 0, 0);
}

static void test_wide_pickup_refresh_and_expiration(void)
{
    Game game;
    int i;

    playing(&game);
    game.ball_y = 140;
    game.vx = 0;
    game.vy = 0;
    game.paddle_x = FIELD_RIGHT - PADDLE_WIDTH;
    catch_capsule(&game);
    assert(game.hits == HIT_CAPSULE && !game.capsules[0].active);
    assert(game.paddle_width == WIDE_PADDLE_WIDTH);
    assert(game.paddle_x == FIELD_RIGHT - WIDE_PADDLE_WIDTH);
    assert(game.wide_ticks == WIDE_FRAMES);
    game_tick(&game, 0, 0);
    catch_capsule(&game);
    assert(game.wide_ticks == WIDE_FRAMES);
    for (i = 0; i < WIDE_FRAMES - 1; ++i)
    {
        game_tick(&game, 0, 0);
    }
    assert(game.wide_ticks == 1 && game.paddle_width == WIDE_PADDLE_WIDTH);
    game_tick(&game, 0, 0);
    assert(game.wide_ticks == 0 && game.paddle_width == PADDLE_WIDTH);
    assert(game.paddle_x >= FIELD_LEFT);
    assert(game.paddle_x + game.paddle_width <= FIELD_RIGHT);
}

static void test_wide_collision_and_misses(void)
{
    Game game;
    int left;

    playing(&game);
    game.ball_y = 140;
    game.vx = 0;
    game.vy = 0;
    game.paddle_x = FIELD_LEFT;
    catch_capsule(&game);
    assert(game.paddle_x == FIELD_LEFT);
    game_move_paddle(&game, 127);
    game_move_paddle(&game, 127);
    assert(game.paddle_x == FIELD_RIGHT - WIDE_PADDLE_WIDTH);
    left = game.paddle_x;
    game.ball_x = left + WIDE_PADDLE_WIDTH - BALL_SIZE;
    game.ball_y = PADDLE_Y - BALL_SIZE;
    game.vy = 3 * BALL_UNIT;
    game_tick(&game, 0, 0);
    assert(game.hits == HIT_PADDLE && game.vy < 0 && game.vx > 0);

    playing(&game);
    game.ball_y = 140;
    game.vx = 0;
    game.vy = 0;
    game.capsules[0].active = 1;
    game.capsules[0].x = game.paddle_x + PADDLE_WIDTH;
    game.capsules[0].y = PADDLE_Y - CAPSULE_SIZE;
    game_tick(&game, 0, 0);
    assert(game.capsules[0].active && game.wide_ticks == 0);
}

static void test_life_and_level_cleanup(void)
{
    Game game;
    int i;

    playing(&game);
    catch_capsule(&game);
    game.score = 40;
    game.capsules[1].active = 1;
    game.ball_y = FIELD_BOTTOM - 1;
    game.vx = 0;
    game.vy = 3 * BALL_UNIT;
    game_tick(&game, 0, 0);
    assert(game.lives == 2 && game.state == STATE_READY && game.score == 40);
    assert(game.paddle_width == PADDLE_WIDTH && game.wide_ticks == 0);
    assert(game.ball_fraction_x == 0 && game.ball_fraction_y == 0);
    assert(game.ball_x == game.paddle_x + 20);
    for (i = 0; i < CAPSULE_COUNT; ++i)
    {
        assert(!game.capsules[i].active);
    }
    game.state = STATE_PLAYING;
    game.lives = 1;
    game.ball_y = FIELD_BOTTOM - 1;
    game.vy = 3 * BALL_UNIT;
    game_tick(&game, 0, 0);
    assert(game.state == STATE_GAME_OVER && game.lives == 0);
    game_tick(&game, 0, 1);
    assert(game.state == STATE_READY && game.score == 0 && game.lives == 3);

    game.state = STATE_PLAYING;
    catch_capsule(&game);
    game.state = STATE_LEVEL_CLEAR;
    game_next_level(&game);
    assert(game.level == 1 && game.paddle_width == PADDLE_WIDTH);
    assert(game.wide_ticks == 0 && !game.capsules[0].active);
}

int main(void)
{
    test_reset_and_launch();
    test_paddle_limits();
    test_wall_bounces();
    test_damage_and_faces();
    test_neighbours_and_corner();
    test_fractional_motion_and_angles();
    test_levels_and_final_win();
    test_capsule_drops_and_pool();
    test_wide_pickup_refresh_and_expiration();
    test_wide_collision_and_misses();
    test_life_and_level_cleanup();
    puts("PASS: 11 gameplay test groups");
    return 0;
}
