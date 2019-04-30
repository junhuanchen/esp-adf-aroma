#include <stdio.h>
#include <string.h>
#include <time.h>

#include "aroma_config.h"

#include "esp_log.h"
#include "aroma_view.h"
#include "aroma_spray.h"
#include "aroma_ledc.h"

#include "board.h"
#include "PCF8563.h"

#include "esp_system.h"
#include "driver/i2c.h"

#include "aroma.h"

extern const uint8_t sleep_mp3_start[] asm("_binary_sleep_mp3_start");
extern const uint8_t sleep_mp3_end[] asm("_binary_sleep_mp3_end");

extern const uint8_t getup_mp3_start[] asm("_binary_getup_mp3_start");
extern const uint8_t getup_mp3_end[] asm("_binary_getup_mp3_end");

const char * TAG_VIEW = "aroma_view";

extern Aroma Player;
extern bool music_is_play, music_is_pause;


time_t getup_time, sleep_time, local_time, time_change, *time_data = &local_time;

bool is_view = true;

void iic_init()
{
    i2c_config_t conf = {0};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = 21;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = 22;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 100000;
    esp_err_t ret = i2c_param_config(I2C_NUM_0, &conf);
    assert(ret == ESP_OK);
    ret = i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
    assert(ret == ESP_OK);
}

void sync_time(void) 
{
    int ret = PCF_hctosys();
    if (ret != 0) {
        PCF_systohc();
        ESP_LOGE(TAG_VIEW, "Error reading hardware clock: %d", ret);
        esp_restart();
    }

    printf("ret %d time %ld\n", ret, time(NULL));

    TM1620_Init();
}

void set_local_time(time_t now)
{
	int ret;
	PCF_DateTime date = {0};
	struct tm tm = {0};

	ret = PCF_Init(0);
	if (ret != 0) {
		goto fail;
	}

	gmtime_r(&now, &tm);
	date.second = tm.tm_sec;
	date.minute = tm.tm_min;
	date.hour = tm.tm_hour;
	date.day = tm.tm_mday;
	date.month = tm.tm_mon + 1;
	date.year = tm.tm_year + 1900;
	date.weekday = tm.tm_wday;

	ret = PCF_SetDateTime(&date);

fail:

	return ret;
}

int view_mode = view_idle, config_mode = view_idle;

int idle, config, debug;

int time_now = 0;
char info[5] = "    ";

extern bool ledc_fade_enable, ledc_fade_pause, nightly_mode;
extern int spray_state, spray_time, spray_duty, spray_pause;

void display_time()
{
    time_t now = time(NULL);
    struct tm tmp = {0};
    gmtime_r(&now, &tmp);
    sprintf(info, "%02d%02d", tmp.tm_hour, tmp.tm_min);
    printf("%s\n", info);
    TM1620_Print(info);
}

int control_mode = 0;
bool control_pause = false;
time_t control_pause_time = 2 * 60, last_pause_time = 0;

void control_start()
{
    ;
}

void control_close()
{
    set_ledc(false, false, false), all_ledc_off();

    spray_time = spray_state = 0, all_spray_off();

    // aroma_stop(&Player);
    
    view_mode = view_idle;
}

