
#include "aroma_ledc.h"
#include "driver/ledc.h"
#include "time.h"

#define LEDC_TIMER             LEDC_TIMER_0
#define LEDC_MODE              LEDC_HIGH_SPEED_MODE
#define LEDC_FADE_GPIO1        (23)   //Set GPIO 22 as LEDC
#define LEDC_FADE_GPIO2        (18)   //Set GPIO 22 as LEDC
#define LEDC_FADE_CHANNEL1      LEDC_CHANNEL_0
#define LEDC_FADE_CHANNEL2      LEDC_CHANNEL_1

#define LEDC_TEST_DUTY_MAX     (8000)//max to 2 ** duty_resolution - 1 = 2 ** 13 -1 = 8191
#define LEDC_TEST_DUTY_MIN     (200)
#define LEDC_TEST_FADE_TIME    (4000) //T = 8s

bool ledc_fade_enable = false;
bool ledc_fade_pause = false;

int nightly_mode = true; // 红灯

void set_ledc(int enable, int pause, bool mode)
{
    ledc_fade_enable = enable;
    ledc_fade_pause = pause;
    nightly_mode = mode;
}

void all_ledc_on()
{
    //ledc_set_fade_time_and_start(LEDC_MODE, LEDC_FADE_CHANNEL, LEDC_TEST_DUTY_MAX, LEDC_TEST_FADE_TIME, LEDC_FADE_NO_WAIT);
    ledc_set_fade_with_time(LEDC_MODE,LEDC_FADE_CHANNEL1, LEDC_TEST_DUTY_MAX, 1000);
    ledc_fade_start(LEDC_MODE,LEDC_FADE_CHANNEL1, LEDC_FADE_NO_WAIT);

    ledc_set_fade_with_time(LEDC_MODE,LEDC_FADE_CHANNEL2, LEDC_TEST_DUTY_MAX, 1000);
    ledc_fade_start(LEDC_MODE,LEDC_FADE_CHANNEL2, LEDC_FADE_NO_WAIT);
}

void all_ledc_off()
{
    //ledc_set_fade_time_and_start(LEDC_MODE, LEDC_FADE_CHANNEL, LEDC_TEST_DUTY_MIN, LEDC_TEST_FADE_TIME, LEDC_FADE_NO_WAIT);
    ledc_set_fade_with_time(LEDC_MODE,LEDC_FADE_CHANNEL1, 0, 1000);
    ledc_fade_start(LEDC_MODE,LEDC_FADE_CHANNEL1, LEDC_FADE_NO_WAIT);
    
    ledc_set_fade_with_time(LEDC_MODE,LEDC_FADE_CHANNEL2, 0, 1000);
    ledc_fade_start(LEDC_MODE,LEDC_FADE_CHANNEL2, LEDC_FADE_NO_WAIT);
}

void start_ledc_fade()
{
    puts("start_ledc_fade");
    if(nightly_mode)
    {
        //ledc_set_fade_time_and_start(LEDC_MODE, LEDC_FADE_CHANNEL, LEDC_TEST_DUTY_MAX, LEDC_TEST_FADE_TIME, LEDC_FADE_NO_WAIT);
        ledc_set_fade_with_time(LEDC_MODE,LEDC_FADE_CHANNEL1, LEDC_TEST_DUTY_MAX, LEDC_TEST_FADE_TIME);
        ledc_fade_start(LEDC_MODE,LEDC_FADE_CHANNEL1, LEDC_FADE_NO_WAIT);
    }
    else
    {
        ledc_set_fade_with_time(LEDC_MODE,LEDC_FADE_CHANNEL2, LEDC_TEST_DUTY_MAX, LEDC_TEST_FADE_TIME);
        ledc_fade_start(LEDC_MODE,LEDC_FADE_CHANNEL2, LEDC_FADE_NO_WAIT);
    }
    vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_RATE_MS);
}

void close_ledc_fade()
{
    puts("close_ledc_fade");
    if(nightly_mode)
    {
        //ledc_set_fade_time_and_start(LEDC_MODE, LEDC_FADE_CHANNEL, LEDC_TEST_DUTY_MIN, LEDC_TEST_FADE_TIME, LEDC_FADE_NO_WAIT);
        ledc_set_fade_with_time(LEDC_MODE,LEDC_FADE_CHANNEL1, 0, LEDC_TEST_FADE_TIME);
        ledc_fade_start(LEDC_MODE,LEDC_FADE_CHANNEL1, LEDC_FADE_NO_WAIT);
    }
    else
    {
        ledc_set_fade_with_time(LEDC_MODE,LEDC_FADE_CHANNEL2, 0, LEDC_TEST_FADE_TIME);
        ledc_fade_start(LEDC_MODE,LEDC_FADE_CHANNEL2, LEDC_FADE_NO_WAIT);
    }
    vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_RATE_MS);
}

void task_ledc_fade(void *arg)
{
    // Aroma * aroma = (Aroma *)arg;

    // vTaskDelay(5000 / portTICK_RATE_MS);

    // aroma_stop(aroma);
    // aroma_volume(aroma, 100);
    // aroma_play(aroma);

    // while(true) vTaskDelay(5000 / portTICK_RATE_MS);

    uint64_t time_start = esp_timer_get_time();
    ledc_timer_config_t ledc_timer;
    ledc_timer.duty_resolution = LEDC_TIMER_13_BIT; // resolution of PWM duty
    ledc_timer.freq_hz = 5000;                      // frequency of PWM signal
    ledc_timer.speed_mode = LEDC_MODE;           // timer mode
    ledc_timer.timer_num = LEDC_TIMER;           // timer index
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel1;
    ledc_channel1.channel    = LEDC_FADE_CHANNEL1;
    ledc_channel1.duty       = 0;
    ledc_channel1.gpio_num   = LEDC_FADE_GPIO1;
    ledc_channel1.speed_mode = LEDC_MODE;
        //.hpoint     = 0,
    ledc_channel1.timer_sel  = LEDC_TIMER;
    ledc_channel_config(&ledc_channel1);

    ledc_channel_config_t ledc_channel2;
    ledc_channel2.channel    = LEDC_FADE_CHANNEL2;
    ledc_channel2.duty       = 0;
    ledc_channel2.gpio_num   = LEDC_FADE_GPIO2;
    ledc_channel2.speed_mode = LEDC_MODE;
        //.hpoint     = 0,
    ledc_channel2.timer_sel  = LEDC_TIMER;
    ledc_channel_config(&ledc_channel2);

    ledc_fade_func_install(0);

    while(true)
    {
        if(ledc_fade_enable)
        {
            start_ledc_fade();
            // music value += 1
            
            if (ledc_fade_pause)
            {
                ledc_fade_enable = false;
            }
            else
            {
                close_ledc_fade();
            }
        }
        else
        {
            if (ledc_fade_pause == false)
            {
                close_ledc_fade();
            }
        }
        
        vTaskDelay(2500 / portTICK_RATE_MS);
    }
    
    // mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
    // ledc_fade_func_uninstall();
}

void unit_test_ledc_fade()
{
    xTaskCreate(task_ledc_fade, "task_ledc_fade", 8192, NULL, 2, NULL);
}
