/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __FUEL_GAUGE_H__
#define __FUEL_GAUGE_H__

#include <zephyr/device.h>

int charger_get_soc(const struct device *charger);

#endif /* __FUEL_GAUGE_H__ */
