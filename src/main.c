#include "aroma.h"

Aroma Player;

#include "aroma_config.h"
#include "aroma_ledc.h"
#include "aroma_ledc.h"
#include "aroma_spray.h"
#include "aroma_view.h"

extern const uint8_t sleep_mp3_start[] asm("_binary_sleep_mp3_start");
extern const uint8_t sleep_mp3_end[] asm("_binary_sleep_mp3_end");

extern const uint8_t getup_mp3_start[] asm("_binary_getup_mp3_start");
extern const uint8_t getup_mp3_end[] asm("_binary_getup_mp3_end");

#include "driver/gpio.h"
static gpio_config_t init_io(gpio_num_t num)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << num);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    return io_conf;
}

void app_main(void)
{
    // gpio_config_t io_config_19 = init_io(GPIO_NUM_19);
    // gpio_config(&io_config_19);
    // gpio_set_level(GPIO_NUM_19, 1);
    // vTaskDelay(1000 / portTICK_RATE_MS);
    // gpio_set_level(GPIO_NUM_19, 0);

    aroma_init(&Player);
    aroma_music(&Player, getup_mp3_start, getup_mp3_end);
    // aroma_music(&Player, sleep_mp3_start, sleep_mp3_end);
    
    // aroma_play(&Player);
    
    // unit_test_aroma_config();
    
    unit_test_ledc_fade();
    unit_test_spray();

    unit_test_view();

    // aroma_volume(&Player, 100);
    // aroma_play(&Player);
    
    // return;
    
    // aroma_stop_time = 15 * 1000000;

    bool is_ative = true;
    while (is_ative)
    {
        switch (aroma_idle(&Player))
        {
            case -3:
                is_ative = false;
                break;
        }

    }

    aroma_exit(&Player);
}
