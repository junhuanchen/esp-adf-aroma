#include "aroma.h"

static const char *TAG_Aroma = "Aroma";

static uint8_t *music_head = NULL, *music_tail = NULL;
static int music_pos;

bool music_is_play = false, music_is_pause = false;

int music_read_cb(audio_element_handle_t el, char *buf, int len, TickType_t wait_time, void *ctx)
{
    int read_size = music_tail - music_head - music_pos;
    if (read_size == 0)
    {
        return AEL_IO_DONE;
    }
    else if (len < read_size)
    {
        read_size = len;
    }
    memcpy(buf, music_head + music_pos, read_size);
    music_pos += read_size;
    return read_size;
}

void aroma_play(Aroma *Self)
{
    aroma_resume(Self); // 注意 注释
    // music_is_play = true;
    // music_is_pause = false;
    audio_pipeline_run(Self->pipeline);

}

void aroma_pause(Aroma *Self)
{
    audio_pipeline_pause(Self->pipeline);
    music_is_pause = true;
}

void aroma_resume(Aroma *Self)
{
    audio_pipeline_resume(Self->pipeline);
    music_is_play = true;
    music_is_pause = false;
}

void aroma_stop(Aroma *Self)
{
    audio_pipeline_stop(Self->pipeline);
    music_pos = 0;
    music_is_pause = music_is_play = false;
    // aroma_resume(Self);
}

int aroma_music(Aroma *Self, const uint8_t *start, const uint8_t *end)
{
    aroma_stop(Self);
    music_head = start, music_tail = end;
    return music_tail - music_head;
}

int aroma_volume(Aroma *Self, int Value)
{
    printf("aroma_volume Value %d\n", Value);
    Self->player_volume += Value;
    if (Self->player_volume > 100)
        Self->player_volume = 100;
    if (Self->player_volume < 0)
        Self->player_volume = 0;
    audio_hal_set_volume(Self->hal, Self->player_volume);
    return Self->player_volume;
}

