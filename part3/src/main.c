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

#define GRADIENT_BANDS 28
#define GRADIENT_LINES 8

#define MOSAIC_FRAMES 30

#define SAVE_MAGIC 0x4B42

#define PADDLE_ATTR (OBJ_PAL(0) | OBJ_PRIO(3))
#define PADDLE_PIECES(frame)                       \
    METASPR_ITEM(0, 0, (frame), PADDLE_ATTR),      \
    METASPR_ITEM(16, 0, 8 + (frame), PADDLE_ATTR), \
    METASPR_ITEM(32, 0, 16 + (frame), PADDLE_ATTR), \
    METASPR_TERM

#define SFX_TADA 0
#define SFX_STRINGS 1
#define SFX_PIANO 2
#define SFX_MARIMBA 3
#define SFX_COWBELL 4

enum Screen {
    SCREEN_TITLE,
    SCREEN_GAME
};

typedef struct SaveData {
    u16 magic;
    u16 high_score;
    u16 check;
} SaveData;

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
extern u8 title_logo_tiles[], title_logo_tiles_end[];
extern u8 title_logo_map[], title_logo_palette[];

const t_metasprite paddle_frame0[] = { PADDLE_PIECES(0) };
const t_metasprite paddle_frame1[] = { PADDLE_PIECES(1) };
const t_metasprite paddle_frame2[] = { PADDLE_PIECES(2) };
const t_metasprite paddle_frame3[] = { PADDLE_PIECES(3) };
const t_metasprite paddle_frame4[] = { PADDLE_PIECES(4) };
const t_metasprite paddle_frame5[] = { PADDLE_PIECES(5) };

const t_metasprite *paddle_frames[GLIMMER_FRAMES + 1] = {
    paddle_frame0, paddle_frame1, paddle_frame2,
    paddle_frame3, paddle_frame4, paddle_frame5
};

Game game;

static u16 brick_map[BRICK_ROWS][BRICK_COLS * 2];
static unsigned char row_changed[BRICK_ROWS];
static unsigned char shown_alive[BRICK_COUNT];
static u8 gradient_table[GRADIENT_BANDS * 5 + 1];
static char score_text[] = "000";
static char lives_text[] = "3";
static char high_score_text[] = "HIGH SCORE 000";
static int shown_bricks = -1;
static int shown_score = -1;
static int shown_lives = -1;
static int shown_state = -1;
static int shown_paused = -1;
static int paddle_glimmer;
static int ball_glimmer;
static int shown_paddle_frame;
static int screen;
static int next_screen;
static int brightness;
static int fade_step;
static int mosaic_timer;
static int paused;
static SaveData save;
static unsigned int high_score;
static int new_high_score;
static int mouse_port;
static int mouse_dx;
static int shown_mouse;

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

static void format_score(char *text, unsigned int score)
{
    text[0] = '0' + score / 100;
    text[1] = '0' + (score / 10) % 10;
    text[2] = '0' + score % 10;
}

static void draw_text(void)
{
    if (shown_score != game.score) {
        format_score(score_text, game.score);
        consoleDrawText(8, 1, score_text);
        shown_score = game.score;
    }
    if (shown_lives != game.lives) {
        lives_text[0] = '0' + game.lives;
        consoleDrawText(20, 1, lives_text);
        shown_lives = game.lives;
    }

    if (shown_state == game.state && shown_paused == paused)
        return;

    consoleDrawText(3, 18, "                          ");
    consoleDrawText(3, 20, "                          ");
    if (paused) {
        consoleDrawText(13, 18, "PAUSED");
        consoleDrawText(8, 20, "START TO RESUME");
    } else if (game.state == STATE_READY) {
        consoleDrawText(7, 18, "B / START TO LAUNCH");
        consoleDrawText(6, 20, "LEFT / RIGHT TO MOVE");
    } else if (game.state == STATE_GAME_OVER) {
        consoleDrawText(11, 18, "GAME OVER");
        consoleDrawText(7, 20, "B / START: CONTINUE");
    } else if (game.state == STATE_WON) {
        consoleDrawText(12, 18, "YOU WIN!");
        consoleDrawText(7, 20, "B / START: CONTINUE");
    }
    if (new_high_score)
        consoleDrawText(8, 22, "NEW HIGH SCORE!");
    shown_state = game.state;
    shown_paused = paused;
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

    if (shown_paddle_frame != paddle_frame) {
        shown_paddle_frame = paddle_frame;
        oambuffer[1].oamrefresh = 1;
    }
    oamMetaDrawDyn16(1, game.paddle_x, PADDLE_Y,
                     (u8 *)paddle_frames[paddle_frame], paddle_tiles,
                     OBJ_LARGE);
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
        }
    } else if (game.hits & HIT_BRICK) {
        play_effect(SFX_COWBELL, 4);
    } else if (game.hits & HIT_PADDLE) {
        play_effect(SFX_MARIMBA, 4);
    } else if (game.hits & HIT_WALL) {
        play_effect(SFX_MARIMBA, 6);
    }
}

