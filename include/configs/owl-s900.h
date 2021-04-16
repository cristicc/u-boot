/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Board configuration file for Actions Semi Owl S700 & S900 SoCs.
 *
 * Copyright (C) 2015 Actions Semi Co., Ltd.
 * Copyright (C) 2018 Manivannan Sadhasivam <manivannan.sadhasivam@linaro.org>
 *
 */

#ifndef _OWL_S900_CONFIG_H_
#define _OWL_S900_CONFIG_H_

/* Some commands use this as the default load address */
#define CONFIG_SYS_LOAD_ADDR		(CONFIG_SYS_SDRAM_BASE + 0x7ffc0)

/*
 * This is the initial SP which is used only briefly for relocating the u-boot
 * image to the top of SDRAM. After relocation u-boot moves the stack to the
 * proper place.
 */
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_TEXT_BASE + 0x7ff00)

/* Bring in the Owl common config */
#include <configs/owl-common.h>

#endif
