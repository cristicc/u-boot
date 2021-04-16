// SPDX-License-Identifier: GPL-2.0+
/*
 * Actions Semi Owl SoC timer driver
 *
 * Copyright (C) 2012 Actions Semi Inc.
 * Copyright (C) 2021 Cristian Ciocaltea <cristian.ciocaltea@gmail.com>
 */

#include <clk.h>
#include <common.h>
#include <dm.h>
#include <timer.h>
#include <asm/io.h>
#include <dm/device_compat.h>
#include <linux/bitops.h>
//TODO: optimize includes

/* T{0,1}_CTL register */
#define Tn_CTL_EN		BIT(2)
#define Tn_CTL_IRQEN		BIT(1)
#define Tn_CTL_PD		BIT(0)

struct owl_timer_regs {
	u32 twohz0_ctl;		/* 2Hz0 Control register */
	u32 reserved;
	u32 t0_ctl;		/* Timer0 Control register */
	u32 t0_cmp;		/* Timer0 Compare register */
	u32 t0_val;		/* Timer0 Value register */
	u32 t1_ctl;		/* Timer1 Control register */
	u32 t1_cmp;		/* Timer1 Compare register */
	u32 t1_val;		/* Timer1 Value register */
	u32 twohz1_ctl;		/* 2Hz1 Control register */
};

struct owl_timer_priv {
	struct owl_timer_regs *base;
};

static u64 owl_timer_get_count(struct udevice *dev)
{
	struct owl_timer_priv *priv = dev_get_priv(dev);
	struct owl_timer_regs *regs = priv->base;
	u32 val = readl(&regs->t0_val);

	return timer_conv_64(val);
}

static int owl_timer_probe(struct udevice *dev)
{
	struct timer_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	struct owl_timer_priv *priv = dev_get_priv(dev);
	struct owl_timer_regs *regs;
	struct clk clk;
	int ret;

	priv->base = dev_read_addr_ptr(dev);
	if (!priv->base)
		return -ENOENT;

	ret = clk_get_by_index(dev, 0, &clk);
	if (ret)
		return ret;

	ret = clk_enable(&clk);
	if (ret) {
		dev_err(dev, "failed to enable clock\n");
		return ret;
	}

	uc_priv->clock_rate = clk_get_rate(&clk);
	if (!uc_priv->clock_rate)
		return -EINVAL;

	regs = priv->base;

	/* Disable timer */
	writel(0, &regs->t0_ctl);

	/* Reset timer */
	writel(Tn_CTL_PD, &regs->t0_ctl);
	writel(0, &regs->t0_val);
	writel(0, &regs->t0_cmp);

	/* Enable timer */
	writel(Tn_CTL_EN | Tn_CTL_PD, &regs->t0_ctl);

	return 0;
}

static const struct timer_ops owl_timer_ops = {
	.get_count = owl_timer_get_count,
};

static const struct udevice_id owl_timer_ids[] = {
	{ .compatible = "actions,s500-timer" },
	{ }
};

U_BOOT_DRIVER(owl_timer) = {
	.name = "owl_timer",
	.id = UCLASS_TIMER,
	.of_match = owl_timer_ids,
	.priv_auto = sizeof(struct owl_timer_priv),
	.probe = owl_timer_probe,
	.ops = &owl_timer_ops,
	.flags = DM_FLAG_PRE_RELOC,
};