void aroma_init(Aroma *Self)
{
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG_Aroma, ESP_LOG_INFO);

    ESP_LOGI(TAG_Aroma, "[ 1 ] Start audio codec chip");
    audio_hal_codec_config_t audio_hal_codec_cfg = AUDIO_HAL_ES8388_DEFAULT();
    Self->hal = audio_hal_init(&audio_hal_codec_cfg, 0);
    audio_hal_ctrl_codec(Self->hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    audio_hal_get_volume(Self->hal, &Self->player_volume);

    ESP_LOGI(TAG_Aroma, "[ 2 ] Create audio pipeline, add all elements to pipeline, and subscribe pipeline event");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    Self->pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(Self->pipeline);

    ESP_LOGI(TAG_Aroma, "[2.1] Create mp3 decoder to decode mp3 file and set custom read callback");
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    Self->mp3_decoder = mp3_decoder_init(&mp3_cfg);
    audio_element_set_read_cb(Self->mp3_decoder, music_read_cb, NULL);

    ESP_LOGI(TAG_Aroma, "[2.2] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    Self->i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(TAG_Aroma, "[2.3] Register all elements to audio pipeline");
    audio_pipeline_register(Self->pipeline, Self->mp3_decoder, "mp3");
    audio_pipeline_register(Self->pipeline, Self->i2s_stream_writer, "i2s");

    ESP_LOGI(TAG_Aroma, "[2.4] Link it together [music_read_cb]-->mp3_decoder-->i2s_stream-->[codec_chip]");
    audio_pipeline_link(Self->pipeline, (const char *[]){"mp3", "i2s"}, 2);

    ESP_LOGI(TAG_Aroma, "[ 3 ] Initialize peripherals");
    esp_periph_config_t periph_cfg = {0};
    esp_periph_init(&periph_cfg);

    // ESP_LOGI(TAG_Aroma, "[3.1] Initialize Touch And Button peripheral");
    // periph_touch_cfg_t touch_cfg = {
    //     .touch_mask = TOUCH_SEL_SET | TOUCH_SEL_PLAY | TOUCH_SEL_VOLUP | TOUCH_SEL_VOLDWN,
    //     .tap_threshold_percent = 70,
    // };
    // Self->touch_periph = periph_touch_init(&touch_cfg);

    // Initialize button peripheral
    periph_button_cfg_t btn_cfg = {
        .gpio_mask = GPIO_SEL_32 | GPIO_SEL_13 | GPIO_SEL_27 | GPIO_SEL_33,
        .long_press_time_ms = 1500
    };
    Self->button_periph = periph_button_init(&btn_cfg);
    
    ESP_LOGI(TAG_Aroma, "[3.2] Start all peripherals");
    esp_periph_start(Self->button_periph);
    // esp_periph_start(Self->touch_periph);

    ESP_LOGI(TAG_Aroma, "[ 4 ] Setup event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    Self->evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG_Aroma, "[4.1] Listening event from all elements of pipeline");
    audio_pipeline_set_listener(Self->pipeline, Self->evt);

    ESP_LOGI(TAG_Aroma, "[4.2] Listening event from peripherals");
    audio_event_iface_set_listener(esp_periph_get_event_iface(), Self->evt);

    ESP_LOGW(TAG_Aroma, "[ 5 ] Tap touch buttons to control music player:");
    ESP_LOGW(TAG_Aroma, "      [Play] to start, pause and resume, [Set] to stop.");
    ESP_LOGW(TAG_Aroma, "      [Vol-] or [Vol+] to adjust volume.");
}

void aroma_exit(Aroma *Self)
{
    ESP_LOGI(TAG_Aroma, "[ 6 ] Stop audio_pipeline");
    audio_pipeline_terminate(Self->pipeline);

    /* Terminate the pipeline before removing the listener */
    audio_pipeline_remove_listener(Self->pipeline);

    /* Make sure audio_pipeline_remove_listener is called before destroying event_iface */
    audio_event_iface_destroy(Self->evt);

    /* Release all resources */
    audio_pipeline_deinit(Self->pipeline);
    audio_element_deinit(Self->i2s_stream_writer);
    audio_element_deinit(Self->mp3_decoder);
}

extern bool ledc_fade_enable, ledc_fade_pause;

extern int spray_state, spray_time, spray_duty;

#include "aroma_view.h"

int aroma_idle(Aroma *Self)
{
    audio_event_iface_msg_t msg;
    esp_err_t ret = audio_event_iface_listen(Self->evt, &msg, portMAX_DELAY);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_Aroma, "[ * ] Event interface error : %d", ret);
        return -1;
    }

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *)Self->mp3_decoder && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
    {
        audio_element_info_t music_info = {0};
        audio_element_getinfo(Self->mp3_decoder, &music_info);

        ESP_LOGI(TAG_Aroma, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                 music_info.sample_rates, music_info.bits, music_info.channels);

        audio_element_setinfo(Self->i2s_stream_writer, &music_info);
        i2s_stream_set_clk(Self->i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
        return -2;
    }

    if (msg.source_type == PERIPH_ID_BUTTON && msg.source == (void *)Self->button_periph)
    {
        // msg.cmd == PERIPH_BUTTON_PRESSED && 
        // printf("msg data %d cmd %d \n", (int)msg.data, msg.cmd);
        if((int)msg.data == 13 && msg.cmd == PERIPH_BUTTON_LONG_PRESSED) // 停止
        {
            puts("button_13_LONG_PRESSED");
            button_13_LONG_PRESSED();
        }

        if((int)msg.data == 13 && msg.cmd == PERIPH_BUTTON_LONG_RELEASE) // 停止
        {
            puts("button_13_LONG_RELEASE");
            button_13_LONG_RELEASE();
        }

        if((int)msg.data == 13 && msg.cmd == PERIPH_BUTTON_PRESSED) // 停止
        {
            puts("button_13_PRESSED");
            button_13_PRESSED();
        }

        if((int)msg.data == 32 && msg.cmd == PERIPH_BUTTON_LONG_PRESSED) // 停止
        {
            puts("button_32_LONG_PRESSED");
            button_32_LONG_PRESSED();
        }

        if((int)msg.data == 32 && msg.cmd == PERIPH_BUTTON_LONG_RELEASE) // 停止
        {
            puts("button_32_LONG_RELEASE");
            button_32_LONG_RELEASE();
        }

        if((int)msg.data == 32 && msg.cmd == PERIPH_BUTTON_PRESSED) // 停止
        {
            puts("button_32_PRESSED");
            button_32_PRESSED();
        }
        
        if((int)msg.data == 33 && msg.cmd == PERIPH_BUTTON_RELEASE) // 停止
        {
            puts("button_33_RELEASE");
            button_33_RELEASE();
        }
        
        if((int)msg.data == 33 && msg.cmd == PERIPH_BUTTON_LONG_PRESSED) // 停止
        {
            puts("button_33_LONG_PRESSED");
            button_33_LONG_PRESSED();
        }

        if((int)msg.data == 33 && msg.cmd == PERIPH_BUTTON_LONG_RELEASE) // 停止
        {
            puts("button_33_LONG_RELEASE");
            button_33_LONG_RELEASE();
        }

        if((int)msg.data == 33 && msg.cmd == PERIPH_BUTTON_PRESSED) // 停止
        {
            puts("button_33_PRESSED");
            button_33_PRESSED();
        }
        
        if((int)msg.data == 27 && msg.cmd == PERIPH_BUTTON_LONG_PRESSED) // 停止
        {
            puts("button_27_LONG_PRESSED");
            button_27_LONG_PRESSED();
        }

        if((int)msg.data == 27 && msg.cmd == PERIPH_BUTTON_LONG_RELEASE) // 停止
        {
            puts("button_27_LONG_RELEASE");
            button_27_LONG_RELEASE();
        }

        if((int)msg.data == 27 && msg.cmd == PERIPH_BUTTON_PRESSED) // 停止
        {
            puts("button_27_PRESSED");
            button_27_PRESSED();
        }
        
        // if((int)msg.data == 32 && msg.cmd == PERIPH_BUTTON_PRESSED) // 太阳 
        // {
        //     ESP_LOGI(TAG_Aroma, "[ * ] [Play] button 32 event");
        //     // spray_time = 10;
        //     // spray_duty = 50;
        //     // spray_state = true;
        //     TM1620_Print(" L  ");
        //     ledc_fade_enable = true;
        //     view_mode = view_idle;
        // }

        // if((int)msg.data == 33 && msg.cmd == PERIPH_BUTTON_PRESSED) // 圆型
        // {
        //     ESP_LOGI(TAG_Aroma, "[ * ] [Play] button 33 event");
        //     ledc_fade_enable = false;
        //     spray_time = 30;
        //     spray_duty = 100;
        //     spray_state = true;
        //     TM1620_Print(" H  ");
        //     view_mode = view_config;
        // }

        // if((int)msg.data == 27 && msg.cmd == PERIPH_BUTTON_PRESSED) // 月亮
        // {
        //     ESP_LOGI(TAG_Aroma, "[ * ] [Play] button 27 event");
        //     spray_state = false;
        //     TM1620_Print(" U  ");
        //     view_mode = view_debug;
        //     aroma_play(Self);
        // }

        // if((int)msg.data == 13 && msg.cmd == PERIPH_BUTTON_PRESSED) // 停止
        // {
        //     ESP_LOGI(TAG_Aroma, "[ * ] [Play] button 13 event");
        //     ESP_LOGI(TAG_Aroma, "aroma_stop(Self);");
 
        //     aroma_stop(Self);
        // }
    }

    // if (msg.source_type == PERIPH_ID_TOUCH && msg.cmd == PERIPH_TOUCH_TAP && msg.source == (void *)Self->touch_periph)
    // {
        
    //     if ((int)msg.data == TOUCH_PLAY)
    //     {
    //         ESP_LOGI(TAG_Aroma, "[ * ] [Play] touch tap event");
    //         audio_element_state_t el_state = audio_element_get_state(Self->i2s_stream_writer);
    //         switch (el_state)
    //         {
    //             case AEL_STATE_INIT:
    //                 ESP_LOGI(TAG_Aroma, "[ * ] Starting audio pipeline");

    //                 aroma_play(Self);
                    
    //                 break;
    //             case AEL_STATE_RUNNING:
    //                 ESP_LOGI(TAG_Aroma, "[ * ] Pausing audio pipeline");

    //                 aroma_pause(Self);

    //                 break;
    //             case AEL_STATE_PAUSED:
    //                 ESP_LOGI(TAG_Aroma, "[ * ] Resuming audio pipeline");

    //                 aroma_resume(Self);

    //                 break;
    //             case AEL_STATE_FINISHED:
    //                 ESP_LOGI(TAG_Aroma, "[ * ] Rewinding audio pipeline");

    //                 aroma_stop(Self);

    //                 break;
    //             default:
    //                 ESP_LOGI(TAG_Aroma, "[ * ] Not supported state %d", el_state);
    //         }
    //     }
    //     else if ((int)msg.data == TOUCH_SET)
    //     {
    //         ESP_LOGI(TAG_Aroma, "[ * ] [Set] touch tap event");
    //         ESP_LOGI(TAG_Aroma, "[ * ] Stopping audio pipeline");
    //         return -3;
    //     }
    //     else if ((int)msg.data == TOUCH_VOLUP)
    //     {
    //         ESP_LOGI(TAG_Aroma, "[ * ] [Vol+] touch tap event");
            
    //         aroma_volume(Self, +10);

    //         ESP_LOGI(TAG_Aroma, "[ * ] Volume set to %d %%", Self->player_volume);
    //     }
    //     else if ((int)msg.data == TOUCH_VOLDWN)
    //     {
    //         ESP_LOGI(TAG_Aroma, "[ * ] [Vol-] touch tap event");

    //         aroma_volume(Self, -10);

    //         ESP_LOGI(TAG_Aroma, "[ * ] Volume set to %d %%", Self->player_volume);
    //     }
        
    // }

    return 0;
}
