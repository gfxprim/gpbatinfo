//SPDX-License-Identifier: GPL-2.0-or-later

/*

    Copyright (C) 2022 Cyril Hrubis <metan@ucw.cz>

 */

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "power_supply.h"

static ssize_t read_line(int dir_fd, const char *fname, char *buf, ssize_t buf_len)
{
	int fd = openat(dir_fd, fname, O_RDONLY);
	ssize_t ret;

	if (!fd)
		return -1;

	ret = read(fd, buf, buf_len);

	close(fd);

	return ret;
}

static enum ps_type get_type(int dir_fd)
{
	char buf[16];

	if (read_line(dir_fd, "type", buf, sizeof(buf)) < 0)
		return PS_UNKNOWN;

	switch (buf[0]) {
	case 'B':
		return PS_BATTERY;
	case 'U':
		if (buf[1] == 'P')
			return PS_UPS;
		return PS_USB;
	case 'M':
		return PS_MAINS;
	case 'W':
		return PS_WIRELESS;
	default:
		return PS_UNKNOWN;
	}
}

const char *ps_type_name(enum ps_type type)
{
	switch (type) {
	case PS_UNKNOWN:
		return "Unknown";
	case PS_BATTERY:
		return "Battery";
	case PS_UPS:
		return "UPS";
	case PS_USB:
		return "USB";
	case PS_MAINS:
		return "Mains";
	case PS_WIRELESS:
		return "Wireless";
	default:
		return "Invalid";
	}
}

const char *ps_bat_status_name(enum ps_bat_status status)
{
	switch (status) {
	case PS_BAT_UNKNOWN:
		return "Unknown";
	case PS_BAT_CHARGING:
		return "Charging";
	case PS_BAT_DISCHARGING:
		return "Discharging";
	case PS_BAT_NOT_CHARGING:
		return "Not Charging";
	case PS_BAT_FULL:
		return "Full";
	default:
		return "Invalid";
	}
}

static struct ps *alloc_ps(enum ps_type type, int dir_fd)
{
	struct ps *ret = malloc(sizeof(struct ps));

	memset(ret, 0, sizeof(struct ps));

	ret->dir_fd = dir_fd;
	ret->type = type;

	return ret;
}

static enum ps_bat_status get_bat_status(int dir_fd)
{
	char buf[16];

	if (read_line(dir_fd, "status", buf, sizeof(buf)) <= 0)
		return PS_BAT_UNKNOWN;

	switch (buf[0]) {
	case 'C':
		return PS_BAT_CHARGING;
	case 'D':
		return PS_BAT_DISCHARGING;
	case 'N':
		return PS_BAT_NOT_CHARGING;
	case 'F':
		return PS_BAT_FULL;
	case 'U':
	default:
		return PS_BAT_UNKNOWN;
	}
}

static uint32_t get_uint32(int dir_fd, const char *fname)
{
	char buf[16];

	if (read_line(dir_fd, fname, buf, sizeof(buf)) <= 0)
		return 0;

	return atoll(buf);
}

static void fill_string(int dir_fd, const char *fname, char *dest, size_t dest_len)
{
	ssize_t ret;

	ret = read_line(dir_fd, fname, dest, dest_len);
	dest[ret < 1 ? 0 : ret-1] = 0;
}

static void bat_refresh(struct ps *ps)
{
	ps->bat.status = get_bat_status(ps->dir_fd);
	ps->bat.voltage = get_uint32(ps->dir_fd, "voltage_now");
	ps->bat.energy_now = get_uint32(ps->dir_fd, "energy_now");
	ps->bat.power_now = get_uint32(ps->dir_fd, "power_now");

	ps->bat.power_avg += ((int32_t)ps->bat.power_now - (int32_t)ps->bat.power_avg) / 8;
	ps->bat.voltage_avg += ((int32_t)ps->bat.voltage - (int32_t)ps->bat.voltage_avg) / 8;

}

static void bat_populate(struct ps *ps)
{
	ps->bat.cycle_count = get_uint32(ps->dir_fd, "cycle_count");
	ps->bat.energy_full = get_uint32(ps->dir_fd, "energy_full");
	ps->bat.energy_design = get_uint32(ps->dir_fd, "energy_full_design");
	fill_string(ps->dir_fd, "technology", ps->bat.technology, sizeof(ps->bat.technology));

	ps->bat.voltage_avg = get_uint32(ps->dir_fd, "voltage_now");

	bat_refresh(ps);
}

