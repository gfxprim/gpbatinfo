//SPDX-License-Identifier: GPL-2.0-or-later

/*

    Copyright (C) 2022 Cyril Hrubis <metan@ucw.cz>

 */

#ifndef POWER_SUPPLY_H
#define POWER_SUPPLY_H

#include <stdint.h>

enum ps_type {
	PS_UNKNOWN = 0x00,
	PS_BATTERY = 0x01,
	PS_UPS = 0x02,
	PS_USB = 0x04,
	PS_MAINS = 0x08,
	PS_WIRELESS = 0x10,
};

enum ps_bat_status {
	PS_BAT_UNKNOWN,
	PS_BAT_CHARGING,
	PS_BAT_DISCHARGING,
	PS_BAT_NOT_CHARGING,
	PS_BAT_FULL,
};

struct ps_bat {
	uint8_t status;
	/* battery cycles */
	uint32_t cycle_count;
	/* in uV */
	uint32_t voltage;
	uint32_t voltage_avg;

	/* in uWh */
	uint32_t energy_now;
	uint32_t energy_full;
	uint32_t energy_design;
	/* power in uW */
	uint32_t power_now;
	uint32_t power_avg;
	/* Technology */
	char technology[16];
};

struct ps_mains {
	uint8_t online;
};

struct ps {
	enum ps_type type;
	struct ps *next;
	int dir_fd;
	union {
		struct ps_bat bat;
		struct ps_mains mains;
	};
	char id[];
};

struct ps *ps_init(enum ps_type type_filter);

const char *ps_bat_status_name(enum ps_bat_status status);

/**
 * @brief returns current time estimate to charge/discharge battery
 *
 * Uses power_now low pass, which needs some time to stabilize.
 *
 * @return Remaining time in seconds.
 */
uint32_t ps_bat_sec_rem(struct ps *ps);

/**
 * @brief Returns average current.
 *
 * @return Average current in uA.
 */
uint32_t ps_bat_current_avg(struct ps *ps);

void ps_print(struct ps *root);

void ps_refresh(struct ps *root);

void ps_exit(struct ps *root);

#endif /* POWER_SUPPLY_H */
