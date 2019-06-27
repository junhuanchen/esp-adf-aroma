
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void task_ledc_fade(void *arg);

void set_ledc(int enable, int pause, bool mode);

void unit_test_ledc_fade();

void all_ledc_on();

void all_ledc_off();

void start_ledc_fade();

void close_ledc_fade();

void open_during_led();

void stop_during_led();

void open_nightly_led();

void stop_nightly_led();
