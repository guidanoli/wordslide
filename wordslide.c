#include <riv.h>
#include <math.h>

#include "dictionary.h"

// macros
#define FRACTION_OF_SECOND(a, b) ((a) * riv->target_fps / (b))

// game graphics constants 
#define WS_TARGET_FPS 60
#define WS_SCREEN_SIZE 128
#define WS_SCREEN_CENTER (WS_SCREEN_SIZE / 2)
#define WS_SPRITE_SIZE 16
#define WS_MARGIN 4

// game input constants
#define WS_KEYBOARD_SPAM_DELAY FRACTION_OF_SECOND(1, 4)
#define WS_KEYBOARD_SPAM_PERIOD FRACTION_OF_SECOND(1, 30)
#define WS_INPUT_BUFFER_MAX_WORD_LENGTH 18

// game logic constants
#define WS_MAX_WORD_COUNT 16
#define WS_MAX_HEARTS 10
#define WS_GAMEOVER_DELAY 3
#define WS_HEALING_WORD_PROBABILITY 0.2

enum sprite_id
{
    WS_SPRITE_HEART = 0,
};

enum game_state_t
{
    WS_GAMESTATE_TITLESCREEN,
    WS_GAMESTATE_ONGOING,
    WS_GAMESTATE_END,
};

struct word_object_t
{
    const char* word;             // the word itself. NULL if nonexistent.
    uint64_t creation_frame;      // frame in which the word was created
    uint32_t lifespan_in_frames;  // number of frames the word lives for
    int64_t x;                    // x coordinate (random)
    bool is_healing;              // whether the word gives an extra heart
};

// game logic variables
enum game_state_t game_state = WS_GAMESTATE_TITLESCREEN;
char input_buffer[WS_INPUT_BUFFER_MAX_WORD_LENGTH + 1] = {'\0'};
uint8_t input_buffer_length = 0;
struct word_object_t word_objects[WS_MAX_WORD_COUNT] = {{.word = NULL}};
uint64_t next_frame_to_spawn_word = 0;
uint64_t correct_letters = 0;
uint64_t hearts = WS_MAX_HEARTS;
uint64_t healing_words = 0;
uint64_t start_frame = 0;

// game graphics variables
int sprites_id;

uint64_t uint64_min(uint64_t a, uint64_t b)
{
    return (a < b) ? a : b;
}

void check()
{
    if (ws_dictionary_max_word_length > WS_INPUT_BUFFER_MAX_WORD_LENGTH)
    {
        riv_panic(riv_tprintf("The dictionary contains a word of length %d, which is longer than user input buffer length %d.\n"
                              "Either remove such word from the dictionary or raise the buffer length accordingly.\n",
                              ws_dictionary_max_word_length, WS_INPUT_BUFFER_MAX_WORD_LENGTH));
    }
}

void setup()
{
    riv->width = WS_SCREEN_SIZE;
    riv->height = WS_SCREEN_SIZE;
    riv->target_fps = WS_TARGET_FPS;

    riv->tracked_keys[RIV_KEYCODE_BACKSPACE] = true;

    for (riv_key_code keycode = RIV_KEYCODE_A; keycode <= RIV_KEYCODE_Z; ++keycode)
    {
        riv->tracked_keys[keycode] = true;
    }

    riv->tracked_keys[RIV_KEYCODE_LEFT_CTRL] = true;
    riv->tracked_keys[RIV_KEYCODE_RIGHT_CTRL] = true;

    sprites_id = riv_make_spritesheet(riv_make_image("sprites.png", 255), WS_SPRITE_SIZE, WS_SPRITE_SIZE);
}

