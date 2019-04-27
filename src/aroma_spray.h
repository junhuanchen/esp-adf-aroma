
#include <stdbool.h>

void start_spray();

void close_spray();

void set_spray(int time, int duty, int state, int mode);

void task_spray(void *arg);

void unit_test_spray();

void all_spray_on();

void all_spray_off();
