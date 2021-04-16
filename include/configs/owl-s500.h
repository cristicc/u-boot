/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Board configuration file for Actions Semi Owl S500 SoCs.
 *
 * Copyright (C) 2012 Actions Semi Inc.
 * Copyright (C) 2021 Cristian Ciocaltea <cristian.ciocaltea@gmail.com>
 */

#ifndef _OWL_S500_CONFIG_H_
#define _OWL_S500_CONFIG_H_

/* Some commands use this as the default load address */
#define CONFIG_SYS_LOAD_ADDR		0x08000000

/*
 * This is the initial SP which is used only briefly for relocating the u-boot
 * image to the top of SDRAM. After relocation u-boot moves the stack to the
 * proper place.
 */
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_LOAD_ADDR + 0x7fff0)

/* Bring in the Owl common configuration */
#include <configs/owl-common.h>

#endif
