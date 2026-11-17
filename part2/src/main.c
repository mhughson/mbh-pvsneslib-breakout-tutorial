#include <snes.h>
#include "game.h"
#include "res/soundbank.h"

#define PLAYFIELD_MAP 0x6C00
#define BACKDROP_MAP 0x7000
#define EMPTY_TILE 0x0800
#define BRICK_TILE_X (BRICK_X / 8)
#define BRICK_TILE_Y (BRICK_Y / 8)

#define GLIMMER_FRAMES 5
#define GLIMMER_TICKS 3

#define SFX_TADA 0
#define SFX_STRINGS 1
#define SFX_PIANO 2
#define SFX_MARIMBA 3
#define SFX_COWBELL 4

extern char SOUNDBANK__;

extern u8 paddle_tiles[];
extern u8 ball_tiles[];
extern u8 sprite_palette[];
extern u8 backdrop_tiles[], backdrop_tiles_end[];
extern u8 backdrop_map[];
extern u8 backdrop_palette[];
extern u8 playfield_tiles[], playfield_tiles_end[];
extern u16 playfield_map[];
extern u8 playfield_palette[];

Game game;

static u16 brick_map[BRICK_ROWS][BRICK_COLS * 2];
static unsigned char row_changed[BRICK_ROWS];
static unsigned char shown_alive[BRICK_COUNT];
static char score_text[] = "000";
static char lives_text[] = "3";
static int shown_bricks = -1;
static int shown_score = -1;
static int shown_lives = -1;
static int shown_state = -1;
static int paddle_glimmer;
static int ball_glimmer;

static void prepare_bricks(void)
{
    int index;
    int row;
    int col;
    int tile;

    if (shown_bricks == game.bricks_left)
        return;

    index = 0;
    for (row = 0; row < BRICK_ROWS; ++row) {
        tile = (BRICK_TILE_Y + row) * 32 + BRICK_TILE_X;
        for (col = 0; col < BRICK_COLS * 2; col += 2) {
            if (shown_alive[index] != game.bricks[index]) {
                shown_alive[index] = game.bricks[index];
                if (game.bricks[index]) {
                    brick_map[row][col] = playfield_map[tile + col];
                    brick_map[row][col + 1] = playfield_map[tile + col + 1];
                } else {
                    brick_map[row][col] = EMPTY_TILE;
                    brick_map[row][col + 1] = EMPTY_TILE;
                }
                row_changed[row] = 1;
            }
            ++index;
        }
    }
    shown_bricks = game.bricks_left;
}

static void upload_bricks(void)
{
    int row;

    /* One row per frame keeps the copy inside vblank. */
    for (row = 0; row < BRICK_ROWS; ++row) {
        if (row_changed[row]) {
            dmaCopyVram((u8 *)brick_map[row],
                        PLAYFIELD_MAP + (BRICK_TILE_Y + row) * 32 + BRICK_TILE_X,
                        BRICK_COLS * 4);
            row_changed[row] = 0;
            return;
        }
    }
}

static void draw_text(void)
{
    if (shown_score != game.score) {
        score_text[0] = '0' + game.score / 100;
        score_text[1] = '0' + (game.score / 10) % 10;
        score_text[2] = '0' + game.score % 10;
        consoleDrawText(8, 1, score_text);
        shown_score = game.score;
    }
    if (shown_lives != game.lives) {
        lives_text[0] = '0' + game.lives;
        consoleDrawText(20, 1, lives_text);
        shown_lives = game.lives;
    }

    if (shown_state == game.state)
        return;

    consoleDrawText(3, 18, "                          ");
    consoleDrawText(3, 20, "                          ");
    if (game.state == STATE_READY) {
        consoleDrawText(7, 18, "B / START TO LAUNCH");
        consoleDrawText(6, 20, "LEFT / RIGHT TO MOVE");
    } else if (game.state == STATE_GAME_OVER) {
        consoleDrawText(11, 18, "GAME OVER");
        consoleDrawText(7, 20, "B / START: NEW GAME");
    } else if (game.state == STATE_WON) {
        consoleDrawText(12, 18, "YOU WIN!");
        consoleDrawText(7, 20, "B / START: NEW GAME");
    }
    shown_state = game.state;
}

static int glimmer_frame(int *timer)
{
    if (*timer == 0)
        return 0;
    --*timer;
    return GLIMMER_FRAMES - *timer / GLIMMER_TICKS;
}

static void show_frame(int id, int frame)
{
    if (oambuffer[id].oamframeid != frame) {
        oambuffer[id].oamframeid = frame;
        oambuffer[id].oamrefresh = 1;
    }
}

