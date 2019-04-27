
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void task_ledc_fade(void *arg);

void set_ledc(int enable, int pause, bool mode);

void unit_test_ledc_fade();

void all_ledc_on();

void all_ledc_off();