void control_task(time_t now)
{
    if (view_mode != view_config)
    {
        struct tm tmp_now = {0}, tmp = {0};
        gmtime_r(&now, &tmp_now);
        
        {
            gmtime_r(&getup_time, &tmp);

            printf("getup %ld %ld hour %d %d min %d %d\n", now, sleep_time, tmp_now.tm_hour, tmp.tm_hour, tmp_now.tm_min, tmp.tm_min);
        
        
            if(control_mode == 0 && tmp_now.tm_hour == tmp.tm_hour)
            {
                if(abs(tmp_now.tm_min - tmp.tm_min) < 3)
                {
                    getup_time = now; // 同步时间
                    
                    control_mode = 1;
                    
                    set_spray(60 * 60, 10, true, true);
                    // set_ledc(true, false, true);
                    
                    control_pause = false;
                    
                    // aroma_music(&Player, getup_mp3_start, getup_mp3_end);

                    // aroma_volume(&Player, 25);
                    
                    // aroma_play(&Player);
                    
                    printf("ready getup_time %ld\n", getup_time);
                    
                }
            }
            
            if (control_mode == 1)
            {
                // 起床时间期间的状态机。

                if (control_pause)
                {  
                    if (now > last_pause_time)
                    {
                        control_pause = false; // 恢复过来
                    }
                }
                else
                {
                    printf("now - getup_time %ld\n", now - getup_time);
                    
                    if (now % 60 == 0 && now - getup_time >= 20 * 60)
                    {
                        puts("replay getup music");
                        vTaskDelay(100 / portTICK_RATE_MS); // 100ms 
                        aroma_music(&Player, getup_mp3_start, getup_mp3_end);
                        aroma_resume(&Player);
                        
                        if(Player.player_volume != 50 && now - getup_time >= 20 * 60)
                        {
                            aroma_volume(&Player, 50);
                        }
                        
                        if(Player.player_volume != 75 && now - getup_time >= 25 * 60)
                        {
                            aroma_volume(&Player, 75);
                        }
                        
                        if(Player.player_volume != 100 && now - getup_time >= 30 * 60)
                        {
                            aroma_volume(&Player, 100);
                        }
                    }
                    

                    if (ledc_fade_enable == false && now > (getup_time + 10 * 60))
                    {
                        set_ledc(true, false, true);
                        puts("getup ledc_fade_enable");
                    }

                    if (music_is_play == false && now > (getup_time + 19 * 60))
                    {
                        aroma_music(&Player, getup_mp3_start, getup_mp3_end);

                        aroma_volume(&Player, 25);
                    
                        aroma_play(&Player);
                    }
                    
                    if (ledc_fade_pause == false && now > (getup_time + 30 * 60))
                    {
                        set_ledc(true, true, true);
                        puts("getup ledc_fade_pause");
                    }
                }
                
                if (now - getup_time > 60 * 60)
                {
                    control_mode = 0;
                    control_close();
                    puts("getup exit");
                }
            }
            
        }
        
        {
            gmtime_r(&sleep_time, &tmp);

            printf("sleep %ld %ld hour %d %d min %d %d\n", now, sleep_time, tmp_now.tm_hour, tmp.tm_hour, tmp_now.tm_min, tmp.tm_min);
        
            if(control_mode == 0 && tmp_now.tm_hour == tmp.tm_hour)
            {
                if(abs(tmp_now.tm_min - tmp.tm_min) < 3)
                {
                    sleep_time = now; // 同步时间
                    control_mode = 2;
                    set_spray(60 * 60, 10, true, false);
                    set_ledc(true, false, false);
                    
                    aroma_music(&Player, sleep_mp3_start, sleep_mp3_end);

                    aroma_volume(&Player, 100);
                    aroma_play(&Player);
                    
                    printf("ready sleep_time %ld\n", sleep_time);
                    
                }
            }

            if (control_mode == 2)
            {
                // 睡觉时间期间的状态机
                
                printf("now - sleep_time %ld\n", now - sleep_time);
                
                if (now % 60 == 0 && now - sleep_time < 45 * 60)
                {
                    puts("replay sleep music");
                    vTaskDelay(100 / portTICK_RATE_MS); // 100ms 
                    aroma_music(&Player, sleep_mp3_start, sleep_mp3_end);
                    aroma_resume(&Player);
                    
                    if(Player.player_volume != 75 && now - sleep_time >= 10 * 60)
                    {
                        aroma_volume(&Player, 75);
                    }
                    
                    if(Player.player_volume != 50 && now - sleep_time >= 20 * 60)
                    {
                        aroma_volume(&Player, 50);
                    }
                    
                    if(Player.player_volume != 25 && now - sleep_time >= 30 * 60)
                    {
                        aroma_volume(&Player, 25);
                    }
                }
                
                if (ledc_fade_enable && now - sleep_time > 45 * 60)
                {
                    set_ledc(false, false, false);
                    aroma_stop(&Player);
                    puts("sleep mid");
                }
                
                if (now - sleep_time > 60 * 60)
                {
                    control_mode = 0;
                    control_close();
                    puts("sleep exit");
                }
            }
        }
        
    }

}

void button_13_PRESSED()
{
    TM1620_Print(" A  ");
    
    if (control_mode == 0)
    {
        static int last_state = 0;
        if (last_state)
        {
            ledc_fade_pause = ledc_fade_enable = false;
        }
        else
        {
            ledc_fade_pause = ledc_fade_enable = true;
        }
        last_state = !last_state;
    }

    if (control_mode == 1)
    {
        // 起床 需要 懒觉
        
        if (control_pause == false)
        {
            control_pause = true;
            set_ledc(false, false, false), all_ledc_off();
            aroma_stop(&Player);
            last_pause_time = time(NULL) + 2 * 60; // 设定一个未来时间，时间到达可以恢复任务。
        }
    }
    
    vTaskDelay(1000 / portTICK_RATE_MS);
    display_time();
    
}

