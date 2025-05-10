#include <zephyr/device.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define CHGR_BASE 0x03U

struct npm1300_charger_config
{
    const struct device *mfd;
    int32_t term_microvolt;
    int32_t term_warm_microvolt;
    int32_t current_microamp;
    int32_t dischg_limit_microamp;
    int32_t vbus_limit_microamp;
    int32_t temp_thresholds[4U];
    int32_t dietemp_thresholds[2U];
    uint32_t thermistor_ohms;
    uint16_t thermistor_beta;
    uint8_t thermistor_idx;
    uint8_t trickle_sel;
    uint8_t iterm_sel;
    bool charging_enable;
    bool vbatlow_charge_enable;
    bool disable_recharge;
};