static void load_high_score(void)
{
    consoleLoadSram((u8 *)&save, sizeof(save));
    /* New or corrupt save RAM holds random bytes, so check it first. */
    if (save.magic == SAVE_MAGIC && save.check == (save.high_score ^ 0xFFFF))
        high_score = save.high_score;
    else
        high_score = 0;
}

static void save_high_score(void)
{
    save.magic = SAVE_MAGIC;
    save.high_score = high_score;
    save.check = high_score ^ 0xFFFF;
    consoleCopySram((u8 *)&save, sizeof(save));
}

static void record_score(int old_state)
{
    if (game.state == old_state || game.score <= high_score)
        return;
    if (game.state == STATE_GAME_OVER || game.state == STATE_WON) {
        high_score = game.score;
        new_high_score = 1;
        save_high_score();
    }
}

static void init_sprite(int id, u8 *graphics, int frame, int size)
{
    oambuffer[id].oamframeid = frame;
    oambuffer[id].oamattribute = OBJ_PAL(0) | OBJ_PRIO(3) | size;
    oambuffer[id].oamgraphics = graphics;
    oambuffer[id].oamrefresh = 1;
}

static void init_gradient(void)
{
    int band;
    u16 color;
    u8 *entry;

    entry = gradient_table;
    for (band = 0; band < GRADIENT_BANDS; ++band) {
        color = RGB5(1 + band / 3, 1 + band / 9, 4 + band / 4);
        entry[0] = GRADIENT_LINES;
        entry[1] = 0;
        entry[2] = 0;
        entry[3] = color & 0xFF;
        entry[4] = color >> 8;
        entry += 5;
    }
    entry[0] = 0;
    setModeHdmaColor(HDMA_CHANNEL6, gradient_table);
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
    bgInitTileSet(1, title_logo_tiles, title_logo_palette, 3,
                  title_logo_tiles_end - title_logo_tiles, 64 * 2,
                  BG_16COLORS, 0x2000);
    dmaCopyVram(title_logo_map, 0x7400, 32 * 28 * 2);
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
    shown_paddle_frame = -1;
    init_sprite(0, ball_tiles, 0, OBJ_SIZES);

    setMode(BG_MODE1, 0);
    init_gradient();

    for (row = 0; row < BRICK_ROWS; ++row) {
        row_changed[row] = 0;
        for (col = 0; col < BRICK_COLS * 2; ++col)
            brick_map[row][col] =
                playfield_map[(BRICK_TILE_Y + row) * 32 + BRICK_TILE_X + col];
    }
    for (i = 0; i < BRICK_COUNT; ++i)
        shown_alive[i] = 1;
    shown_bricks = BRICK_COUNT;
}

static void clear_text(void)
{
    int row;

    /* consoleDrawText is slow, so only clear the rows we write to. */
    consoleDrawText(0, 1, "                                ");
    for (row = 8; row <= 24; row += 2)
        consoleDrawText(0, row, "                                ");
}

static void show_title(void)
{
    screen = SCREEN_TITLE;
    clear_text();
    bgSetGfxPtr(1, 0x2000);
    bgSetMapPtr(1, 0x7400, SC_32x32);
    bgSetEnable(1);
    consoleDrawText(10, 22, "PRESS START");
    shown_mouse = -1;
    start_music();
}

static void show_game(void)
{
    screen = SCREEN_GAME;
    clear_text();
    bgSetGfxPtr(1, 0x4000);
    bgSetMapPtr(1, PLAYFIELD_MAP, SC_32x32);
    bgSetEnable(1);
    game_reset(&game);
    paused = 0;
    new_high_score = 0;
    shown_score = -1;
    shown_lives = -1;
    shown_state = -1;
    consoleDrawText(2, 1, "SCORE       LIVES");
    draw_text();
}

static void fade_to(int target)
{
    next_screen = target;
    fade_step = -1;
}

