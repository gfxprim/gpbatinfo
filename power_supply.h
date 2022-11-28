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

enum ps_bat_energy_type {
	/* state_* is in uAh */
	PS_BAT_CHARGE = 0x00,
	/* state_* is in uWh */
	PS_BAT_ENERGY = 0x01,
};

struct ps_bat {
	/* enum ps_bat_status */
	uint8_t status;
	/* enum ps_bat_energy_type */
	uint8_t state_type;
	/* internal, do not touch */
	uint8_t idx;

	/* in uV */
	uint32_t voltage_now;
	uint32_t voltage_avg;
	/* in uA */
	uint32_t current_now;
	uint32_t current_avg;
	/* in uW */
	uint32_t power_now;
	uint32_t power_avg;

	/* in uWh or uAh depends on state_type */
	uint32_t state_now;
	uint32_t state_full;
	uint32_t state_design;

	/* battery cycles set to 0 if not available */
	uint32_t cycle_count;

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

static inline const char *ps_bat_state_unit(struct ps *ps)
{
	if (ps->bat.state_type == PS_BAT_CHARGE)
		return "Ah";

	return "Wh";
}

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

/**
 * @brief Returns current.
 *
 * @return Current in uA.
 */
uint32_t ps_bat_current_now(struct ps *ps);

/**
 * @brief Returns average power.
 *
 * @return Average power in uW.
 */
uint32_t ps_bat_power_avg(struct ps *ps);

/**
 * @brief Returns power.
 *
 * @return Power in uW.
 */
uint32_t ps_bat_power_now(struct ps *ps);

void ps_print(struct ps *root);

void ps_refresh(struct ps *root);

void ps_exit(struct ps *root);

#endif /* POWER_SUPPLY_H */