void button_13_LONG_PRESSED()
{
    if (control_mode != 0)
    {
        control_mode = 0;
    }
    control_close();
    aroma_stop(&Player);
}

void button_13_LONG_RELEASE()
{
    control_close();
    
}

void button_32_PRESSED()
{
    if (view_mode == view_config)
    {
        *time_data += 60;
    }
    else
    {
        nightly_mode = !nightly_mode;
        if(nightly_mode)
        {
            TM1620_Print(" L ");
        }
        else
        {
            TM1620_Print(" P ");
        }
    }
}

void button_32_LONG_PRESSED()
{
    if (view_mode == view_config)
    {
        time_change = +10;
    }
    else
    {
        ledc_fade_enable = true;
    }
}

void button_32_LONG_RELEASE()
{
    if (view_mode == view_config)
    {
        time_change = 0;
    }
    else
    {
        ledc_fade_enable = false;
        display_time();
    }
}

time_t config_timeout = 0;

void button_33_PRESSED()
{
    switch (view_mode)
    {
        case view_idle:
            config_mode = view_local_time;
            // get config
            view_mode = view_config;
            AromaCfg tmp;
            if(aroma_config_read(&tmp))
            {
                getup_time = tmp.getup_time;
                sleep_time = tmp.sleep_time;

                // FLAG 00  修正偏移的任务时间
                getup_time += 30*60; // 起床前提前半小时触发
                sleep_time += 60*60; // 睡觉前提前一小时触发

                local_time = time(NULL) + 15;

                if (local_time < tmp.local_time)
                {
                    local_time = tmp.local_time;
                }

                printf("config read %ld %ld %ld\n", local_time, sleep_time, getup_time);
            }
            // set config timeout 10 * 60 s
            config_timeout = time(NULL) + 10 * 60;
            break;
    }

    switch (config_mode)
    {
        case view_idle:
        {
            // set config
            view_mode = view_idle;
            {
                AromaCfg tmp;

                // FLAG 00  修正偏移的任务时间
                getup_time -= 30*60; // 起床前提前半小时触发
                sleep_time -= 60*60; // 睡觉前提前一小时触发

                tmp.getup_time = getup_time < 0 ? getup_time + 24*60*60 : getup_time;
                tmp.sleep_time = sleep_time < 0 ? sleep_time + 24*60*60 : sleep_time;
                tmp.local_time = local_time < 0 ? local_time + 24*60*60 : local_time;

                if(aroma_config_write(&tmp))
                {
                    printf("config writed\n");
                }

                printf("config writed %ld %ld %ld\n", local_time, sleep_time, getup_time);

                set_local_time(local_time);
                sync_time(); // 对一下时间
            }

            TM1620_Print("    ");
            vTaskDelay(1000 / portTICK_RATE_MS);

            {
                time_t now = time(NULL);
                struct tm tmp = {0};
                gmtime_r(&now, &tmp);
                sprintf(info, "%02d%02d", tmp.tm_hour, tmp.tm_min);
                printf("%s\n", info);
                TM1620_Print(info);
            }

            break;
        }
        case view_local_time:
            TM1620_Print(" F1 ");
            vTaskDelay(1000 / portTICK_RATE_MS);
            time_data = &local_time;
            config_mode = view_getup_time;
            break;
        case view_getup_time:
            TM1620_Print(" F2 ");
            vTaskDelay(1000 / portTICK_RATE_MS);
            time_data = &getup_time;
            config_mode = view_sleep_time;
            break;
        case view_sleep_time:
            TM1620_Print(" F3 ");
            vTaskDelay(1000 / portTICK_RATE_MS);
            time_data = &sleep_time;
            config_mode = view_idle;
            break;
    }

}

void button_33_RELEASE()
{
    
}

void button_33_LONG_PRESSED()
{
    if (view_mode == view_config)
    {
        view_mode = view_idle;
    }

    all_spray_on();
    TM1620_Print(" H  ");
}

void button_33_LONG_RELEASE()
{
    all_spray_off();
    display_time();
}

void button_27_PRESSED()
{
    if (view_mode == view_config)
    {
        *time_data -= 60;
    }
    else
    {
        is_view = !is_view;

        aroma_stop(&Player);

        if(!is_view)
        {
            aroma_music(&Player, sleep_mp3_start, sleep_mp3_end);
            TM1620_Print("    ");
        }
        else
        {
            aroma_music(&Player, getup_mp3_start, getup_mp3_end);
            display_time();
        }
    }
}

