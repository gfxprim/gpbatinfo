//SPDX-License-Identifier: GPL-2.1-or-later

/*

    Copyright (C) 2022 Cyril Hrubis <metan@ucw.cz>

 */

#include <gfxprim.h>
#include "power_supply.h"

static struct ps *ps;

struct bat {
	gp_widget *status;
	gp_widget *voltage;
	gp_widget *current_avg;
	gp_widget *technology;
	gp_widget *cycle_count;

	gp_widget *energy_now;
	gp_widget *energy_full;
	gp_widget *energy_design;
	gp_widget *energy_pbar;
	gp_widget *power_now;
	gp_widget *time_rem;
} bat;

static gp_app_info app_info = {
	.name = "gpbatinfo",
	.desc = "Battery information",
	.version = "1.0",
	.license = "GPL-2.0-or-later",
	.url = "http://github.com/gfxprim/gpbatinfo",
	.authors = (gp_app_info_author []) {
		{.name = "Cyril Hrubis", .email = "metan@ucw.cz", .years = "2022"},
		{}
	}
};

static void update(void)
{
	if (bat.status)
		gp_widget_label_set(bat.status, ps_bat_status_name(ps->bat.status));

	if (bat.voltage)
		gp_widget_label_printf(bat.voltage, "%u.%03u V", ps->bat.voltage/1000000, (ps->bat.voltage%1000000)/1000);

	if (bat.current_avg) {
		uint32_t current_avg = ps_bat_current_avg(ps);
		gp_widget_label_printf(bat.current_avg, "%u.%03u A", current_avg/1000000, (current_avg%1000000)/1000);
	}

	if (bat.energy_now)
		gp_widget_label_printf(bat.energy_now, "%u mWh", ps->bat.energy_now/1000);

	if (bat.energy_full)
		gp_widget_label_printf(bat.energy_full, "%u mWh", ps->bat.energy_full/1000);

	if (bat.energy_design)
		gp_widget_label_printf(bat.energy_design, "%u mWh", ps->bat.energy_design/1000);

	if (bat.power_now)
		gp_widget_label_printf(bat.power_now, "%u mW", ps->bat.power_now/1000);

	if (bat.energy_pbar) {
		gp_widget_pbar_set_max(bat.energy_pbar, ps->bat.energy_full);
		gp_widget_pbar_set(bat.energy_pbar, ps->bat.energy_now);
	}

	if (bat.time_rem) {
		uint32_t time_est = ps_bat_sec_rem(ps);
		gp_widget_label_printf(bat.time_rem, "%02i:%02i:%02i", time_est/3600, (time_est%3600)/60, (time_est%60));
	}
}

static uint32_t refresh_callback(gp_timer *self)
{
	(void)self;

	ps_refresh(ps);
	update();

	return 0;
}

static gp_timer refresh_tmr = {
	.period = 500,
	.callback = refresh_callback,
	.id = "Refresh timer"
};

int main(int argc, char *argv[])
{
	gp_htable *uids;
	gp_widget *layout = gp_app_layout_load("gpbatinfo", &uids);
	gp_app_info_set(&app_info);

	ps = ps_init(PS_BATTERY);
	if (!ps)
		gp_dialog_msg_run(GP_DIALOG_MSG_ERR, "Error", "Battery not found!");

	bat.status = gp_widget_by_uid(uids, "status", GP_WIDGET_LABEL);
	bat.voltage = gp_widget_by_uid(uids, "voltage", GP_WIDGET_LABEL);
	bat.current_avg = gp_widget_by_uid(uids, "current_avg", GP_WIDGET_LABEL);
	bat.technology = gp_widget_by_uid(uids, "technology", GP_WIDGET_LABEL);
	bat.cycle_count = gp_widget_by_uid(uids, "cycle_count", GP_WIDGET_LABEL);

	bat.energy_now = gp_widget_by_uid(uids, "energy_now", GP_WIDGET_LABEL);
	bat.energy_full = gp_widget_by_uid(uids, "energy_full", GP_WIDGET_LABEL);
	bat.energy_design = gp_widget_by_uid(uids, "energy_design", GP_WIDGET_LABEL);
	bat.power_now = gp_widget_by_uid(uids, "power_now", GP_WIDGET_LABEL);
	bat.energy_pbar = gp_widget_by_uid(uids, "energy_pbar", GP_WIDGET_PROGRESSBAR);
	bat.time_rem = gp_widget_by_uid(uids, "time_rem", GP_WIDGET_LABEL);

	if (bat.technology)
		gp_widget_label_set(bat.technology, ps->bat.technology);

	if (bat.cycle_count)
		gp_widget_label_printf(bat.cycle_count, "%u", ps->bat.cycle_count);

	gp_widget *wear_pbar = gp_widget_by_uid(uids, "wear_pbar", GP_WIDGET_PROGRESSBAR);

	if (wear_pbar) {
		gp_widget_pbar_set_max(wear_pbar, ps->bat.energy_design);
		gp_widget_pbar_set(wear_pbar, ps->bat.energy_full);
	}

	update();

	gp_htable_free(uids);

	gp_widgets_timer_ins(&refresh_tmr);

	gp_widgets_main_loop(layout, "gpbatinfo", NULL, argc, argv);

	return 0;
}
