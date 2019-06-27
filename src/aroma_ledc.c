
#include "aroma_ledc.h"
#include "driver/ledc.h"
#include "time.h"

#define LEDC_TIMER             LEDC_TIMER_0
#define LEDC_MODE              LEDC_HIGH_SPEED_MODE
#define LEDC_FADE_GPIO1        (23)
#define LEDC_FADE_GPIO2        (18)
#define LEDC_FADE_CHANNEL1      LEDC_CHANNEL_0
#define LEDC_FADE_CHANNEL2      LEDC_CHANNEL_1

#define LEDC_TEST_FADE_TIME 2000

int DuringDutyMax = 4000;
// int DuringDutyMin = 4000;
int DuringDutyTime = 1000;

int NightlyDutyMax = 1000;
// int NightlyDutyMin = 4000;
int NightlyDutyTime = 1000;

bool ledc_fade_enable = false;
bool ledc_fade_pause = false;

int nightly_mode = false; //  nightly 红灯

void open_during_led()
{
    ledc_set_fade_with_time(LEDC_MODE,LEDC_FADE_CHANNEL1, DuringDutyMax, DuringDutyTime);
    ledc_fade_start(LEDC_MODE,LEDC_FADE_CHANNEL1, LEDC_FADE_NO_WAIT);
}

void stop_during_led()
{
    ledc_set_fade_with_time(LEDC_MODE,LEDC_FADE_CHANNEL1, 0, DuringDutyTime);
    ledc_fade_start(LEDC_MODE,LEDC_FADE_CHANNEL1, LEDC_FADE_NO_WAIT);
    vTaskDelay(DuringDutyTime / portTICK_RATE_MS);
    ledc_stop(LEDC_MODE, LEDC_FADE_CHANNEL1, 0);

    // vTaskDelay(DuringDutyTime / portTICK_RATE_MS);
    // gpio_set_level(GPIO_NUM_23, 1);
}

void open_nightly_led()
{
    ledc_set_fade_with_time(LEDC_MODE,LEDC_FADE_CHANNEL2, NightlyDutyMax, NightlyDutyTime);
    ledc_fade_start(LEDC_MODE,LEDC_FADE_CHANNEL2, LEDC_FADE_NO_WAIT);
}

void stop_nightly_led()
{
    ledc_set_fade_with_time(LEDC_MODE,LEDC_FADE_CHANNEL2, 0, NightlyDutyTime);
    ledc_fade_start(LEDC_MODE,LEDC_FADE_CHANNEL2, LEDC_FADE_NO_WAIT);
    vTaskDelay(NightlyDutyTime / portTICK_RATE_MS);
    ledc_stop(LEDC_MODE, LEDC_FADE_CHANNEL2, 0);

    // vTaskDelay(NightlyDutyTime / portTICK_RATE_MS);
    // gpio_set_level(GPIO_NUM_18, 1);
}

void set_ledc(int enable, int pause, bool mode)
{
    ledc_fade_enable = enable;
    ledc_fade_pause = pause;
    nightly_mode = mode;
}

void all_ledc_on()
{
    //ledc_set_fade_time_and_start(LEDC_MODE, LEDC_FADE_CHANNEL, LEDC_TEST_DUTY_MAX, LEDC_TEST_FADE_TIME, LEDC_FADE_NO_WAIT);
    open_during_led();
    open_nightly_led();
    
}

void all_ledc_off()
{    
    ledc_fade_enable = false;
    stop_during_led();
    stop_nightly_led();

    // ledc_timer_resume(LEDC_MODE, LEDC_TIMER);
    // vTaskDelay(10 / portTICK_RATE_MS);
    
}

void start_ledc_fade()
{
    puts("start_ledc_fade");
    if(nightly_mode)
    {
        //ledc_set_fade_time_and_start(LEDC_MODE, LEDC_FADE_CHANNEL, LEDC_TEST_DUTY_MAX, LEDC_TEST_FADE_TIME, LEDC_FADE_NO_WAIT);
        open_during_led();
        stop_nightly_led();
    }
    else
    {
        open_nightly_led();
        stop_during_led();
    }
}

void close_ledc_fade()
{
    puts("close_ledc_fade");
    if(nightly_mode)
    {
        stop_nightly_led();
    }
    else
    {
        stop_during_led();
    }
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
            vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_RATE_MS);
            // music value += 1
            
            if (ledc_fade_pause)
            {
                ledc_fade_enable = false;
            }
            else
            {
                close_ledc_fade();
                vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_RATE_MS);
            }
        }
        else
        {
            if (ledc_fade_pause == false)
            {
                close_ledc_fade();
                vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_RATE_MS);
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