static void update_fade(void)
{
    if (fade_step < 0 && brightness == 0) {
        /* The screen is black, so it is safe to rebuild it. */
        if (next_screen == SCREEN_TITLE)
            show_title();
        else
            show_game();
        fade_step = 1;
    } else if (fade_step != 0) {
        brightness += fade_step;
        if (brightness == 15)
            fade_step = 0;
    }
}

static void update_mosaic(void)
{
    int size;

    if (mosaic_timer == 0)
        return;
    --mosaic_timer;
    /* Grow to the biggest block size, then shrink back to normal. */
    size = mosaic_timer;
    if (size > MOSAIC_FRAMES / 2)
        size = MOSAIC_FRAMES - size;
    REG_MOSAIC = (size << 4) | MOSAIC_BG1 | MOSAIC_BG2 | MOSAIC_BG3;
}

static void set_paused(int on)
{
    paused = on;
    if (paused) {
        /* Don't replay the last hit's glimmer on every paused frame. */
        game.hits = 0;
        spcPauseMusic();
        /* Halve the backdrop and BG2/BG3; BG1 text and sprites stay bright. */
        setColorIntensity(CM_APPLY_RED | CM_APPLY_GREEN | CM_APPLY_BLUE, 0);
        setColorEffect(0, CM_SUB_MAINSUB | CM_DIV2_DIVIDE | CM_MSCR_BACK |
                              CM_MSCR_BG2 | CM_MSCR_BG3);
    } else {
        spcResumeMusic();
        setColorEffect(0, 0);
    }
}

static void read_mouse(u16 *pressed)
{
    mouse_dx = 0;
    if (mouseConnect[0])
        mouse_port = 0;
    else if (mouseConnect[1])
        mouse_port = 1;
    else {
        mouse_port = -1;
        return;
    }

    /* Bit 7 is the direction (1 = left) and bits 0-6 are the distance. */
    mouse_dx = mouse_x[mouse_port] & 0x7F;
    if (mouse_x[mouse_port] & 0x80)
        mouse_dx = -mouse_dx;
    if (mouseButton[mouse_port] & mouse_L)
        *pressed |= KEY_B;
    if (mouseButton[mouse_port] & mouse_R)
        *pressed |= KEY_START;
}

static void update_title(u16 pressed)
{
    int connected;

    connected = mouse_port >= 0;
    if (shown_mouse != connected) {
        if (connected)
            consoleDrawText(7, 24, "OR CLICK THE MOUSE");
        else
            consoleDrawText(7, 24, "                  ");
        shown_mouse = connected;
    }

    if (pressed & (KEY_B | KEY_START))
        fade_to(SCREEN_GAME);
}

static void update_game(u16 held, u16 pressed)
{
    int movement;
    int old_state;

    if ((game.state == STATE_GAME_OVER || game.state == STATE_WON) &&
        (pressed & (KEY_B | KEY_START))) {
        fade_to(SCREEN_TITLE);
        pressed = 0;
    }
    if (game.state == STATE_PLAYING && (pressed & KEY_START)) {
        set_paused(!paused);
        pressed = 0;
    }

    if (!paused) {
        game_move_paddle(&game, mouse_dx);
        movement = 0;
        if (held & KEY_LEFT)
            --movement;
        if (held & KEY_RIGHT)
            ++movement;
        old_state = game.state;
        game_tick(&game, movement, pressed & (KEY_B | KEY_START));
        if (old_state == STATE_PLAYING &&
            (game.state == STATE_READY || game.state == STATE_GAME_OVER))
            mosaic_timer = MOSAIC_FRAMES;
        play_sounds(old_state);
        record_score(old_state);
    }

    draw_sprites();
    prepare_bricks();
    draw_text();
}

int main(void)
{
    u16 held;
    u16 pressed;

    spcBoot();
    spcSetBank(&SOUNDBANK__);

    init_video();
    initMouse(MOUSE_SLOW);
    load_high_score();
    show_title();
    brightness = 0;
    fade_step = 1;
    mosaic_timer = 0;

    while (1) {
        held = padsCurrent(0);
        pressed = padsDown(0);
        read_mouse(&pressed);
        if (fade_step != 0) {
            held = 0;
            pressed = 0;
            mouse_dx = 0;
        }
        update_fade();
        if (screen == SCREEN_TITLE)
            update_title(pressed);
        else
            update_game(held, pressed);

        spcProcess();
        oamInitDynamicSpriteEndFrame();
        WaitForVBlank();
        setBrightness(brightness);
        update_mosaic();
        oamVramQueueUpdate();
        upload_bricks();
    }
    return 0;
}