bool is_key_triggered(riv_key_state keystate)
{
    if (keystate.press)
    {
        return true;
    }
    else if (keystate.down)
    {
        uint64_t frames_since_down = riv->frame - keystate.down_frame;

        if (frames_since_down >= WS_KEYBOARD_SPAM_DELAY)
        {
            return (frames_since_down - WS_KEYBOARD_SPAM_DELAY) % WS_KEYBOARD_SPAM_PERIOD == 0;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

bool is_any_letter_key_pressed(char *ch)
{
    for (riv_key_code keycode = RIV_KEYCODE_A; keycode <= RIV_KEYCODE_Z; ++keycode)
    {
        if (is_key_triggered(riv->keys[keycode]))
        {
            *ch = (keycode - RIV_KEYCODE_A) + 'a';
            return true;
        }
    }

    return false;
}

bool try_pushing_char_to_input_buffer(char ch)
{
    if (input_buffer_length < WS_INPUT_BUFFER_MAX_WORD_LENGTH)
    {
        input_buffer[input_buffer_length++] = ch;
        input_buffer[input_buffer_length] = '\0';
        return true;
    }
    else
    {
        return false;
    }
}

bool try_popping_char_from_input_buffer()
{
    if (input_buffer_length > 0)
    {
        input_buffer[--input_buffer_length] = '\0';
        return true;
    }
    else
    {
        return false;
    }
}

void clear_input_buffer()
{
    input_buffer_length = 0;
    input_buffer[0] = '\0';
}

void handle_keypresses()
{
    char ch;

    if (is_any_letter_key_pressed(&ch))
    {
        try_pushing_char_to_input_buffer(ch);
    }
    else if (is_key_triggered(riv->keys[RIV_KEYCODE_BACKSPACE]))
    {
        if (riv->keys[RIV_KEYCODE_LEFT_CTRL].down || riv->keys[RIV_KEYCODE_RIGHT_CTRL].down)
        {
            clear_input_buffer();
        }
        else
        {
            try_popping_char_from_input_buffer();
        }
    }
}

int64_t mystrlen(const char* s)
{
    int64_t n = 0;

    while (*s != '\0')
    {
        ++n;
        ++s;
    }

    return n;
}

double calculate_word_lifespan()
{
    double a = 4.0;
    double b = 2.0;
    double c = 30.0 * riv->target_fps;
    double t = riv->frame - start_frame;
    return a + b * exp(-t/c);
}

double calculate_word_spawn_period()
{
    double a = 1.0;
    double b = 1.0;
    double c = 30.0 * riv->target_fps;
    double t = riv->frame - start_frame;
    return a + b * exp(-t/c);
}

void try_to_spawn_word(const char* new_word)
{
    for (int i = 0; i < WS_MAX_WORD_COUNT; ++i)
    {
        if (word_objects[i].word == NULL)
        {
            int64_t len = mystrlen(new_word);
            int64_t w = 5 * len;
            bool is_healing = ((hearts + healing_words) < WS_MAX_HEARTS) && (riv_rand_float() < WS_HEALING_WORD_PROBABILITY);

            word_objects[i] = (struct word_object_t){
                    new_word,
                    riv->frame,
                    riv->target_fps * calculate_word_lifespan(),
                    riv_rand_int(WS_MARGIN + w / 2 + 1, WS_SCREEN_SIZE - WS_MARGIN - w / 2 - 2),
                    is_healing,
            };

            if (is_healing)
            {
                ++healing_words;
            }

            break;
        }
    }
}

const char* pick_random_word_from_dictionary()
{
    return ws_dictionary[riv_rand_uint(ws_dictionary_word_count - 1)];
}

void handle_spawns()
{
    if (riv->frame >= next_frame_to_spawn_word)
    {
        try_to_spawn_word(pick_random_word_from_dictionary());
        next_frame_to_spawn_word = riv->frame + riv->target_fps * calculate_word_spawn_period();
    }
}

bool streq(const char* a, const char* b)
{
    while (true)
    {
        if (*a == *b)
        {
            if (*a == '\0')
            {
                return true;
            }
            else
            {
                ++a;
                ++b;
            }
        }
        else
        {
            return false;
        }
    }
}

void delete_word_object(struct word_object_t* word_obj)
{
    if (word_obj->is_healing)
    {
        --healing_words;
    }

    word_obj->word = NULL;
}

void handle_matches()
{
    bool found_match = false;

    for (int i = 0; i < WS_MAX_WORD_COUNT; ++i)
    {
        struct word_object_t* word_obj = &word_objects[i];
        const char* word = word_obj->word;

        if (word != NULL && streq(word, input_buffer))
        {
            found_match = true;
            correct_letters += input_buffer_length;

            if (word_obj->is_healing && hearts < WS_MAX_HEARTS)
            {
                ++hearts;
            }

            delete_word_object(word_obj);
        }
    }

    if (found_match)
    {
        clear_input_buffer();
    }
}

void handle_purges()
{
    for (int i = 0; i < WS_MAX_WORD_COUNT; ++i)
    {
        struct word_object_t* word_obj = &word_objects[i];
        const char* word = word_obj->word;

        if (word != NULL)
        {
            uint64_t creation_frame = word_obj->creation_frame;
            uint32_t lifespan_in_frames = word_obj->lifespan_in_frames;

            if (riv->frame >= creation_frame + lifespan_in_frames)
            {
                if (hearts > 0)
                {
                    --hearts;
                }

                delete_word_object(word_obj);
            }
        }
    }
}

void end_game()
{
    game_state = WS_GAMESTATE_END;
    riv->stop_frame = riv->frame + WS_GAMEOVER_DELAY * riv->target_fps;
}

void check_hearts()
{
    if (hearts == 0)
    {
        end_game();
    }
}

void update()
{
    if (game_state == WS_GAMESTATE_TITLESCREEN)
    {
        if (riv->key_toggle_count > 0)
        {
            game_state = WS_GAMESTATE_ONGOING;
            start_frame = riv->frame + 1;
        }
    }
    else if (game_state == WS_GAMESTATE_ONGOING)
    {
        handle_keypresses();
        handle_spawns();
        handle_matches();
        handle_purges();
        check_hearts();
    }
}

void draw_title_screen()
{
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

int64_t draw_input_buffer()
{
    riv_recti rect = riv_draw_text(
            input_buffer,
            RIV_SPRITESHEET_FONT_5X7,
            RIV_BOTTOMLEFT,
            WS_MARGIN,
            WS_SCREEN_SIZE - WS_MARGIN,
            1,
            RIV_COLOR_LIGHTBLUE);

    if (((riv->frame - start_frame) / riv->target_fps) % 2 == 0)
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

    return WS_SCREEN_SIZE - WS_MARGIN - 7 - WS_MARGIN;
}

void draw_sliding_words(int64_t input_buffer_min_y)
{
    for (int i = 0; i < WS_MAX_WORD_COUNT; ++i)
    {
        struct word_object_t* word_obj = &word_objects[i];
        const char* word = word_obj->word;

        if (word != NULL)
        {
            uint64_t creation_frame = word_obj->creation_frame;
            uint32_t lifespan_in_frames = word_obj->lifespan_in_frames;
            bool is_healing = word_obj->is_healing;

            int64_t y = (input_buffer_min_y * (riv->frame - creation_frame)) / lifespan_in_frames;

            riv_draw_text(
                    word,
                    RIV_SPRITESHEET_FONT_5X7,
                    RIV_CENTER,
                    word_objects[i].x,
                    y,
                    1,
                    is_healing ? RIV_COLOR_PINK : RIV_COLOR_LIGHTGREY);
        }
    }
}

void draw_heart()
{
    riv_draw_sprite(
            WS_SPRITE_HEART,
            sprites_id,
            WS_SCREEN_SIZE - WS_SPRITE_SIZE - WS_MARGIN,
            WS_MARGIN,
            1, 1, 1, 1);

    riv_draw_text(
            riv_tprintf("%d", hearts),
            RIV_SPRITESHEET_FONT_3X5,
            RIV_CENTER,
            WS_SCREEN_SIZE - WS_SPRITE_SIZE / 2 - WS_MARGIN,
            WS_MARGIN + WS_SPRITE_SIZE / 2,
            1,
            RIV_COLOR_DARKRED);
}

void draw_game_over()
{
    riv_draw_text(
            "GAME OVER",
            RIV_SPRITESHEET_FONT_5X7,
            RIV_CENTER,
            WS_SCREEN_CENTER,
            WS_SCREEN_CENTER,
            2,
            RIV_COLOR_RED);
}

void draw()
{
    riv_clear(RIV_COLOR_DARKSLATE);

    if (game_state == WS_GAMESTATE_TITLESCREEN)
    {
        draw_title_screen();
    }
    else if (game_state == WS_GAMESTATE_ONGOING)
    {
        int64_t sliding_words_max_y = draw_input_buffer();
        draw_sliding_words(sliding_words_max_y);
        draw_heart();
    }
    else if (game_state == WS_GAMESTATE_END)
    {
        draw_game_over();
    }
}

int main()
{
    check();
    setup();

    do
    {
        update();
        draw();
    }
    while (riv_present());

    return 0;
}
