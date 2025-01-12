#ifndef _RGB_H
#define _RGB_H
#include <stdint.h>

#define RMT_TX_CHANNEL    RMT_CHANNEL_0
#define LED_GPIO_PIN      GPIO_NUM_7
#define BITS_PER_LED      24U                     /* 8 bits each for R, G, B channels */
#define RMT_CLK_DIV       2U                      /* RMT clock divider - assuming default clock ~ 80MHz APB CLK */
/* The calculation for this goes as follows: Duration (in us) * Clock (MHz) / Clock Div */
/* more info here: https://github.com/JSchaenzle/ESP32-NeoPixel-WS2812-RMT */
#define T0H_DURATION   (0.300F * 80.0F / 2.0F) /* 0.3 us */
#define T0L_DURATION   (0.800F * 80.0F / 2.0F) /* 0.8 us */
#define T1H_DURATION   (0.600F * 80.0F / 2.0F) /* 0.6 us */
#define T1L_DURATION   (0.200F * 80.0F / 2.0F) /* 0.2 us */
#define RESET_DURATION (80.00F * 80.0F / 2.0F) /* Reset signal duration ~ 80 us */

/* COLORS!!!! >w< literally HTML colors */
#define HOT_PINK       0xff2e5b
#define KUROMI_PURPLE  0xad31f5
#define SORA_BLUE      0x34b2fa
#define EVA_GREEN      0x2ef29d
#define PIKACHU_YELLOW 0xf2c01b
#define SUNSET_ORANGE  0xeb3b00
#define PLAIN_RED      0xff0000
#define SOSO_BLACK     0x000000

/* RGB struct, translates from HEX colors to RGB values */
typedef struct {
    union {
        struct {
            uint8_t blue;
            uint8_t green;
            uint8_t red;
        };
        uint32_t hex;
    };
} RGB;

/* Morph struct to manage color blending given a color sequence and a time step */
typedef struct {
    RGB current_color;
    RGB target_color;
    int64_t morph_step; /* us */
    int64_t last_tick;  /* us */
    RGB *color_list;
    uint8_t list_index;
    uint8_t list_size;
} Morph;

void init_rmt(void);
void set_led_channels(uint8_t red, uint8_t green, uint8_t blue);
void set_led_rgb(RGB rgb);
uint8_t morph_set_sequence(Morph *morph, RGB *rgb_list, uint8_t rgb_list_size, int64_t morph_step_time_us);
void morph_tick(Morph *morph);

#endif /* _RGB_H */