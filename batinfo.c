//SPDX-License-Identifier: GPL-2.0-or-later

/*

    Copyright (C) 2021 Cyril Hrubis <metan@ucw.cz>

 */

#include "power_supply.h"

int main(void)
{
	struct ps *ps = ps_init(PS_BATTERY | PS_MAINS);

	ps_print(ps);

	ps_exit(ps);

	return 0;
}
