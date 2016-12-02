#ifndef LED_SERVICE_H
#define LED_SERVICE_H

#define DEVICE_NAME		"NRF52 LED"
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)

static int brightness_value;
static int scheduler_onoff = 0;
static int start_time = 0x1800;
static int end_time = 0x1800;

static int64_t seconds_before_sheduler_update = 0;
extern void led_service_init(void);
extern void check_scheduler(void);


#endif // LED_SERVICE_H
