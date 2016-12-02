#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <device.h>
#include <pwm.h>
#include <sys_clock.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "led-service.h"

#define LED_PRIMARY_UUID 0x32, 0xbd, 0x0a, 0x21, 0x09, 0xa0, 0xd8, 0x93, \
                                 0x49, 0x4d, 0xb6, 0xef, 0x0c, 0x07, 0x23, 0xe6
//e623 070c -efb6-4d49-93d8-a009210abd32
static struct bt_uuid_128 led_uuid = BT_UUID_INIT_128(LED_PRIMARY_UUID);

//297c 070c-655d-43ca-ac7d-15 d7 d8 f4 45 5e
static struct bt_uuid_128 led_brightness_uuid = BT_UUID_INIT_128(
        0x5e, 0x45, 0xf4, 0xd8, 0xd7, 0x15, 0x7d, 0xac,
        0xca, 0x43, 0x5d, 0x65, 0x0c, 0x07, 0x7c, 0x29);

//fe29 070c-4f68-41fc-ab5d-3d 67 f7 09 3d eb
static const struct bt_uuid_128 led_signed_uuid = BT_UUID_INIT_128(
        0xeb, 0x3d, 0x09, 0xf7, 0x67, 0x3d, 0x5d, 0xab,
        0xfc, 0x41, 0x68, 0x4f, 0x0c, 0x07, 0x29, 0xfe);

//Scheduler ON/OFF
//0 - means no scheduler enabled
//any other value - means that time schedule is enabled, value represents client local time in 0xHHMM
//8b02070c-1314-4488-a618-de21e9eea8d8
static struct bt_uuid_128 led_scheduler_uuid = BT_UUID_INIT_128(
        0xd8, 0xa8, 0xee, 0xe9, 0x21, 0xde, 0x18, 0xa6,
        0x88, 0x44, 0x14, 0x13, 0x0c, 0x07, 0x02, 0x8b);

//9318 07 0c-5025-479a-bd36-b5c9abedc43c
static const struct bt_uuid_128 led_scheduler_signed_uuid = BT_UUID_INIT_128(
        0x3c, 0xc4, 0xed, 0xab, 0xc9, 0xb5, 0x36, 0xbd,
        0x9a, 0x47, 0x25, 0x50, 0x0c, 0x07, 0x18, 0x93);

//Scheduler start time
//b518 07 0c-7783-4a9e-8df8-ae84fc80a840
static struct bt_uuid_128 led_starttime_uuid = BT_UUID_INIT_128(
        0x40, 0xa8, 0x80, 0xfc, 0x84, 0xae, 0xf8, 0x8d,
        0x9e, 0x4a, 0x83, 0x77, 0x0c, 0x07, 0x18, 0xb5);

//f9 0e 07 0c-8d 09-4b 22-90 4c-ab02e243f9a6
static const struct bt_uuid_128 led_starttime_signed_uuid = BT_UUID_INIT_128(
        0xa6, 0xf9, 0x43, 0xe2, 0x02, 0xab, 0x4c, 0x90,
        0x22, 0x4b, 0x09, 0x8d, 0x0c, 0x07, 0x0e, 0xf9);

//Scheduler end time
//066c 07 0c-cb23-42f6-8d86-74695036cd1d
static struct bt_uuid_128 led_endtime_uuid = BT_UUID_INIT_128(
        0x1d, 0xcd, 0x36, 0x50, 0x69, 0x74, 0x86, 0x8d,
        0xf6, 0x42, 0x23, 0xcb, 0x0c, 0x07, 0x6c, 0x06);

//8b19 07 0c-8d03-477e-b124-33ebd5f3cbd9
static const struct bt_uuid_128 led_endtime_signed_uuid = BT_UUID_INIT_128(
        0xd9, 0xcb, 0xf3, 0xd5, 0xeb, 0x33, 0x24, 0xb1,
        0x7e, 0x47, 0x03, 0x8d, 0x0c, 0x07, 0x19, 0x8b);


struct device *pwm_dev;

static ssize_t read_brightness(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        void *buf, uint16_t len, uint16_t offset)
{
    const char *value = attr->user_data;

    return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
                             sizeof(brightness_value));
}

