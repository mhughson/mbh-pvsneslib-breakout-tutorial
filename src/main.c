#include <snes.h>
#include "game.h"

extern u8 sprite_tiles[];
extern u8 sprite_palette[];

Game game;

static unsigned char shown_alive[BRICK_COUNT];
static char score_text[] = "000";
static char lives_text[] = "3";
static int shown_bricks = -1;
static int shown_score = -1;
static int shown_lives = -1;
static int shown_state = -1;

static void draw_bricks(void)
{
    int index;
    int id;

    if (shown_bricks == game.bricks_left)
    {
        return;
    }

    for (index = 0; index < BRICK_COUNT; ++index)
    {
        if (shown_alive[index] == game.bricks[index])
        {
            continue;
        }

        id = (index + 4) * 4;
        if (game.bricks[index])
        {
            oamSet(id, BRICK_X + (index % BRICK_COLS) * BRICK_PITCH_X,
                   BRICK_Y + (index / BRICK_COLS) * BRICK_PITCH_Y, 3, 0, 0,
                   (index / BRICK_COLS) * 2, 0);
            oamSetEx(id, OBJ_LARGE, OBJ_SHOW);
        }
        else
        {
            oamSetVisible(id, OBJ_HIDE);
        }
        shown_alive[index] = game.bricks[index];
    }
    shown_bricks = game.bricks_left;
}

static void draw_text(void)
{
    if (shown_score != game.score)
    {
        score_text[0] = '0' + game.score / 100;
        score_text[1] = '0' + (game.score / 10) % 10;
        score_text[2] = '0' + game.score % 10;
        consoleDrawText(8, 1, score_text);
        shown_score = game.score;
    }
    if (shown_lives != game.lives)
    {
        lives_text[0] = '0' + game.lives;
        consoleDrawText(20, 1, lives_text);
        shown_lives = game.lives;
    }

    if (shown_state == game.state)
    {
        return;
    }

    consoleDrawText(3, 18, "                          ");
    consoleDrawText(3, 20, "                          ");
    if (game.state == STATE_READY)
    {
        consoleDrawText(7, 18, "B / START TO LAUNCH");
        consoleDrawText(6, 20, "LEFT / RIGHT TO MOVE");
    }
    else if (game.state == STATE_GAME_OVER)
    {
        consoleDrawText(11, 18, "GAME OVER");
        consoleDrawText(7, 20, "B / START: NEW GAME");
    }
    else if (game.state == STATE_WON)
    {
        consoleDrawText(12, 18, "YOU WIN!");
        consoleDrawText(7, 20, "B / START: NEW GAME");
    }
    shown_state = game.state;
}

static void draw_game(void)
{
    int i;

    oamSetXY(0, game.ball_x, game.ball_y);
    for (i = 0; i < 3; ++i)
    {
        oamSetXY((i + 1) * 4, game.paddle_x + i * 16, PADDLE_Y);
    }
    draw_bricks();
    draw_text();
}

static void init_video(void)
{
    int row;
    int i;

    setScreenOff();
    consoleInitDefaultText(0);
    bgSetGfxPtr(0, 0x3000);
    bgSetMapPtr(0, 0x6800, SC_32x32);
    oamInitGfxSet(sprite_tiles, 1024, sprite_palette, 32, 0, 0x0000,
                  OBJ_SIZE8_L16);

    setMode(BG_MODE1, 0);
    bgSetDisable(1);
    bgSetDisable(2);
    oamClear(0, 0);
    for (i = 0; i < BRICK_COUNT; ++i)
    {
        shown_alive[i] = 0;
    }

    oamSet(0, game.ball_x, game.ball_y, 3, 0, 0, 12, 0);
    oamSetEx(0, OBJ_SMALL, OBJ_SHOW);
    for (i = 0; i < 3; ++i)
    {
        oamSet((i + 1) * 4, game.paddle_x + i * 16, PADDLE_Y, 3, 0, 0, 10, 0);
        oamSetEx((i + 1) * 4, OBJ_LARGE, OBJ_SHOW);
    }

    consoleDrawText(2, 1, "SCORE       LIVES");
    consoleDrawText(1, 3, "##############################");
    for (row = 4; row < 28; ++row)
    {
        consoleDrawText(1, row, "#");
        consoleDrawText(30, row, "#");
    }
    draw_game();
    setScreenOn();
}

int main(void)
{
    u16 held;
    u16 pressed;
    int movement;

    game_reset(&game);
    init_video();

    while (1)
    {
        WaitForVBlank();
        held = padsCurrent(0);
        pressed = padsDown(0);
        movement = 0;
        if (held & KEY_LEFT)
        {
            --movement;
        }
        if (held & KEY_RIGHT)
        {
            ++movement;
        }
        game_tick(&game, movement, pressed & (KEY_B | KEY_START));
        draw_game();
    }
    return 0;
}
