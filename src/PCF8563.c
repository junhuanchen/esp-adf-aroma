
#include "esp_system.h"
#include "PCF8563.h"
#include "driver/i2c.h"
#include "timegm.h"

#include <stdlib.h>
#include <time.h>
#include <sys/time.h>


static esp_err_t last_i2c_err = ESP_OK; 

esp_err_t PCF_Write(uint8_t addr, uint8_t *data, size_t count) {

	last_i2c_err = ESP_OK;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, PCF8563_WRITE_ADDR, true);
	i2c_master_write_byte(cmd, addr, true);
	i2c_master_write(cmd, data, count, true);
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
	last_i2c_err = ret;
	printf("last_i2c_err %d\n", last_i2c_err);
    return ret;
}

esp_err_t PCF_Read(uint8_t addr, uint8_t *data, size_t count) {

	last_i2c_err = ESP_OK;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, PCF8563_WRITE_ADDR, true);
	i2c_master_write_byte(cmd, addr, true);
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, PCF8563_READ_ADDR, true);
	i2c_master_read(cmd, data, count, I2C_MASTER_LAST_NACK);
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
	last_i2c_err = ret;
	printf("last_i2c_err %d\n", last_i2c_err);
    return ret;
}

esp_err_t PCF_GetLastError(){
	return last_i2c_err;
}

#define BinToBCD(bin) ((((bin) / 10) << 4) + ((bin) % 10))

int PCF_Init(uint8_t mode){
	static bool init = false;
	if(!init){
		uint8_t tmp = 0b00000000;
		esp_err_t ret = PCF_Write(0x00, &tmp, 1);
		if (ret != ESP_OK){
			return -1;
		}
		mode &= 0b00010011;
		ret = PCF_Write(0x01, &mode, 1);
		if (ret != ESP_OK){
			return -2;
		}
		init = true;
	}
	return 0;
}

int PCF_GetAndClearFlags(){
	uint8_t flags;

	esp_err_t ret = PCF_Read(0x01, &flags, 1);
	if (ret != ESP_OK){
		return -1;
	}
	uint8_t cleared = flags & 0b00010011;
	ret = PCF_Write(0x01, &cleared, 1);
	if (ret != ESP_OK){
		return -1;
	}

	return flags & 0x0C;
}

int PCF_SetClockOut(uint8_t mode){

	mode &= 0b10000011;
	esp_err_t ret = PCF_Write(0x0D, &mode, 1);
	if (ret != ESP_OK) {
		return -1;
	}
	return 0;	
}

int PCF_SetTimer(uint8_t mode, uint8_t count){

	mode &= 0b10000011;
	esp_err_t ret = PCF_Write(0x0E, &mode, 1);
	if (ret != ESP_OK) {
		return -1;
	}
	ret = PCF_Write(0x0F, &count, 1);
	if (ret != ESP_OK) {
		return -1;
	}
	return 0;
}

int PCF_GetTimer(){
	uint8_t count;

	esp_err_t ret = PCF_Read(0x0F, &count, 1);
	if (ret != ESP_OK) {
		return -1;
	}
	return (int) count;
}

int PCF_SetAlarm(PCF_Alarm *alarm){
	if ((alarm->minute >= 60 && alarm->minute != 80) || (alarm->hour >= 24 && alarm->hour != 80) || (alarm->day > 32 && alarm->day != 80) || (alarm->weekday > 6 && alarm->weekday != 80))
	{
		return -2;
	}

	uint8_t buffer[4];

	buffer[0] = BinToBCD(alarm->minute) & 0xFF;
	buffer[1] = BinToBCD(alarm->hour) & 0xBF;
	buffer[2] = BinToBCD(alarm->day) & 0xBF;
	buffer[3] = BinToBCD(alarm->weekday) & 0x87;

	esp_err_t ret = PCF_Write(0x09, buffer, sizeof(buffer));
	if (ret != ESP_OK) {
		return -1;
	}

	return 0;
}

int PCF_GetAlarm(PCF_Alarm *alarm) {
	uint8_t buffer[4];

	esp_err_t ret = PCF_Read(0x09, buffer, sizeof(buffer));
	if (ret != ESP_OK) {
		return -1;
	}

	alarm->minute = (((buffer[0] >> 4) & 0x0F) * 10) + (buffer[0] & 0x0F);
	alarm->hour = (((buffer[1] >> 4) & 0x0B) * 10) + (buffer[1] & 0x0F);
	alarm->day = (((buffer[2] >> 4) & 0x0B) * 10) + (buffer[2] & 0x0F);
	alarm->weekday = (((buffer[3] >> 4) & 0x08) * 10) + (buffer[3] & 0x07);

	return 0;
}