uint32_t ps_bat_sec_rem(struct ps *ps)
{
	uint64_t diff;

	switch (ps->bat.status) {
	case PS_BAT_CHARGING:
		diff = ps->bat.energy_full - ps->bat.energy_now;
	break;
	case PS_BAT_DISCHARGING:
		diff = ps->bat.energy_now;
	break;
	default:
		return 0;
	}

	return 3600 * diff / ps->bat.power_avg;
}

uint32_t ps_bat_current_avg(struct ps *ps)
{
	if (!ps->bat.voltage_avg)
		return 0;

	return 1000000 * (uint64_t)ps->bat.power_avg / ps->bat.voltage_avg;
}

static void mains_refresh(struct ps *ps)
{
	char buf[16];

	if (read_line(ps->dir_fd, "online", buf, sizeof(buf)) <= 0)
		return;

	switch (buf[0]) {
	case '0':
		ps->mains.online = 0;
	break;
	case '1':
		ps->mains.online = 1;
	break;
	}
}

static void mains_populate(struct ps *ps)
{
	mains_refresh(ps);
}

void ps_refresh(struct ps *root)
{
	struct ps *i;

	for (i = root; i; i = i->next) {
		switch (i->type) {
		case PS_MAINS:
			mains_refresh(i);
		break;
		case PS_BATTERY:
			bat_refresh(i);
		break;
		default:
		break;
		}

	}
}

struct ps *ps_init(enum ps_type type_filter)
{
	int dir_fd;
	DIR *dir;
	struct dirent *entry;
	struct ps *ret = NULL;

	dir_fd = open("/sys/class/power_supply", O_DIRECTORY);
	if (!dir_fd) {
		return NULL;
	}

	dir = fdopendir(dir_fd);
	if (!dir) {
		close(dir_fd);
		return NULL;
	}

	while ((entry = readdir(dir))) {

		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;

		int cdir_fd = openat(dir_fd, entry->d_name, O_DIRECTORY);
		if (!cdir_fd)
			continue;

		enum ps_type type = get_type(cdir_fd);

		if (!(type & type_filter))
			continue;

		struct ps *new = alloc_ps(type, cdir_fd);
		if (!new) {
			close(cdir_fd);
			continue;
		}

		switch (type) {
		case PS_BATTERY:
			bat_populate(new);
		break;
		case PS_MAINS:
			mains_populate(new);
		break;
		default:
		break;
		}

		new->next = ret;
		ret = new;
	}

	closedir(dir);

	return ret;
}

void ps_print(struct ps *root)
{
	struct ps *i;

	for (i = root; i; i = i->next) {
		printf("Power source:\t%s\n", ps_type_name(i->type));
		printf("-----------------------\n");

		switch (i->type) {
		case PS_BATTERY:
			printf("\n");
			printf("Cycle count:\t%u\n", i->bat.cycle_count);
			printf("Voltage:\t%u.%u V\n", i->bat.voltage/1000000, i->bat.voltage%1000000);
			printf("Technology:\t%s\n", i->bat.technology);
			printf("\n");
			printf("Status:\t\t%s\n", ps_bat_status_name(i->bat.status));
			printf("Energy now:\t%u.%u mWh\n", i->bat.energy_now/1000, i->bat.energy_now%1000);
			printf("Energy full:\t%u.%u mWh\n", i->bat.energy_full/1000, i->bat.energy_full%1000);
			printf("Energy design:\t%u.%u mWh\n", i->bat.energy_design/1000, i->bat.energy_design%1000);
		break;
		case PS_MAINS:
			printf("\n");
			printf("Online:\t\t%s\n", i->mains.online ? "Yes" : "No");
		break;
		default:
		break;
		}

		if (i->next)
			printf("\n\n");
	}
}

void ps_exit(struct ps *root)
{
	struct ps *j, *i = root;

	while (i) {
		j = i;
		i = i->next;

		close(j->dir_fd);
		free(j);
	}
}
