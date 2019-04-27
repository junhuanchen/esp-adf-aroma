
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_mem.h"
#include "audio_common.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "esp_peripherals.h"
#include "periph_touch.h"
#include "periph_button.h"
#include "audio_hal.h"
#include "board.h"

typedef struct aroma
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t i2s_stream_writer, mp3_decoder;
    audio_hal_handle_t hal;
    esp_periph_handle_t touch_periph;
    esp_periph_handle_t button_periph;
    audio_event_iface_handle_t evt;
    int player_volume;
} Aroma;

void aroma_play(Aroma *Self);

void aroma_pause(Aroma *Self);

void aroma_resume(Aroma *Self);

void aroma_stop(Aroma *Self);

int aroma_music(Aroma *Self, const uint8_t *start, const uint8_t *end);

int aroma_volume(Aroma *Self, int Value);

void aroma_init(Aroma *Self);

int aroma_idle(Aroma *Self);

void aroma_exit(Aroma *Self);