static void draw_sprites(void)
{
    int i;
    int paddle_frame;
    int ball_frame;

    if (game.hits & HIT_PADDLE)
        paddle_glimmer = GLIMMER_FRAMES * GLIMMER_TICKS;
    if (game.hits)
        ball_glimmer = GLIMMER_FRAMES * GLIMMER_TICKS;
    paddle_frame = glimmer_frame(&paddle_glimmer);
    ball_frame = glimmer_frame(&ball_glimmer);

    oambuffer[0].oamx = game.ball_x;
    oambuffer[0].oamy = game.ball_y;
    show_frame(0, ball_frame);
    oamDynamic8Draw(0);

    for (i = 0; i < 3; ++i) {
        oambuffer[i + 1].oamx = game.paddle_x + i * 16;
        oambuffer[i + 1].oamy = PADDLE_Y;
        show_frame(i + 1, i * 8 + paddle_frame);
        oamDynamic16Draw(i + 1);
    }
}

static void play_effect(int sound, int pitch)
{
    spcEffect(pitch, sound, 15 * 16 + 8);
}

static void start_music(void)
{
    int i;

    spcStop();
    spcLoad(MOD_POLLEN8);
    for (i = 0; i < 5; ++i)
        spcLoadEffect(i);
    spcPlay(0);
}

static void play_sounds(int old_state)
{
    if (game.state != old_state) {
        if (game.state == STATE_PLAYING) {
            play_effect(SFX_PIANO, 4);
        } else if (game.state == STATE_GAME_OVER) {
            spcStop();
            play_effect(SFX_STRINGS, 2);
        } else if (game.state == STATE_WON) {
            spcStop();
            play_effect(SFX_TADA, 4);
        } else if (old_state == STATE_PLAYING) {
            play_effect(SFX_STRINGS, 3);
        } else {
            start_music();
        }
    } else if (game.hits & HIT_BRICK) {
        play_effect(SFX_COWBELL, 4);
    } else if (game.hits & HIT_PADDLE) {
        play_effect(SFX_MARIMBA, 4);
    } else if (game.hits & HIT_WALL) {
        play_effect(SFX_MARIMBA, 6);
    }
}

static void init_sprite(int id, u8 *graphics, int frame, int size)
{
    oambuffer[id].oamframeid = frame;
    oambuffer[id].oamattribute = OBJ_PAL(0) | OBJ_PRIO(3) | size;
    oambuffer[id].oamgraphics = graphics;
    oambuffer[id].oamrefresh = 1;
}

static void init_video(void)
{
    int row;
    int col;
    int i;

    setScreenOff();
    consoleInitDefaultText(1);
    bgSetGfxPtr(0, 0x3000);
    bgSetMapPtr(0, 0x6800, SC_32x32);
    bgInitTileSet(1, playfield_tiles, playfield_palette, 2,
                  playfield_tiles_end - playfield_tiles, 16 * 2,
                  BG_16COLORS, 0x4000);
    bgInitMapSet(1, (u8 *)playfield_map, 32 * 32 * 2, SC_32x32,
                 PLAYFIELD_MAP);
    bgInitTileSet(2, backdrop_tiles, backdrop_palette, 0,
                  backdrop_tiles_end - backdrop_tiles, 4 * 2,
                  BG_4COLORS, 0x5000);
    bgInitMapSet(2, backdrop_map, 32 * 32 * 2, SC_32x32, BACKDROP_MAP);

    oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);
    setPalette(sprite_palette, 128, 16 * 2);
    paddle_glimmer = 0;
    ball_glimmer = 0;
    init_sprite(0, ball_tiles, 0, OBJ_SIZES);
    for (i = 0; i < 3; ++i)
        init_sprite(i + 1, paddle_tiles, i * 8, OBJ_SIZEL);

    setMode(BG_MODE1, 0);

    for (row = 0; row < BRICK_ROWS; ++row) {
        row_changed[row] = 0;
        for (col = 0; col < BRICK_COLS * 2; ++col)
            brick_map[row][col] =
                playfield_map[(BRICK_TILE_Y + row) * 32 + BRICK_TILE_X + col];
    }
    for (i = 0; i < BRICK_COUNT; ++i)
        shown_alive[i] = 1;
    shown_bricks = BRICK_COUNT;

    consoleDrawText(2, 1, "SCORE       LIVES");
    draw_text();
    setScreenOn();
}

int main(void)
{
    u16 held;
    u16 pressed;
    int movement;
    int old_state;

    spcBoot();
    spcSetBank(&SOUNDBANK__);

    game_reset(&game);
    init_video();
    start_music();

    while (1) {
        held = padsCurrent(0);
        pressed = padsDown(0);
        movement = 0;
        if (held & KEY_LEFT)
            --movement;
        if (held & KEY_RIGHT)
            ++movement;
        old_state = game.state;
        game_tick(&game, movement, pressed & (KEY_B | KEY_START));

        play_sounds(old_state);
        draw_sprites();
        prepare_bricks();
        draw_text();

        spcProcess();
        oamInitDynamicSpriteEndFrame();
        WaitForVBlank();
        oamVramQueueUpdate();
        upload_bricks();
    }
    return 0;
}
