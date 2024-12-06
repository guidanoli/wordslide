#include <riv.h>

#include "dictionary.h"

// game graphics constants 
#define WS_TARGET_FPS 60
#define WS_SCREEN_SIZE 128
#define WS_SCREEN_CENTER (WS_SCREEN_SIZE / 2)
#define WS_KEYBOARD_MARGIN 4
#define WS_TYPED_WORD_MARGIN 2

// game logic constants
#define WS_MAX_WORD_COUNT 16
#define WS_MAX_WORD_LENGTH 16
#define WS_KEYBOARD_NROWS 3
#define WS_KEYBOARD_NCOLS 10

enum game_state_t
{
    WS_GAMESTATE_TITLESCREEN,
    WS_GAMESTATE_ONGOING,
};

struct word_object_t
{
    const char* word;             // the word itself. NULL if nonexistent.
    uint32_t creation_frame;      // frame in which the word was created
    uint32_t duration_in_frames;  // number of frames the word lives for
};

const char keyboard[WS_KEYBOARD_NROWS][WS_KEYBOARD_NCOLS] = {
    {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'},
    {'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', '\0'},
    {'Z', 'X', 'C', 'V', 'B', 'N', 'M', '\0', '\0', '\0'},
};

// game logic variables
enum game_state_t game_state = WS_GAMESTATE_TITLESCREEN;
riv_vec2i curr_key_pos = {4, 1}; // G
char curr_typed_word[WS_MAX_WORD_LENGTH + 1] = {'\0'};
uint8_t curr_typed_word_length = 0;
struct word_object_t word_objects[WS_MAX_WORD_COUNT] = {{.word = NULL}};
uint64_t next_frame_to_spawn_word = 0;
uint64_t seconds_per_word = 3;
uint64_t words_missed = 0;

uint64_t uint64_min(uint64_t a, uint64_t b) {
    return (a < b) ? a : b;
}

void setup() {
    riv->width = WS_SCREEN_SIZE;
    riv->height = WS_SCREEN_SIZE;
    riv->target_fps = WS_TARGET_FPS;
    riv->tracked_keys[RIV_KEYCODE_UP] = true;
    riv->tracked_keys[RIV_KEYCODE_DOWN] = true;
    riv->tracked_keys[RIV_KEYCODE_LEFT] = true;
    riv->tracked_keys[RIV_KEYCODE_RIGHT] = true;
    riv->tracked_keys[RIV_KEYCODE_X] = true;
    riv->tracked_keys[RIV_KEYCODE_Z] = true;
}

bool was_any_arrow_key_pressed(riv_vec2i* dir)
{
    if (riv->keys[RIV_KEYCODE_UP].press)
    {
        dir->x = 0;
        dir->y = -1;
        return true;
    }
    else if (riv->keys[RIV_KEYCODE_DOWN].press)
    {
        dir->x = 0;
        dir->y = 1;
        return true;
    }
    else if (riv->keys[RIV_KEYCODE_LEFT].press)
    {
        dir->x = -1;
        dir->y = 0;
        return true;
    }
    else if (riv->keys[RIV_KEYCODE_RIGHT].press)
    {
        dir->x = 1;
        dir->y = 0;
        return true;
    }
    else
    {
        return false;
    }
}

char get_key_at(riv_vec2i pos)
{
    return keyboard[pos.y][pos.x];
}

riv_vec2i find_next_valid_key_pos_in_direction(riv_vec2i key_pos, riv_vec2i dir)
{
    do
    {
        key_pos.x = (key_pos.x + WS_KEYBOARD_NCOLS + dir.x) % WS_KEYBOARD_NCOLS;
        key_pos.y = (key_pos.y + WS_KEYBOARD_NROWS + dir.y) % WS_KEYBOARD_NROWS;
    }
    while (get_key_at(key_pos) == '\0');

    return key_pos;
}

void handle_keypresses()
{
    riv_vec2i dir;

    if (was_any_arrow_key_pressed(&dir))
    {
        curr_key_pos = find_next_valid_key_pos_in_direction(curr_key_pos, dir);
    }
    else if (riv->keys[RIV_KEYCODE_X].press)
    {
        if (curr_typed_word_length < WS_MAX_WORD_LENGTH)
        {
            curr_typed_word[curr_typed_word_length++] = get_key_at(curr_key_pos);
            curr_typed_word[curr_typed_word_length] = '\0';
        }
    }
    else if (riv->keys[RIV_KEYCODE_Z].press)
    {
        if (curr_typed_word_length > 0)
        {
            curr_typed_word[--curr_typed_word_length] = '\0';
        }
    }
}

void try_to_spawn_word(const char* new_word)
{
    for (int i = 0; i < WS_MAX_WORD_COUNT; ++i)
    {
        if (word_objects[i].word == NULL)
        {
            word_objects[i] = (struct word_object_t){new_word, riv->frame, riv->target_fps * seconds_per_word};
            riv_printf("word created: %s\n", word_objects[i].word);
            break;
        }
    }
}

const char* pick_random_word_from_dictionary()
{
    return ws_dictionary[riv_rand_uint(ws_dictionary_length - 1)];
}

void handle_spawns()
{
    if (riv->frame >= next_frame_to_spawn_word)
    {
        try_to_spawn_word(pick_random_word_from_dictionary());
        next_frame_to_spawn_word = riv->frame + riv_rand_int(2 * riv->target_fps, 4 * riv->target_fps);
    }
}

void handle_purges()
{
    for (int i = 0; i < WS_MAX_WORD_COUNT; ++i)
    {
        if (word_objects[i].word != NULL &&
            riv->frame >= word_objects[i].creation_frame + word_objects[i].duration_in_frames)
        {
            riv_printf("word purged: %s\n", word_objects[i].word);
            word_objects[i].word = NULL;
            ++words_missed;
        }
    }
}

void update()
{
    if (game_state == WS_GAMESTATE_TITLESCREEN)
    {
        if (riv->key_toggle_count > 0)
        {
            game_state = WS_GAMESTATE_ONGOING;
        }
    }
    else if (game_state == WS_GAMESTATE_ONGOING)
    {
        handle_keypresses();
        handle_spawns();
        handle_purges();
    }
}

void draw_title_screen()
{
    riv_clear(RIV_COLOR_DARKSLATE);

    int title_font_height = 7;
    int title_size_multipler = 2;
    int title_target_y = WS_SCREEN_CENTER - title_font_height * title_size_multipler / 2;

    riv_draw_text(
            "wordslide",
            RIV_SPRITESHEET_FONT_5X7,
            RIV_CENTER,
            WS_SCREEN_CENTER,
            uint64_min(title_target_y, riv->frame / 2), // stop in the center
            title_size_multipler,
            RIV_COLOR_GREEN);

    int title_margin = 5;

    riv_draw_text(
            "press any key to play",
            RIV_SPRITESHEET_FONT_3X5,
            RIV_CENTER,
            WS_SCREEN_CENTER,
            title_target_y + title_font_height * title_size_multipler + title_margin,
            1,
            ((3 * riv->frame / riv->target_fps) % 4 != 3) ? RIV_COLOR_RED : RIV_COLOR_PINK); // fast blink
}

int64_t draw_keyboard()
{
    int64_t y = WS_SCREEN_SIZE - WS_KEYBOARD_MARGIN;

    for (int i = WS_KEYBOARD_NROWS - 1; i >= 0; --i)
    {
        int64_t x = WS_KEYBOARD_MARGIN + 2 * i;
        int64_t maxh = 0;

        for (int j = 0; j < WS_KEYBOARD_NCOLS; ++j)
        {
            char key = keyboard[i][j];

            if (key == '\0') break; // end of row

            const char text[2] = {key, '\0'};

            riv_recti rect = riv_draw_text(
                    text,
                    RIV_SPRITESHEET_FONT_5X7,
                    RIV_BOTTOMLEFT,
                    x,
                    y,
                    1,
                    (curr_key_pos.x == j && curr_key_pos.y == i) ? RIV_COLOR_YELLOW : RIV_COLOR_WHITE);

            x += rect.width + 2;

            if (rect.height > maxh)
            {
                maxh = rect.height;
            }
        }

        y -= maxh + 2;
    }

    return y;
}

int64_t draw_typed_word(uint64_t keyboard_min_y)
{
    riv_recti rect = riv_draw_text(
            curr_typed_word,
            RIV_SPRITESHEET_FONT_5X7,
            RIV_BOTTOMLEFT,
            WS_KEYBOARD_MARGIN,
            keyboard_min_y - WS_TYPED_WORD_MARGIN,
            1,
            RIV_COLOR_LIGHTBLUE);

    if ((riv->frame / riv->target_fps) % 2 == 0)
    {
        riv_draw_text(
                "_",
                RIV_SPRITESHEET_FONT_5X7,
                RIV_BOTTOMLEFT,
                rect.x + rect.width + ((rect.width == 0) ? 0 : 1),
                rect.y + rect.height,
                1,
                RIV_COLOR_LIGHTBLUE);
    }

    return keyboard_min_y - 7 - 2 * WS_TYPED_WORD_MARGIN;
}

void draw_sliding_words(int64_t sliding_words_max_y)
{
    for (int i = 0; i < WS_MAX_WORD_COUNT; ++i)
    {
        if (word_objects[i].word != NULL)
        {
            int64_t y = (sliding_words_max_y * (riv->frame - word_objects[i].creation_frame)) / (word_objects[i].duration_in_frames);
            riv_draw_text(
                    word_objects[i].word,
                    RIV_SPRITESHEET_FONT_5X7,
                    RIV_CENTER,
                    WS_SCREEN_CENTER,
                    y,
                    1,
                    RIV_COLOR_LIGHTGREY);
        }
    }
}

void draw()
{
    if (game_state == WS_GAMESTATE_TITLESCREEN)
    {
        draw_title_screen();
    }
    else if (game_state == WS_GAMESTATE_ONGOING)
    {
        riv_clear(RIV_COLOR_DARKSLATE);
        int64_t keyboard_min_y = draw_keyboard();
        int64_t sliding_words_max_y = draw_typed_word(keyboard_min_y);
        draw_sliding_words(sliding_words_max_y);
    }
}

int main()
{
    setup();

    do
    {
        update();
        draw();
    }
    while (riv_present());

    return 0;
}
