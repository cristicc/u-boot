// SPDX-License-Identifier: GPL-2.0+
/*
 * Common clock driver for Actions Semi SoCs.
 *
 * Copyright (C) 2015 Actions Semi Co., Ltd.
 * Copyright (C) 2018 Manivannan Sadhasivam <manivannan.sadhasivam@linaro.org>
 */

#include <common.h>
#include <dm.h>
#include "clk_owl.h"
#include <asm/io.h>
#if defined(CONFIG_MACH_S900)
#include <asm/arch-owl/regs_s900.h>
#include <dt-bindings/clock/actions,s900-cmu.h>
#elif defined(CONFIG_MACH_S700)
#include <asm/arch-owl/regs_s700.h>
#include <dt-bindings/clock/actions,s700-cmu.h>
#elif defined(CONFIG_MACH_S500)
#include <asm/arch-owl/regs_s500.h>
#include <dt-bindings/clock/actions,s500-cmu.h>
#endif
#include <linux/bitops.h>
#include <linux/delay.h>

static void owl_clk_init(struct owl_clk_priv *priv)
{
#if defined(CONFIG_MACH_S500)
	u32 mask, val;

	/* Enable DE and NOC1 system clocks */
	setbits_le32(priv->base + CMU_DEVCLKEN0, CMU_DEVCLKEN0_DE);
	setbits_le32(priv->base + CMU_DEVCLKEN1, CMU_DEVCLKEN1_NOC1);
	udelay(4 * PLL_STABILITY_WAIT_US);

	/* Configure BUS_CLK */
	mask = CMU_BUSCLK_PDBGDIV_MASK | CMU_BUSCLK_PERDIV_MASK |
	       CMU_BUSCLK_NICDIV_MASK | CMU_BUSCLK_NICSRC_MASK;
	val = CMU_BUSCLK_PDBGDIV_DIV | CMU_BUSCLK_PERDIV_DIV |
	      CMU_BUSCLK_NICDIV_DIV;
	clrsetbits_le32(priv->base + CMU_BUSCLK, mask, val);
	udelay(PLL_STABILITY_WAIT_US / 10);

	/* Configure BUS1_CLK */
	val = CMU_BUSCLK1_HCLK_DIV | CMU_BUSCLK1_AHBPREDIV_DIV |
	      CMU_BUSCLK1_APBCLK_DIV;
	writel(val, CMU_BUSCLK1);

	/* Enable divisors */
	setbits_le32(priv->base + CMU_BUSCLK, CMU_BUSCLK_DIVEN);
	udelay(PLL_STABILITY_WAIT_US / 10);

	/* Source HOSC for DEV_CLK */
	clrbits_le32(priv->base + CMU_DEVPLL, CMU_DEVPLL_CLK);
	udelay(PLL_STABILITY_WAIT_US / 10);

	/* Configure DEV_PLL */
	val = readl(priv->base + CMU_DEVPLL);
	val |= CMU_DEVPLL_EN | (CMU_DEVPLL_OUT - 10);
	writel(val, priv->base + CMU_DEVPLL);
	udelay(3 * PLL_STABILITY_WAIT_US / 2);

	/* Source DEV_PLL for DEV_CLK */
	setbits_le32(priv->base + CMU_DEVPLL, CMU_DEVPLL_CLK);
	udelay(2 * PLL_STABILITY_WAIT_US);

	/* Source HOSC for CORE_CLK */
	clrsetbits_le32(priv->base + CMU_BUSCLK,
			CMU_BUSCLK_CORESRC_MASK | CMU_BUSCLK_DIVEN,
			CMU_BUSCLK_CORESRC_HOSC);
	udelay(PLL_STABILITY_WAIT_US / 10);

	/* Configure CORE_PLL */
	val = readl(priv->base + CMU_COREPLL);
	val |= CMU_COREPLL_EN | CMU_COREPLL_HOSC_EN | (CMU_COREPLL_OUT + 4);
	writel(val, priv->base + CMU_COREPLL);
	udelay(3 * PLL_STABILITY_WAIT_US / 2);

	/* Source CORE_PLL for CORE_CLK */
	clrsetbits_le32(priv->base + CMU_BUSCLK, CMU_BUSCLK_CORESRC_MASK,
			CMU_BUSCLK_CORESRC_CPLL);
	udelay(PLL_STABILITY_WAIT_US / 10);

#else /* defined(CONFIG_MACH_S500) */
	u32 bus_clk = 0, core_pll, dev_pll;

#if defined(CONFIG_MACH_S900)
	/* Enable ASSIST_PLL */
	setbits_le32(priv->base + CMU_ASSISTPLL, BIT(0));
	udelay(PLL_STABILITY_WAIT_US);
#endif

	/* Source HOSC to DEV_CLK */
	clrbits_le32(priv->base + CMU_DEVPLL, CMU_DEVPLL_CLK);

	/* Configure BUS_CLK */
	bus_clk |= (CMU_PDBGDIV_DIV | CMU_PERDIV_DIV | CMU_NOCDIV_DIV |
			CMU_DMMCLK_SRC | CMU_APBCLK_DIV | CMU_AHBCLK_DIV |
			CMU_NOCCLK_SRC | CMU_CORECLK_HOSC);
	writel(bus_clk, priv->base + CMU_BUSCLK);

	udelay(PLL_STABILITY_WAIT_US);

	/* Configure CORE_PLL */
	core_pll = readl(priv->base + CMU_COREPLL);
	core_pll |= (CMU_COREPLL_EN | CMU_COREPLL_HOSC_EN | CMU_COREPLL_OUT);
	writel(core_pll, priv->base + CMU_COREPLL);

	udelay(PLL_STABILITY_WAIT_US);

	/* Configure DEV_PLL */
	dev_pll = readl(priv->base + CMU_DEVPLL);
	dev_pll |= (CMU_DEVPLL_EN | CMU_DEVPLL_OUT);
	writel(dev_pll, priv->base + CMU_DEVPLL);

	udelay(PLL_STABILITY_WAIT_US);

	/* Source CORE_PLL for CORE_CLK */
	clrsetbits_le32(priv->base + CMU_BUSCLK, CMU_CORECLK_MASK,
			CMU_CORECLK_CPLL);

	/* Source DEV_PLL for DEV_CLK */
	setbits_le32(priv->base + CMU_DEVPLL, CMU_DEVPLL_CLK);

	udelay(PLL_STABILITY_WAIT_US);
#endif
}

