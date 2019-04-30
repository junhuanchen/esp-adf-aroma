#include "aroma_spray.h"

#include "soc/rtc.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define GPIO_PWM0A_OUT         (14)   //Set GPIO 05 as PWM0A
#define GPIO_PWM0B_OUT         (12)   //Set GPIO 23 as PWM0B

int spray_start = 5000 / portTICK_RATE_MS, spray_stop = 5000 / portTICK_RATE_MS;
int spray_time = 0, spray_duty = 50, spray_state = 0, spray_mode = 0;

void set_spray(int time, int duty, int state, int mode)
{
    spray_time = time;
    spray_duty = duty;
    spray_state = state;
    spray_mode= mode;
}

void all_spray_on()
{
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A,  spray_duty); // pwm_duty
    // mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
    // mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);

    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B,  spray_duty); // pwm_duty
    // mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
    // mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
}

void all_spray_off()
{
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A,  0); // pwm_duty
    // mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
    // mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
    
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B,  0); // pwm_duty
    // mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
    // mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
}

void start_spray()
{
    if (spray_mode)
    {
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A,  spray_duty); // pwm_duty
        // mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
        // mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
    }
    else
    {
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B,  spray_duty); // pwm_duty
        // mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
        // mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
    }
}

void close_spray()
{
    if (spray_mode)
    {
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A,  0); // pwm_duty
        // mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
        // mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
    }
    else
    {
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B,  0); // pwm_duty
        // mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
        // mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
    }
}

int get_second()
{
    return esp_timer_get_time() / 1000000;
}

void task_spray(void *arg)
{
    // Aroma * aroma = (Aroma *)arg;
    
    //1. mcpwm gpio initialization
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_PWM0A_OUT);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, GPIO_PWM0B_OUT);

    //2. initial mcpwm configuration
    printf("Configuring Initial Parameters of mcpwm...\n");
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 300;    //frequency = 300Hz,
    pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

    close_spray();

    int64_t last_time = get_second();
    int interval = 0;

    while(true)
    {
        interval = (get_second() - last_time);
        printf("spray_state %d spray_time %d interval %d\n", spray_state, spray_time, interval);

        // if (spray_pause == false)
        {
            if(spray_state == 1 && spray_time != 0)
            {
                spray_state = 2;
                last_time = get_second();
                start_spray();
            }
        
            if (spray_state == 2 && interval >= spray_time)
            {
                spray_state = 0;
            }
            
            if (spray_state == 0 && spray_time != 0)
            {
                spray_time = 0;
                close_spray();
            }
            
        }

        vTaskDelay(5000 / portTICK_RATE_MS);
    }
}

void unit_test_spray()
{
    xTaskCreate(task_spray, "task_spray", 8192, NULL, 1, NULL);
}