void button_27_LONG_PRESSED()
{
    if (view_mode == view_config)
    {
        time_change = -10;
    }
    else
    {
        TM1620_Print(" U  ");
        aroma_stop(&Player);
        aroma_volume(&Player, 100);
        aroma_play(&Player);
    }
}

void button_27_LONG_RELEASE()
{
    if (view_mode == view_config)
    {
        time_change = 0;
    }
    else
    {
        aroma_stop(&Player);
        display_time();
    }
}

void task_view(void *arg)
{
    // iic_init();
    
    sync_time(); // 对一下时间

    control_close();

    if (aroma_config_init() == false)
    {
        puts("flash config error");
        esp_restart();
    }
    else
    {
        AromaCfg tmp;
        if(aroma_config_read(&tmp))
        {
            // 确保数据是原始的
            getup_time = tmp.getup_time;
            sleep_time = tmp.sleep_time;
            local_time = tmp.local_time;

            // FLAG 00  修正偏移的任务时间
            getup_time -= 30*60; // 起床前提前半小时触发
            sleep_time -= 60*60; // 睡觉前提前一小时触发
        }
        else
        {
            // 初始化配置 
            tmp.getup_time = 12*60*60;
            tmp.sleep_time = 24*60*60;
            tmp.local_time = 6*60*60;
            
            if(aroma_config_write(&tmp))
            {
                printf("config writed\n");
                esp_restart();
            }
            
        }

        printf("config %ld %ld %ld\n", local_time, sleep_time, getup_time);
    }

    printf("time %ld local_time %ld\n", time(NULL), local_time);
    if (local_time - time(NULL) > 2 * 60 * 60) // 掉电超过一小时，取回回存储配置
    {
        set_local_time(local_time);
        sync_time(); // 对一下时间
    }

    // vTaskDelay(10000 / portTICK_RATE_MS);

    display_time();

    while(true)
    {
        time_t now = time(NULL);
        // printf("now %ld\n", now);
        if(0 == now % 4)
        {
            TM1620_Init();
        } 
        /*
        if(0 == now % (60 * 60)) // 每过一小时，同步回存储配置
        {
            AromaCfg tmp;
            tmp.getup_time = getup_time < 0 ? getup_time + 24*60*60 : getup_time;
            tmp.sleep_time = sleep_time < 0 ? sleep_time + 24*60*60 : sleep_time;
            
            sync_time();
            tmp.local_time = time(NULL);
            
            // 修正上电时没有配置导致的自动启动。

            if(aroma_config_write(&tmp))
            {
                printf("config writed\n");
            }
        }
        */
        switch (view_mode)
        {
            case view_idle:
            {
                static struct tm last = {0};
                // printf("now %ld\n", now);

                struct tm tmp = {0};
                gmtime_r(&now, &tmp);

                if (is_view)
                {
                    if(tmp.tm_hour != last.tm_hour || last.tm_min != tmp.tm_min)
                    {
                        sprintf(info, "%02d%02d", tmp.tm_hour, tmp.tm_min);
                        last.tm_hour = tmp.tm_hour;
                        last.tm_min = tmp.tm_min;
                        
                        printf("%s\n", info);
                        TM1620_Print(info);
                    }
                }
                else
                {
                    TM1620_Print("    ");
                }

                vTaskDelay(1000 / portTICK_RATE_MS);
                break;
            }
            case view_config:

                if (time_change != 0)
                {
                    *time_data += time_change * 60;
                }
                
                static time_t bak = 0;
                if(bak != *time_data)
                {
                    bak = *time_data;
                    struct tm tm = {0};
                    gmtime_r(time_data, &tm);
                    sprintf(info, "%02d%02d", tm.tm_hour, tm.tm_min);
                    // printf("%s\n", info);
                    TM1620_Print(info);
                }

                vTaskDelay(250 / portTICK_RATE_MS);

                if (now > config_timeout)
                {
                    TM1620_Print("    ");
                    view_mode = view_idle;
                    vTaskDelay(1000 / portTICK_RATE_MS);
                    display_time();
                    puts("config timeout");
                }

                break;
        }
        
        control_task(now);
    }
}

void unit_test_view()
{
    xTaskCreate(task_view, "task_view", 8192, NULL, 4, NULL);
}
