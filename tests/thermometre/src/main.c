/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/dt-bindings/gpio/gpio.h"
#include "zephyr/sys/time_units.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>

void get_sensor_data();

/*
 ***************************************************************************
 * Device tree definitions.
 ***************************************************************************
 */
#define SHTC_DEV DT_NODELABEL(shtcx)
const struct device *sht = DEVICE_DT_GET(SHTC_DEV);

/*
 ***************************************************************************
 * Bluetooth BtHome setup.
 * ref: https://bthome.io/format/
 ****************************************************************************
 */
#define SERVICE_DATA_LEN 9 /* 11 bytes of service data */
#define SERVICE_UUID 0xfcd2 /* BTHome service UUID */
#define IDX_TEMPL 4         /* Index of lo byte of temp in service data*/
#define IDX_TEMPH 5         /* Index of hi byte of temp in service data*/
#define IDX_HUMDL 7         /* Index of lo byte of humidity in service data*/
#define IDX_HUMDH 8         /* Index of hi byte of humidity in service data*/
#define IDX_DOOR  10         /* Index of door state */

static uint8_t service_data[SERVICE_DATA_LEN] = {
    BT_UUID_16_ENCODE(SERVICE_UUID),
    0x40,
    0x02, /* Temperature */
    0xc4, /* Low byte */
    0x00, /* High byte */
    0x03, /* Humidity */
    0xbf, /* low byte */
    0x13, /* high byte */
};

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
            sizeof(CONFIG_BT_DEVICE_NAME) - 1),
   BT_DATA(BT_DATA_SVC_DATA16,
            service_data, ARRAY_SIZE(service_data)),
};

#define ADV_PARAMS BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY, \
          BT_GAP_ADV_SLOW_INT_MIN, \
          BT_GAP_ADV_SLOW_INT_MAX, NULL)

/*
 ***************************************************************************
 * Bluetooth functions.
 ****************************************************************************
 */

void set_random_bt_address(void)
{
    bt_addr_le_t addr;

    addr.a.val[0] = 0xfd;
    addr.a.val[1] = 0xab;
    addr.a.val[2] = 0x55;
    addr.a.val[3] = 0xa7;
    addr.a.val[4] = 0x28;
    addr.a.val[5] = 0xf9;
    addr.type = BT_ADDR_LE_RANDOM;

	//BT_ADDR_SET_STATIC(&(addr.a));
 
    int err;

    printk("Set address.\n");

    err = bt_id_create(&addr, NULL);    // Change Identity->F9:28:A7:55:AB:FD (random)

    if (err) {
        printk("Error bt_id_create (err %d)\n", err);
    }

    char bt_addr_str[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(&addr, bt_addr_str, BT_ADDR_LE_STR_LEN);
    printk("Random address set %s\n", bt_addr_str);
}


/*
 ***************************************************************************
 * Main
 ****************************************************************************
 */
int main(void)
{
    printk("Thermometer !\n");

    // ----- SHT init -----
    if (!device_is_ready(sht)) {
        printk("Device %s is not ready\n", sht->name);
    } else {
        printk("Found device %s. Reading sensor data\n", sht->name);
    }
    
    // ----- BT init -----
    printk("CONFIG_BT_DEVICE_NAME: %s\n", CONFIG_BT_DEVICE_NAME);

    set_random_bt_address();

   	int err = bt_enable(NULL);
    if (err)
    {
        printk("Bluetooth init failed (err %d)\n", err);
        return 0;
    }
    
    get_sensor_data();

    /* Start advertising */
    err = bt_le_adv_start(ADV_PARAMS, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
    }

    printk("Bluetooth init done.\n");

    // ----- Main loop -----
    for (;;)
    {
        get_sensor_data();
    
        /* Update advertising data */
        // err = bt_le_adv_start(ADV_PARAMS, ad, ARRAY_SIZE(ad), NULL, 0);
        // if (err) {
        //     printk("Advertising failed to start (err %d)\n", err);
        // }

        err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
        if (err)
        {
            printk("Failed to update advertising data (err %d)\n", err);
        }
        else{
            printk("Advertising data updated\n");
        }
 
//        k_sleep(K_SECONDS(10));

        // /* Stop advertising */
        // err = bt_le_adv_stop();
        // if (err) {
        //     printk("Advertising failed to stop (err %d)\n", err);
        // }

        k_sleep(K_MINUTES(5));
    }
    return 0;
}

// Read sht sensor and fill adv data array.
void get_sensor_data()
{
    struct sensor_value temp, hum;

    int err = sensor_sample_fetch(sht);
    if (err == 0)
    {
        err = sensor_channel_get(sht, SENSOR_CHAN_AMBIENT_TEMP,
            &temp);
        if (err==0)
        {
            err = sensor_channel_get(sht, SENSOR_CHAN_HUMIDITY,
                &hum);
        }
    }
    if (err != 0)
    {
        printf("SHT: failed: %d\n", err);
    }
    else
    {
        double ftemp = sensor_value_to_double(&temp);
        double fhumd = sensor_value_to_double(&hum);
        // printf("SHT: %.2f Cel ; %0.2f %%RH\n", ftemp, fhumd);

        int temp_h = (int)(ftemp * 100) >> 8;
        int temp_l = (int)(ftemp * 100) & 0xff;
        int hum_h = (int)(fhumd * 100) >> 8;
        int hum_l = (int)(fhumd * 100) & 0xff;

        if (service_data[IDX_TEMPH] != temp_h
            || service_data[IDX_TEMPL] != temp_l
            || service_data[IDX_HUMDH] != hum_h
            || service_data[IDX_HUMDL] != hum_l)
        {
            service_data[IDX_TEMPH] = temp_h;
            service_data[IDX_TEMPL] = temp_l;

            service_data[IDX_HUMDH] = hum_h;
            service_data[IDX_HUMDL] = hum_l;
        }
    }
}