
#include "esp_spiffs.h"
#include "esp_wifi_types.h"

#define AROMA_CONFIG_FILE "/spiffs/aroma.cfg"

typedef struct 
{
    int sleep_time, getup_time, local_time;
} AromaCfg;

static inline bool aroma_config_init()
{
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 3,
      .format_if_mount_failed = true
    };
    // esp_spiffs_format(NULL);
    return (ESP_OK == esp_vfs_spiffs_register(&conf));
}

static inline void aroma_config_exit()
{
    esp_vfs_spiffs_unregister(NULL);
}

static inline bool aroma_config_read(AromaCfg *config)
{
    bool res = false;
    FILE* f = fopen(AROMA_CONFIG_FILE, "rb");
    if(f)
    {
        // puts("fopen");
        if(1 == fread(config, sizeof(*config), 1, f))
        {
            // puts("fread");
            res = true;
        }
        fclose(f);
    }
    return res;
}

static inline bool aroma_config_write(AromaCfg *config)
{
    bool res = false;
    FILE* f = fopen(AROMA_CONFIG_FILE, "wb");
    if(f)
    {
        // puts("fopen");
        if(1 == fwrite(config, sizeof(*config), 1, f))
        {
            // puts("fwrite");
            res = true;
        }
        fclose(f);
    }
    return res;
}

static inline void unit_test_aroma_config()
{
    AromaCfg tmp;
    if (aroma_config_init())
    {
        if(aroma_config_read(&tmp))
        {
            tmp.sleep_time += 1;
            tmp.getup_time += 2;
            printf("sleep_time %d getup_time %d\n", tmp.sleep_time, tmp.getup_time);
        }
        else
        {
            printf("inited\n");
            tmp.sleep_time = 1;
            tmp.getup_time = 2;
        }
        if(aroma_config_write(&tmp))
        {
            printf("writed\n");
        }
        aroma_config_exit();
    }
}
