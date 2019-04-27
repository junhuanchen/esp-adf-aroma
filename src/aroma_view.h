
#include "tm1620.h"

void task_view(void *arg);

void unit_test_view();

enum view_state
{
    view_idle,
    view_config,
    view_local_time,
    view_getup_time,
    view_sleep_time,
    view_debug,
};

void button_13_PRESSED();
void button_13_LONG_PRESSED();
void button_13_LONG_RELEASE();

void button_32_PRESSED();
void button_32_LONG_PRESSED();
void button_32_LONG_RELEASE();

void button_33_RELEASE();
void button_33_PRESSED();
void button_33_LONG_PRESSED();
void button_33_LONG_RELEASE();

void button_27_PRESSED();
void button_27_LONG_PRESSED();
void button_27_LONG_RELEASE();