int owl_clk_enable(struct clk *clk)
{
	struct owl_clk_priv *priv = dev_get_priv(clk->dev);
	enum owl_soc model = dev_get_driver_data(clk->dev);

	switch (clk->id) {
	case CLK_UART5:
		if (model != S900)
			return -EINVAL;
		/* Source HOSC for UART5 interface */
		clrbits_le32(priv->base + CMU_UART5CLK, CMU_UARTCLK_SRC_DEVPLL);
		/* Enable UART5 interface clock */
		setbits_le32(priv->base + CMU_DEVCLKEN1, CMU_DEVCLKEN1_UART5);
		break;
	case CLK_UART3:
		if (model != S700 && model != S500)
			return -EINVAL;
		/* Source HOSC for UART3 interface */
		clrbits_le32(priv->base + CMU_UART3CLK, CMU_UARTCLK_SRC_DEVPLL);
		/* Enable UART3 interface clock */
		setbits_le32(priv->base + CMU_DEVCLKEN1, CMU_DEVCLKEN1_UART3);
		break;
	case CLK_UART2:
		if (model != S500)
			return -EINVAL;
		/* Source HOSC for UART2 interface */
		clrbits_le32(priv->base + CMU_UART2CLK, CMU_UARTCLK_SRC_DEVPLL);
		/* Enable UART2 interface clock */
		setbits_le32(priv->base + CMU_DEVCLKEN1, CMU_DEVCLKEN1_UART2);
		break;
	case CLK_RMII_REF:
	case CLK_ETHERNET:
		setbits_le32(priv->base + CMU_DEVCLKEN1, CMU_DEVCLKEN1_ETH);
		setbits_le32(priv->base + CMU_ETHERNETPLL, 5);
		break;
	case CLK_TIMER:
		if (model != S500)
			return -EINVAL;
		/* Enable timer clock */
		setbits_le32(priv->base + CMU_DEVCLKEN1, CMU_DEVCLKEN1_TIMER);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int owl_clk_disable(struct clk *clk)
{
	struct owl_clk_priv *priv = dev_get_priv(clk->dev);
	enum owl_soc model = dev_get_driver_data(clk->dev);

	switch (clk->id) {
	case CLK_UART5:
		if (model != S900)
			return -EINVAL;
		/* Disable UART5 interface clock */
		clrbits_le32(priv->base + CMU_DEVCLKEN1, CMU_DEVCLKEN1_UART5);
		break;
	case CLK_UART3:
		if (model != S700 && model != S500)
			return -EINVAL;
		/* Disable UART3 interface clock */
		clrbits_le32(priv->base + CMU_DEVCLKEN1, CMU_DEVCLKEN1_UART3);
		break;
	case CLK_UART2:
		if (model != S500)
			return -EINVAL;
		/* Disable UART2 interface clock */
		clrbits_le32(priv->base + CMU_DEVCLKEN1, CMU_DEVCLKEN1_UART2);
		break;
	case CLK_RMII_REF:
	case CLK_ETHERNET:
		clrbits_le32(priv->base + CMU_DEVCLKEN1, CMU_DEVCLKEN1_ETH);
		break;
	case CLK_TIMER:
		if (model != S500)
			return -EINVAL;
		/* Disable timer clock */
		clrbits_le32(priv->base + CMU_DEVCLKEN1, CMU_DEVCLKEN1_TIMER);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int owl_clk_probe(struct udevice *dev)
{
	struct owl_clk_priv *priv = dev_get_priv(dev);

	priv->base = dev_read_addr(dev);
	if (priv->base == FDT_ADDR_T_NONE)
		return -EINVAL;

	/* setup necessary clocks */
	owl_clk_init(priv);

	return 0;
}

static const struct clk_ops owl_clk_ops = {
	.enable = owl_clk_enable,
	.disable = owl_clk_disable,
};

static const struct udevice_id owl_clk_ids[] = {
#if defined(CONFIG_MACH_S900)
	{ .compatible = "actions,s900-cmu", .data = S900 },
#elif defined(CONFIG_MACH_S700)
	{ .compatible = "actions,s700-cmu", .data = S700 },
#elif defined(CONFIG_MACH_S500)
	{ .compatible = "actions,s500-cmu", .data = S500 },
#endif
	{ }
};

U_BOOT_DRIVER(clk_owl) = {
	.name		= "clk_owl",
	.id		= UCLASS_CLK,
	.of_match	= owl_clk_ids,
	.ops		= &owl_clk_ops,
	.priv_auto	= sizeof(struct owl_clk_priv),
	.probe		= owl_clk_probe,
};
