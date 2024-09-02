/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/settings/settings.h>


#define SERVICE_DATA_LEN        9
#define SERVICE_UUID            0xfcd2      /* BTHome service UUID */
#define IDX_TEMPL               4           /* Index of lo byte of temp in service data*/
#define IDX_TEMPH               5           /* Index of hi byte of temp in service data*/


#define ADV_PARAM BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY, \
                  BT_GAP_ADV_SLOW_INT_MIN, \
                  BT_GAP_ADV_SLOW_INT_MAX, NULL)

/*
#define ADV_PARAM BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY, \
                  BT_GAP_ADV_SLOW_INT_MIN, \
                  BT_GAP_PER_ADV_MAX_TIMEOUT, NULL)
*/

static uint8_t service_data[SERVICE_DATA_LEN] = {
    BT_UUID_16_ENCODE(SERVICE_UUID),
    0x40,
    0x02,    /* Temperature */
    0xc4,    /* Low byte */
    0x00,   /* High byte */
    0x03,    /* Humidity */
    0xbf,    /* 50.55%  low byte*/
    0x13,   /* 50.55%  high byte*/
};

static struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
    BT_DATA(BT_DATA_SVC_DATA16, service_data, ARRAY_SIZE(service_data))
};

void set_address()
{
    bt_addr_le_t addr[CONFIG_BT_ID_MAX];
    size_t count;
    char bt_addr_str[BT_ADDR_LE_STR_LEN];

    int err;

    err = settings_load();
    if (err) {
        printk("Error settings_load (err %d)\n", err);
    }

    bt_id_get(NULL, &count);
    printk("bt_id_get count %d\n", count);
    count = CONFIG_BT_ID_MAX;
    bt_id_get(addr, &count);

    bt_addr_le_to_str(&addr[count-1], bt_addr_str, BT_ADDR_LE_STR_LEN);
    printk("Random address preset %s\n", bt_addr_str);

    // Need CONFIG_BT_ID_MAX=2 in prj.conf
    err = bt_id_create(NULL, NULL);    // Change Identity->F9:28:A7:55:AB:FD (random)
    //err = bt_id_reset(0, NULL, NULL);

    if (err<0) {
        printk("Error bt_id_create (err %d)\n", err);
    }
    else {
        err = settings_save();
        if (err<0) {
           printk("Error settings_save (err %d)\n", err);
        }
    }


    bt_id_get(NULL, &count);
    printk("bt_id_get count %d\n", count);
    count = CONFIG_BT_ID_MAX;
    bt_id_get(addr, &count);

    bt_addr_le_to_str(&addr[1], bt_addr_str, BT_ADDR_LE_STR_LEN);
    printk("Random address set %s\n", bt_addr_str);
}

void set_random_bt_address(void)
{
    bt_addr_le_t addr;

    addr.a.val[0] = 0x7f;
    addr.a.val[1] = 0x6e;
    addr.a.val[2] = 0x5d;
    addr.a.val[3] = 0x4c;
    addr.a.val[4] = 0x7b;
    addr.a.val[5] = 0x3a;
    addr.type = BT_ADDR_LE_RANDOM;
    //BT_ADDR_SET_STATIC(&addr.a);
 
    int err;

    //bt_addr_le_copy(&addr, BT_ADDR_LE_NONE);
    //if (err) {
    //    printk("Error bt_addr_le_copy (err %d)\n", err);
    //}

    //err = bt_addr_le_create_static(&addr);
    //if (err) {
    //    printk("Error bt_addr_le_create_static (err %d)\n", err);
    //}
    //printk("Random address set %d %x:%x:%x:%x:%x:%x\n", addr.type, addr.a.val[0], addr.a.val[1], addr.a.val[2], addr.a.val[3], addr.a.val[4], addr.a.val[5]);

    printk("Set address.\n");

    err = bt_id_create(&addr, NULL);    // Change Identity->F9:28:A7:55:AB:FD (random)

    if (err) {
        printk("Error bt_id_create (err %d)\n", err);
    }

    char bt_addr_str[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(&addr, bt_addr_str, BT_ADDR_LE_STR_LEN);
    printk("Random address set %s\n", bt_addr_str);
}

#if False
int main(void)
{
    int err;
    int temp = 0;

    printk("Starting BTHome sensor template\n");

    set_random_bt_address();

    int sequence_count = 0;

    for (;;) {
        printk("Start loop.\n");
    
        if (sequence_count<=0)
        {
            printk("Start Advertisement, loop: %d.\n", sequence_count);

            /* Simulate temperature from 0C to 25C */
            service_data[IDX_TEMPH] = (temp * 100) >> 8;
            service_data[IDX_TEMPL] = (temp * 100) & 0xff;
            if (temp++ == 25) {
                temp = 0;
            }
        
            /* Initialize the Bluetooth Subsystem */
            err = bt_enable(bt_ready);
            if (err) {
                printk("Bluetooth init failed (err %d)\n", err);
                return 0;
            }

            // Wait up to 10 seconds for BT ready.
            for (int try_count=0; try_count<100; ++try_count)
            {
                k_msleep(100);
                if (bt_is_ready())
                {
                    break;
                }
            }
            k_msleep(100);

            printk("Advertisement started.\n");

            // Update data
            err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
            if (err) {
                printk("Failed to update advertising data (err %d)\n", err);
            }

            // Give time to transmit
            k_sleep(K_SECONDS(60));

            // Go to sleep.
            err = bt_disable();
            if (err) {
                printk("Failed to stop bluetooth (err %d)\n", err);
            }

            printk("Advertisement stopped.\n");

            sequence_count = 9;
        }
        else
        {
            --sequence_count;
        }
    
        k_sleep(K_MINUTES(1));
    }
    return 0;
}
#else
int main(void)
{
    int err;
    int temp = 0;

    printk("Starting BTHome sensor template\n");

    set_random_bt_address();

    /* Initialize the Bluetooth Subsystem */
    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return 0;
    }

    // set_address();

    printk("Bluetooth initialized\n");

    // Wait up to 10 seconds for BT ready.
    for (int try_count=0; try_count<100; ++try_count)
    {
        k_msleep(100);
        if (bt_is_ready())
        {
            break;
        }
    }

    int sequence_count = 0;

    for (;;) {
        printk("Start loop.\n");
    
        if (sequence_count<=0)
        {
            printk("Start Advertisement, loop: %d.\n", sequence_count);

            /* Simulate temperature from 0C to 25C */
            service_data[IDX_TEMPH] = (temp * 100) >> 8;
            service_data[IDX_TEMPL] = (temp * 100) & 0xff;
            if (temp++ == 25) {
                temp = 0;
            }
        
            /* Start advertising */
            err = bt_le_adv_start(ADV_PARAM, ad, ARRAY_SIZE(ad), NULL, 0);
            if (err) {
                printk("Advertising failed to start (err %d)\n", err);
            }
            else {
                printk("Advertising started\n");
            }

            // Update data
            err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
            if (err) {
                printk("Failed to update advertising data (err %d)\n", err);
            }

            // Give time to transmit
            k_sleep(K_SECONDS(60));

            /* Stop advertising */
            err = bt_le_adv_stop();
            if (err) {
                printk("Advertising failed to stop 1 (err %d)\n", err);
            }
            
            printk("Advertisement stopped.\n");

            sequence_count = 9;
        }
        else
        {
            --sequence_count;
        }
    
        k_sleep(K_MINUTES(1));
    }
    return 0;
}
#endif