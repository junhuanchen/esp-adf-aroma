
#include "aroma_ledc.h"
#include "driver/ledc.h"
#include "time.h"

#define LEDC_TEST_FADE_TIME 2000
#define GPIO_18 18
#define GPIO_23 23

int DuringDutyMax = 4000;
// int DuringDutyMin = 4000;
int DuringDutyTime = 1000;

int NightlyDutyMax = 1000;
// int NightlyDutyMin = 4000;
int NightlyDutyTime = 1000;

bool ledc_fade_enable = false;
bool ledc_fade_pause = false;

int nightly_mode = false; //  nightly 红灯

bool During = false, Nightly = false;

void open_during_led()
{
    During = true;

    const int value = DuringDutyMax / 400;
    
    int i = 0;
    for (; i < NightlyDutyMax; i += value)
    {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, i);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        vTaskDelay(value / portTICK_PERIOD_MS);
    }
    
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    i -= value;
    for (; i > 0; i -= value)
    {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, i);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        vTaskDelay(value / portTICK_PERIOD_MS);
    }
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    // ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, DuringDutyMax);
    // ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    // ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0, DuringDutyMax, DuringDutyTime);
    // ledc_fade_start(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);
}

void stop_during_led()
{
    During = false;

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0, 0, DuringDutyTime);
    // ledc_fade_start(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);
    // vTaskDelay(DuringDutyTime / portTICK_RATE_MS);
    // ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);

    // vTaskDelay(500 / portTICK_RATE_MS);
    // gpio_set_direction(GPIO_NUM_23, GPIO_MODE_OUTPUT);
    // gpio_set_level(GPIO_NUM_23, 0);
}

void open_nightly_led()
{
    Nightly = true;

    const int value = NightlyDutyMax / 100;
    int i = 0;
    for (; i < NightlyDutyMax; i += value)
    {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, i);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
        vTaskDelay(value / portTICK_PERIOD_MS);
    }
    
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    i -= value;
    for (; i > 0; i -= value)
    {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, i);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
        vTaskDelay(value / portTICK_PERIOD_MS);
    }
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);

    // ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, NightlyDutyMax);
    // ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);

    // ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_1, NightlyDutyMax, NightlyDutyTime);
    // ledc_fade_start(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_1, LEDC_FADE_NO_WAIT);
}

void stop_nightly_led()
{
    Nightly = false;

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void set_ledc(int enable, int pause, bool mode)
{
    ledc_fade_enable = enable;
    ledc_fade_pause = pause;
    nightly_mode = mode;
}

void all_ledc_on()
{
    //ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_FADE_CHANNEL, LEDC_TEST_DUTY_MAX, LEDC_TEST_FADE_TIME, LEDC_FADE_NO_WAIT);

    // open_during_led();
    // open_nightly_led();
    
    // ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    // ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    // // vTaskDelay(DuringDutyTime / portTICK_PERIOD_MS);

    // ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
    // ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    // // vTaskDelay(NightlyDutyTime / portTICK_PERIOD_MS);
    
    puts("all_ledc_on");
    if(nightly_mode)
    {
        //ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_FADE_CHANNEL, LEDC_TEST_DUTY_MAX, LEDC_TEST_FADE_TIME, LEDC_FADE_NO_WAIT);
        
        if (Nightly)
        {
            stop_nightly_led();
        }

        const int value = DuringDutyMax / 400;
        
        int i = 0;
        for (; i < DuringDutyMax; i += value)
        {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, i);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            vTaskDelay(value / portTICK_PERIOD_MS);
        }
        During = true;
        
        // ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, DuringDutyMax);
        // ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        // vTaskDelay(1000 / portTICK_PERIOD_MS);

        // open_during_led();
    }
    else
    {
        if (During)
        {
            stop_during_led();
        }

        const int value = NightlyDutyMax / 100;
        
        int i = 0;
        for (; i < NightlyDutyMax; i += value)
        {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, i);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
            vTaskDelay(value / portTICK_PERIOD_MS);
        }
        Nightly = true;

        // ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, NightlyDutyMax);
        // ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
        // vTaskDelay(1000 / portTICK_PERIOD_MS);

        // open_nightly_led();
    }
}

void all_ledc_off()
{    
    ledc_fade_enable = false;
    
    // stop_during_led();
    // stop_nightly_led();

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    // vTaskDelay(DuringDutyTime / portTICK_PERIOD_MS);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    // vTaskDelay(NightlyDutyTime / portTICK_PERIOD_MS);
}

void start_ledc_fade()
{
    puts("start_ledc_fade");
    if(nightly_mode)
    {
        //ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_FADE_CHANNEL, LEDC_TEST_DUTY_MAX, LEDC_TEST_FADE_TIME, LEDC_FADE_NO_WAIT);
        
        if (Nightly)
        {
            stop_nightly_led();
        }

        open_during_led();
    }
    else
    {
        if (During)
        {
            stop_during_led();
        }
        
        open_nightly_led();
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

    ledc_timer_config_t ledc_time_0;
    ledc_time_0.duty_resolution = LEDC_TIMER_13_BIT; // resolution of PWM duty
    ledc_time_0.freq_hz = 5000;                      // frequency of PWM signal
    ledc_time_0.speed_mode = LEDC_LOW_SPEED_MODE;           // timer mode
    ledc_time_0.timer_num = LEDC_TIMER_0;           // timer index
    ledc_timer_config(&ledc_time_0);

    ledc_timer_config_t ledc_time_1;
    ledc_time_1.duty_resolution = LEDC_TIMER_13_BIT; // resolution of PWM duty
    ledc_time_1.freq_hz = 5000;                      // frequency of PWM signal
    ledc_time_1.speed_mode = LEDC_LOW_SPEED_MODE;           // timer mode
    ledc_time_1.timer_num = LEDC_TIMER_1;           // timer index
    ledc_timer_config(&ledc_time_1);

    ledc_channel_config_t ledc_channel1;
    ledc_channel1.channel    = LEDC_CHANNEL_0;
    ledc_channel1.duty       = 0;
    ledc_channel1.gpio_num   = GPIO_23;
    ledc_channel1.speed_mode = LEDC_LOW_SPEED_MODE;
        //.hpoint     = 0,
    ledc_channel1.timer_sel  = LEDC_TIMER_0;
    ledc_channel_config(&ledc_channel1);

    ledc_channel_config_t ledc_channel2;
    ledc_channel2.channel    = LEDC_CHANNEL_1;
    ledc_channel2.duty       = 0;
    ledc_channel2.gpio_num   = GPIO_18;
    ledc_channel2.speed_mode = LEDC_LOW_SPEED_MODE;
        //.hpoint     = 0,
    ledc_channel2.timer_sel  = LEDC_TIMER_1;
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
        // else if (ledc_fade_pause == false)
        // {
        //     close_ledc_fade();
        //     vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_RATE_MS);
        // }
        
        vTaskDelay(2500 / portTICK_RATE_MS);
        puts("task_ledc_fade");
    }
    
    // mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
    // ledc_fade_func_uninstall();
}

void unit_test_ledc_fade()
{
    xTaskCreate(task_ledc_fade, "task_ledc_fade", 8192, NULL, 2, NULL);
}
