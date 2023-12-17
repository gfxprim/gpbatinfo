//SPDX-License-Identifier: GPL-2.1-or-later

/*

    Copyright (C) 2022 Cyril Hrubis <metan@ucw.cz>

 */

#include <gfxprim.h>
#include <sysinfo/power_supply.h>

static struct ps *ps;

struct bat {
	gp_widget *status;
	gp_widget *voltage_now;
	gp_widget *voltage_avg;
	gp_widget *current_avg;
	gp_widget *current_now;
	gp_widget *power_avg;
	gp_widget *power_now;
	gp_widget *technology;
	gp_widget *cycle_count;

	gp_widget *state_now;
	gp_widget *state_full;
	gp_widget *state_design;
	gp_widget *energy_pbar;
	gp_widget *time_rem;
} bat;

gp_app_info app_info = {
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
	const char *unit = ps_bat_state_unit(ps);

	if (bat.status)
		gp_widget_label_set(bat.status, ps_bat_status_name(ps->bat.status));

	if (bat.voltage_now)
		gp_widget_label_printf(bat.voltage_now, "%u.%03u V", ps->bat.voltage_now/1000000, (ps->bat.voltage_now%1000000)/1000);

	if (bat.voltage_avg)
		gp_widget_label_printf(bat.voltage_avg, "%u.%03u V", ps->bat.voltage_avg/1000000, (ps->bat.voltage_avg%1000000)/1000);

	if (bat.current_avg) {
		uint32_t current_avg = ps_bat_current_avg(ps);
		gp_widget_label_printf(bat.current_avg, "%u.%03u A", current_avg/1000000, (current_avg%1000000)/1000);
	}

	if (bat.current_now) {
		uint32_t current_now = ps_bat_current_now(ps);
		gp_widget_label_printf(bat.current_now, "%u.%03u A", current_now/1000000, (current_now%1000000)/1000);
	}

	if (bat.state_now)
		gp_widget_label_printf(bat.state_now, "%u m%s", ps->bat.state_now/1000, unit);

	if (bat.state_full)
		gp_widget_label_printf(bat.state_full, "%u m%s", ps->bat.state_full/1000, unit);

	if (bat.state_design)
		gp_widget_label_printf(bat.state_design, "%u m%s", ps->bat.state_design/1000, unit);

	if (bat.power_now) {
		uint32_t power_now = ps_bat_power_now(ps);
		gp_widget_label_printf(bat.power_now, "%u mW", power_now/1000);
	}

	if (bat.power_avg) {
		uint32_t power_avg = ps_bat_power_avg(ps);
		gp_widget_label_printf(bat.power_avg, "%u mW", power_avg/1000);
	}

	if (bat.energy_pbar) {
		gp_widget_pbar_max_set(bat.energy_pbar, ps->bat.state_full);
		gp_widget_pbar_val_set(bat.energy_pbar, ps->bat.state_now);
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

	return self->period;
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

	gp_widgets_getopt(&argc, &argv);

	ps = ps_init(PS_BATTERY);
	if (!ps) {
		gp_dialog_msg_run(GP_DIALOG_MSG_ERR, "Error", "Battery not found!");
		return 0;
	}

	bat.status = gp_widget_by_uid(uids, "status", GP_WIDGET_LABEL);
	bat.technology = gp_widget_by_uid(uids, "technology", GP_WIDGET_LABEL);
	bat.cycle_count = gp_widget_by_uid(uids, "cycle_count", GP_WIDGET_LABEL);

	bat.state_now = gp_widget_by_uid(uids, "state_now", GP_WIDGET_LABEL);
	bat.state_full = gp_widget_by_uid(uids, "state_full", GP_WIDGET_LABEL);
	bat.state_design = gp_widget_by_uid(uids, "state_design", GP_WIDGET_LABEL);
	bat.voltage_now = gp_widget_by_uid(uids, "voltage_now", GP_WIDGET_LABEL);
	bat.voltage_avg = gp_widget_by_uid(uids, "voltage_avg", GP_WIDGET_LABEL);
	bat.current_avg = gp_widget_by_uid(uids, "current_avg", GP_WIDGET_LABEL);
	bat.current_now = gp_widget_by_uid(uids, "current_now", GP_WIDGET_LABEL);
	bat.power_now = gp_widget_by_uid(uids, "power_now", GP_WIDGET_LABEL);
	bat.power_avg = gp_widget_by_uid(uids, "power_avg", GP_WIDGET_LABEL);
	bat.energy_pbar = gp_widget_by_uid(uids, "energy_pbar", GP_WIDGET_PROGRESSBAR);
	bat.time_rem = gp_widget_by_uid(uids, "time_rem", GP_WIDGET_LABEL);

	if (bat.technology)
		gp_widget_label_set(bat.technology, ps->bat.technology);

	if (bat.cycle_count)
		gp_widget_label_printf(bat.cycle_count, "%u", ps->bat.cycle_count);

	gp_widget *wear_pbar = gp_widget_by_uid(uids, "wear_pbar", GP_WIDGET_PROGRESSBAR);

	if (wear_pbar) {
		gp_widget_pbar_max_set(wear_pbar, ps->bat.state_design);
		gp_widget_pbar_val_set(wear_pbar, ps->bat.state_full);
	}

	gp_htable_free(uids);

	gp_widgets_timer_ins(&refresh_tmr);

	gp_widgets_main_loop(layout, NULL, 0, NULL);

	return 0;
}