int PCF_SetDateTime(PCF_DateTime *dateTime) {
	if (dateTime->second >= 60 || dateTime->minute >= 60 || dateTime->hour >= 24 || dateTime->day > 32 || dateTime->weekday > 6 || dateTime->month > 12 || dateTime->year < 1900 || dateTime->year >= 2100)
	{
		return -2;
	}

	uint8_t buffer[7];

	buffer[0] = BinToBCD(dateTime->second) & 0x7F;
	buffer[1] = BinToBCD(dateTime->minute) & 0x7F;
	buffer[2] = BinToBCD(dateTime->hour) & 0x3F;
	buffer[3] = BinToBCD(dateTime->day) & 0x3F;
	buffer[4] = BinToBCD(dateTime->weekday) & 0x07;
	buffer[5] = BinToBCD(dateTime->month) & 0x1F;

	if (dateTime->year >= 2000)
	{
		buffer[5] |= 0x80;
		buffer[6] = BinToBCD(dateTime->year - 2000);
	}
	else
	{
		buffer[6] = BinToBCD(dateTime->year - 1900);
	}

	esp_err_t ret = PCF_Write(0x02, buffer, sizeof(buffer));
	if (ret != ESP_OK) {
		return -1;
	}

	return 0;
}

int PCF_GetDateTime(PCF_DateTime *dateTime) {
	uint8_t buffer[7];
	esp_err_t ret;

	ret = PCF_Read(0x02, buffer, sizeof(buffer));
	if (ret != ESP_OK) {
		return -1;
	}

	dateTime->second = (((buffer[0] >> 4) & 0x07) * 10) + (buffer[0] & 0x0F);
	dateTime->minute = (((buffer[1] >> 4) & 0x07) * 10) + (buffer[1] & 0x0F);
	dateTime->hour = (((buffer[2] >> 4) & 0x03) * 10) + (buffer[2] & 0x0F);
	dateTime->day = (((buffer[3] >> 4) & 0x03) * 10) + (buffer[3] & 0x0F);
	dateTime->weekday = (buffer[4] & 0x07);
	dateTime->month = ((buffer[5] >> 4) & 0x01) * 10 + (buffer[5] & 0x0F);
	dateTime->year = 1900 + ((buffer[6] >> 4) & 0x0F) * 10 + (buffer[6] & 0x0F);

	if (buffer[5] &  0x80)
	{
		dateTime->year += 100;
	}

	if (buffer[0] & 0x80) //Clock integrity not guaranted
	{
		return 1;
	}

	return 0;
}

int PCF_hctosys(){
	int ret;
	PCF_DateTime date = {0};
	struct tm tm = {0};
	struct timeval tv = {0};
	
	ret = PCF_Init(0);
	printf("PCF_Init %d\n", ret);
	if (ret != 0) {
		goto fail;
	}
    ret = PCF_GetDateTime(&date);
	printf("PCF_GetDateTime %d\n", ret);
    if (ret != 0) {
		goto fail;
    }
	tm.tm_sec = date.second;
	tm.tm_min = date.minute;
	tm.tm_hour = date.hour;
	tm.tm_mday = date.day;
	tm.tm_mon = date.month - 1;
	tm.tm_year = date.year - 1900;

	tv.tv_sec = timegm(&tm);
	tv.tv_usec = 0;
	ret = settimeofday(&tv, NULL);
fail:
	return ret;
}

int PCF_systohc(){
	int ret;
	PCF_DateTime date = {0};
	struct tm tm = {0};

	ret = PCF_Init(0);
	if (ret != 0) {
		goto fail;
	}

	time_t now = time(NULL);
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

#ifdef main

#include "PCF8563.h"

void iic_set_up()
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

static const char *LOG_TAG = "PCF8563";

void app_main(void)
{
    iic_set_up();

    while (true) {
        
        int ret = PCF_hctosys();
        if (ret != 0) {
            
            PCF_systohc();

            ESP_LOGE(LOG_TAG, "Error reading hardware clock: %d", ret);
        }

        printf("ret %d time %ld\n ", ret, time(NULL));

        vTaskDelay(5000 / portTICK_RATE_MS);

    }

    return;
}
#endif