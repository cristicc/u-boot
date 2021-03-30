// SPDX-License-Identifier: GPL-2.0+
/*
 * Actions Semi Owl SoCs platform support.
 *
 * Copyright (C) 2018 Manivannan Sadhasivam <manivannan.sadhasivam@linaro.org>
 */

#include <cpu_func.h>
#include <init.h>
#include <asm/cache.h>
#include <asm/global_data.h>
#include <linux/arm-smccc.h>
#include <linux/psci.h>
#include <common.h>
#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/psci.h>

#define DMM_INTERLEAVE_PER_CH_CFG	0xe0290028
#define OWL_S500_AFI_ADDR		0xb4060300

DECLARE_GLOBAL_DATA_PTR;

/* Owl S500 Actions Firmware Infomation (AFI). */
struct owl_s500_afi
{
	char magic[4];			/* 'A','F','I' */
	unsigned char boot_mode;
	unsigned char boot_dev;
	unsigned char uart_index;	/* >7: disabled */
	unsigned char uart_pad;
	unsigned int uart_baudrate;	/* 0x9600: 38400 */
	unsigned int ddr_size;
};

unsigned int owl_s500_get_ddrcap(void)
{
	struct owl_s500_afi *afi = (struct owl_s500_afi *)OWL_S500_AFI_ADDR;
	if (!memcmp(afi->magic, "AFI", 3))
		return afi->ddr_size;

	return 512;
}

/*
 * For S500 SoC variant, DDR capacity is available via AFI.
 *
 * For S700 and S900 variants, DDR capacity is available from
 * a register initialized by DDR driver in early bootloader.
 */
static unsigned int owl_get_ddrcap(void)
{
	unsigned int val, cap;

#if defined(CONFIG_MACH_S500)
	cap = owl_s500_get_ddrcap();
#elif defined(CONFIG_MACH_S700)
	val = (readl(DMM_INTERLEAVE_PER_CH_CFG) >> 8) & 0x7;
	cap =  (val + 1) * 256;
#elif defined(CONFIG_MACH_S900)
	val = (readl(DMM_INTERLEAVE_PER_CH_CFG) >> 8) & 0xf;
	cap =  64 * (1 << val);
#endif

	return cap;
}

/*
 * dram_init - sets uboots idea of sdram size
 */
int dram_init(void)
{
	gd->ram_size = owl_get_ddrcap() * 1024 * 1024;
	return 0;
}

/* This is called after dram_init() so use get_ram_size result */
int dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = gd->ram_size;

	return 0;
}

static void show_psci_version(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(ARM_PSCI_0_2_FN_PSCI_VERSION, 0, 0, 0, 0, 0, 0, 0, &res);

	printf("PSCI:  v%ld.%ld\n",
		PSCI_VERSION_MAJOR(res.a0),
		PSCI_VERSION_MINOR(res.a0));
}

int board_init(void)
{
	show_psci_version();

	return 0;
}

void reset_cpu(ulong addr)
{
	psci_system_reset();
}
