#include <stdint.h>
#include <stddef.h>
#include "driver/rmt.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "rgb.h"

/* Encode a single bit into RMT items */
static void encode_bit(uint8_t bit, rmt_item32_t *item)
{
    if (bit == 1U)
    {
        item->level0 = 1U;
        item->duration0 = (uint32_t)T1H_DURATION;
        item->level1 = 0U;
        item->duration1 = (uint32_t)T1L_DURATION;
    }
    else
    {
        item->level0 = 1U;
        item->duration0 = (uint32_t)T0H_DURATION;
        item->level1 = 0U;
        item->duration1 = (uint32_t)T0L_DURATION;
    }
}

/* Send reset signal to the LED */
static void send_reset_signal(void)
{
    rmt_item32_t reset_item = {
        .level0 = 0U,
        .duration0 = (uint32_t)RESET_DURATION,
        .level1 = 0U,
        .duration1 = 0U
    };

    esp_err_t err = rmt_write_items(RMT_TX_CHANNEL, &reset_item, 1U, true);
    if (err != ESP_OK)
    {
        ESP_LOGE("send_reset_signal", "Failed to send reset: %d", err);
    }
}

/* RMT initialization */
void init_rmt(void)
{
    rmt_config_t config = {
        .rmt_mode = RMT_MODE_TX,
        .channel = RMT_TX_CHANNEL,
        .gpio_num = LED_GPIO_PIN,
        .clk_div = RMT_CLK_DIV,
        .mem_block_num = 1U,
        .flags = 0U
    };

    esp_err_t err = rmt_config(&config);
    if (err != ESP_OK)
    {
        ESP_LOGE("init_rmt", "RMT config failed: %d", err);
    }

    err = rmt_driver_install(config.channel, 0U, 0U);
    if (err != ESP_OK)
    {
        ESP_LOGE("init_rmt", "RMT driver install failed: %d", err);
    }
}

/* Set RGB LED color using PWM */
void set_led_channels(uint8_t red, uint8_t green, uint8_t blue)
{
    rmt_item32_t items[BITS_PER_LED] = { 0 };

    /* Encode bits for Green, Red, and Blue in GRB order */
    uint8_t colors[3U] = { green, red, blue };
    uint8_t index = 0U;

    for (uint8_t channel = 0U; channel < 3U; channel++)
    {
        for (uint8_t bit = 0U; bit < 8U; bit++)
        {
            encode_bit((colors[channel] >> (7U - bit)) & 0x1U, &items[index]);
            index++;
        }
    }

    /* Send encoded signal */
    esp_err_t err = rmt_write_items(RMT_TX_CHANNEL, items, BITS_PER_LED, true);
    if (err != ESP_OK)
    {
        ESP_LOGE("set_led_pwm", "Failed to write items: %d", err);
    }

    /* Reset signal to latch the data */
    send_reset_signal();
}

void set_led_rgb(RGB rgb)
{
    set_led_channels(rgb.red, rgb.green, rgb.blue);
}

uint8_t morph_set_sequence(Morph *morph, RGB *rgb_list, uint8_t rgb_list_size, int64_t morph_step_time_us)
{
    if (rgb_list == NULL)   { ESP_LOGE("set_morph_sequence", "RGB List is NULL"); return 1; }
    if (morph == NULL)      { ESP_LOGE("set_morph_sequence", "Morph obj is NULL"); return 1; }
    if (rgb_list_size <= 0U) { ESP_LOGE("set_morph_sequence", "RGB size is <= 0"); return 1; }

    if (morph_step_time_us < 0) { morph_step_time_us = 0U; }

    morph->color_list = rgb_list;
    morph->list_size = rgb_list_size;
    morph->list_index = 0U;
    morph->current_color.hex = rgb_list->hex;
    morph->target_color.hex = rgb_list->hex;
    morph->morph_step = morph_step_time_us; /* convert step time to us */
    morph->last_tick = esp_timer_get_time();

    return 0;
}

void morph_tick(Morph *morph)
{

    if (esp_timer_get_time() - morph->last_tick >= morph->morph_step)
    {
        if (morph->current_color.hex == morph->target_color.hex)
        {
            morph->list_index++;
            if (morph->list_index >= morph->list_size) { morph->list_index = 0; } /* wrap index back to the start */
            morph->target_color = *(morph->color_list + morph->list_index); /* get current target color from rgb list */
        }
        else
        {
            if (morph->current_color.red   > morph->target_color.red)   { morph->current_color.red--; }
            if (morph->current_color.red   < morph->target_color.red)   { morph->current_color.red++; }
            if (morph->current_color.green > morph->target_color.green) { morph->current_color.green--; }
            if (morph->current_color.green < morph->target_color.green) { morph->current_color.green++; }
            if (morph->current_color.blue  > morph->target_color.blue)  { morph->current_color.blue--; }
            if (morph->current_color.blue  < morph->target_color.blue)  { morph->current_color.blue++; }
            set_led_rgb(morph->current_color);
        }
        morph->last_tick = esp_timer_get_time();
    }
}