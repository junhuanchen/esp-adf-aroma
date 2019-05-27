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

void app_main(void)
{
    aroma_init(&Player);
    aroma_music(&Player, getup_mp3_start, getup_mp3_end);
    // aroma_music(&Player, sleep_mp3_start, sleep_mp3_end);
    
    // aroma_play(&Player);
    
    // return;
    
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
