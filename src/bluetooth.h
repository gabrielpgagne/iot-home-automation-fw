#include <stdbool.h>
#include <stdint.h>

void ble_disable(void);
int ble_init(void);
void bt_update_temp(float temp);
void bt_update_humidity(float hm);
void bt_update_door_state(bool open);
void bt_update_battery(bool charging, uint8_t batt);
void bt_update_all(float temp, float hm, bool open);
int bt_publish(void);