static ssize_t write_brightness(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                         const void *buf, uint16_t len, uint16_t offset,
                         uint8_t flags)
{
    uint8_t *value = attr->user_data;

    if (offset + len > sizeof(brightness_value)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(value + offset, buf, len);

    printk("write data: %d\n", brightness_value);
    printk("set durty cycle to %d\n", brightness_value);
    pwm_pin_set_duty_cycle(pwm_dev, 0, brightness_value);
    return len;
}

static ssize_t read_scheduler(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                               void *buf, uint16_t len, uint16_t offset)
{
    const char *value = attr->user_data;

    return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
                             sizeof(scheduler_onoff));
}

static ssize_t write_scheduler(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                const void *buf, uint16_t len, uint16_t offset,
                                uint8_t flags)
{
    uint8_t *value = attr->user_data;

    if (offset + len > sizeof(scheduler_onoff)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(value + offset, buf, len);


    seconds_before_sheduler_update = (int64_t)(_sys_clock_tick_count/sys_clock_ticks_per_sec);
    return len;
}

static ssize_t read_scheduler_time(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                              void *buf, uint16_t len, uint16_t offset)
{
    const char *value = attr->user_data;

    return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
                             sizeof(scheduler_onoff));
}

static ssize_t write_scheduler_time(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                               const void *buf, uint16_t len, uint16_t offset,
                               uint8_t flags)
{
    uint8_t *value = attr->user_data;

    if (offset + len > sizeof(scheduler_onoff)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(value + offset, buf, len);
    return len;
}

void check_scheduler(void) {
    int minutes = scheduler_onoff & 0xff;
    int hours = (scheduler_onoff & 0xff00) >> 2;
    int hours_passed = (_sys_clock_tick_count/sys_clock_ticks_per_sec - seconds_before_sheduler_update)/3600;
    int minutes_passed = ((_sys_clock_tick_count/sys_clock_ticks_per_sec - seconds_before_sheduler_update) - hours*3600)/60;
    hours += hours_passed;
    minutes += minutes_passed;
    if(minutes > 60) {
        hours++;
        minutes -= 60;
    }
    if(hours > 24) hours = 0;

    printk("Local time: %dH %dM\n", hours, minutes);
    printk("Seconds after start: %d\n", (int)(_sys_clock_tick_count/sys_clock_ticks_per_sec));
    printk("Scheduler: %d, start: %d -> end: %d\n", scheduler_onoff, start_time, end_time);
}


/* Starcontrol Service Declaration */
static struct bt_gatt_attr starcontrol_attrs[] = {
        BT_GATT_PRIMARY_SERVICE(&led_uuid),
        BT_GATT_CHARACTERISTIC(&led_brightness_uuid.uuid,
                               BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE),
        BT_GATT_DESCRIPTOR(&led_signed_uuid.uuid,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_brightness, write_brightness, &brightness_value),
        BT_GATT_CHARACTERISTIC(&led_scheduler_uuid.uuid,
                               BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE),
        BT_GATT_DESCRIPTOR(&led_scheduler_signed_uuid.uuid,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_scheduler, write_scheduler, &scheduler_onoff),
        BT_GATT_CHARACTERISTIC(&led_starttime_uuid.uuid,
                               BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE),
        BT_GATT_DESCRIPTOR(&led_starttime_signed_uuid.uuid,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_scheduler_time, write_scheduler_time, &start_time),
        BT_GATT_CHARACTERISTIC(&led_endtime_uuid.uuid,
                               BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE),
        BT_GATT_DESCRIPTOR(&led_endtime_signed_uuid.uuid,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_scheduler_time, write_scheduler_time, &end_time)
};

static const struct bt_data led_ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA_BYTES(BT_DATA_UUID128_ALL, LED_PRIMARY_UUID),
};

static const struct bt_data led_sd[] = {
        BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

void led_service_init(void) {
    bt_gatt_register(led_attrs, ARRAY_SIZE(led_attrs));

    int err = bt_le_adv_start(BT_LE_ADV_CONN, led_ad, ARRAY_SIZE(led_ad),
                          led_sd, ARRAY_SIZE(led_ad));
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    printk("Advertising successfully started\n");

    //create PWM
    pwm_dev = device_get_binding("PWM_0");
    if (!pwm_dev) {
        printk("Cannot find PWM_0!\n");
    }
    err = pwm_pin_set_duty_cycle(pwm_dev, 0, brightness_value);
    if(err) {
        printk("Failed to ser PWM duty cycles: %d (err %d)\n", brightness_value, err);
    }
}
