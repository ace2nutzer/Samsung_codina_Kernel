/*
 * Copyright (C) 2017 Shilin Victor <chrono.monochrome@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/preempt.h>
#include <linux/mfd/dbx500-prcmu.h> // prcmu_qos_{add,update}_requirement
#include <linux/earlysuspend.h>
#include <linux/wakelock.h>
#include <asm/io.h> // readl, writel
#include <asm/irqflags.h> // local_{irq,fiq}_{enable,disable}

#include <mach/db8500-regs.h> // U8500_PRCMU_BASE

#define ATTR_RW(_name)  \
        static struct kobj_attribute _name##_interface = __ATTR(_name, 0644, _name##_show, _name##_store);

/*
 * PRCMU registers
 */

#define PRCMU_ACLK_REG			0x0004
#define PRCMU_SVACLK_REG		0x0008
#define PRCMU_SIACLK_REG		0x000c
#define PRCMU_SDMMCCLK_REG		0x0024
#define PRCMU_PER1CLK_REG		0x002c
#define PRCMU_PER2CLK_REG		0x0030
#define PRCMU_PER3CLK_REG		0x0034
#define PRCMU_PER5CLK_REG		0x0038
#define PRCMU_PER6CLK_REG		0x003c
#define PRCMU_BMLCLK_REG		0x004c
#define PRCMU_HSIRXCLK_REG		0x0054
#define PRCMU_HDMICLK_REG		0x0058
#define PRCMU_MCDECLK_REG		0x0064
#define PRCMU_DMACLK_REG		0x0074
#define PRCMU_B2R2CLK_REG		0x0078
#define PRCMU_MODECLK_REG		0x00e8
#define PRCMU_UICCCLK_REG		0x027c
#define PRCMU_PLLDDR_REG		0x008C
#define PRCMU_ARM_PLLDIVPS_REG		0x05B4
#define PRCMU_ARMFIX_REG		0x0000
#define PRCMU_APEATCLK_REG		0x005c
#define PRCMU_DSIALTCLK_REG		0x006c
#define PRCMU_SSPCLK_REG		0x0280
#define PRCMU_I2CCLK_REG		0x0020
#define PRCMU_HSITXCLK_REG		0x0050
#define PRCMU_IPICLK_REG		0x0068
#define PRCMU_CLK009_REG		0x00e4
#define PRCMU_CLK4500_REG		0x00f8
#define PRCMU_YY1CLK_REG		0x0524

/*
 * Original PRCMU registers frequency
 */
#define ORIG_SDMMCCLK			 99840
#define ORIG_PERXCLK			133120
#define ORIG_MCDECLK			159744
#define ORIG_HSIRXCLK			 99840
#define ORIG_HDMICLK			 66560
#define ORIG_SXACLK			399360

static __iomem void *prcmu_base = NULL;

static unsigned int ddr_oc_delay_ms = 1000;
module_param(ddr_oc_delay_ms, uint, 0644);

static bool ddr_oc_on_suspend = false; // set true if needed to schedule DDR OC on suspend
static u32 pending_pllddr_val; // scheduled PLLDDR value
static int pending_pllddr_freq; // scheduled PLLDDR freq
static int pllddr_oc_delay_us = 100; // delay between graduate PLLDDR changing
static bool is_suspend = false;
static bool sdmmc_is_calibrated = false,
	    perx_is_calibrated = false,
	    pllddr_is_calibrated = false;
static bool ddr_clocks_boost = false;
static struct wake_lock pllddr_oc_lock;

struct prcmu_regs_table
{
	u32 reg;
	u32 boost_value;
	u32 unboost_value;
	char *name;
};

/*
 * PRCMU registers table
 * Registers above are partly depends on PLLDDR
 */

static struct prcmu_regs_table prcmu_regs[] = {
        // PRCMU reg		| Boost val	| Unboost val	| Name
	{PRCMU_ACLK_REG,	0x184,		0x184,		"aclk"},
	{PRCMU_SVACLK_REG,	0x2,		0x2,		"svaclk"},
	{PRCMU_SIACLK_REG,	0x2,		0x2,		"siaclk"},
	{PRCMU_SDMMCCLK_REG,	0x88,		0x88,		"sdmmcclk"},
	{PRCMU_PER1CLK_REG,	0x186,		0x186,		"per1clk"},
	{PRCMU_PER2CLK_REG,	0x186,		0x186,		"per2clk"},
	{PRCMU_PER3CLK_REG,	0x186,		0x186,		"per3clk"},
	{PRCMU_PER5CLK_REG,	0x186,		0x186,		"per5clk"},
	{PRCMU_PER6CLK_REG,	0x186,		0x186,		"per6clk"},
	{PRCMU_BMLCLK_REG,	0x4,		0x4,		"bmlclk"},
	{PRCMU_HSIRXCLK_REG,	0x8,		0x8,		"hsirxclk"},
	{PRCMU_HDMICLK_REG,	0x8c,		0x8c,		"hdmiclk"},
	{PRCMU_MCDECLK_REG,	0x85,		0x85,		"mcdeclk"},
	{PRCMU_DMACLK_REG,	0x184,		0x184,		"dmaclk"},
	{PRCMU_B2R2CLK_REG,	0x4,		0x4,		"b2r2clk"},
	{PRCMU_MODECLK_REG,	0x4,		0x4,		"modeclk"},
	{PRCMU_UICCCLK_REG,	0x8,		0x8,		"uiccclk"},
	{PRCMU_ARM_PLLDIVPS_REG,0x10,		0x10,		"armplldivps"},
	{PRCMU_ARMFIX_REG,	0x741,		0x741,		"armfix"},
	{PRCMU_APEATCLK_REG,	0x4,		0x4,		"apeatclk"},
	{PRCMU_DSIALTCLK_REG,	0x1,		0x1,		"dsialtclk"},
	{PRCMU_SSPCLK_REG,	0x11,		0x11,		"sspclk"},
	{PRCMU_I2CCLK_REG,	0x11,		0x11,		"i2cclk"},
	{PRCMU_HSITXCLK_REG,	0x10,		0x10,		"hsitxclk"},
	{PRCMU_IPICLK_REG,	0x11,		0x11,		"ipiclk"},
	{PRCMU_CLK009_REG,	0x1,		0x1,		"clk009"},
	{PRCMU_CLK4500_REG,	0x1,		0x1,		"clk4500"},
	{PRCMU_YY1CLK_REG,	0x1,		0x1,		"yy1clk"},
};

enum {
      ACLK,
      SVACLK,
      SIACLK,
      SDMMCCLK,
      PER1CLK,
      PER2CLK,
      PER3CLK,
      PER5CLK,
      PER6CLK,
      BMLCLK,
      HSIRXCLK,
      HDMICLK,
      MCDECLK,
      DMACLK,
      B2R2CLK,
      MODECLK,
      UICCCLK,
      ARM_PLLDIVPS,
      ARMFIX,
      APEATCLK,
      DSIALTCLK,
      SSPCLK,
      I2CCLK,
      HSITXCLK,
      IPICLK,
      CLK009,
      CLK4500,
      YY1CLK,
} clkddr;

static void ddr_cross_clocks_boost(bool state)
{
	int i, val;
	u32 old_val, new_val;
	int new_divider, old_divider, base;

	// Set APE100/DDR100 to avoid calculating values of PRCMU registers 
	// for different OPP states

	prcmu_qos_update_requirement(PRCMU_QOS_APE_OPP,
			"PLLDDR_OC", PRCMU_QOS_MAX_VALUE);

	prcmu_qos_update_requirement(PRCMU_QOS_DDR_OPP,
			"PLLDDR_OC", PRCMU_QOS_MAX_VALUE);

	for (i = 0; i < ARRAY_SIZE(prcmu_regs); i++) {
		old_val = readl(prcmu_base + prcmu_regs[i].reg);

		new_val = state ? prcmu_regs[i].boost_value :
				prcmu_regs[i].unboost_value;

		if ((!old_val) || (old_val == new_val)) continue;

		old_divider = old_val & 0xf;
		new_divider = new_val & 0xf;

		if (!new_divider) {
			pr_err("[PLLDDR] bad divider, %s:%s:%#05x:\n", __func__, 
					    prcmu_regs[i].name,
					    new_val);
				continue;
		}

		base = old_val ^ old_divider;

		new_val = base | new_divider;

		pr_info("[PLLDDR] set %s=%#05x -> %#05x\n", prcmu_regs[i].name,
							      old_val, new_val);

		for (val = old_val;
		    (new_val > old_val) ? (val <= new_val) : (val >= new_val); 
		    (new_val > old_val) ? val++ : val--)  {
			      writel_relaxed(val, prcmu_base + prcmu_regs[i].reg);
			      udelay(200);
		}
	}

	prcmu_qos_update_requirement(PRCMU_QOS_APE_OPP,
			"PLLDDR_OC", PRCMU_QOS_DEFAULT_VALUE);

	prcmu_qos_update_requirement(PRCMU_QOS_DDR_OPP,
			"PLLDDR_OC", PRCMU_QOS_DEFAULT_VALUE);

}

#define PLLDDR_FREQ_STEPS 38400

static inline int pllarm_freq(u32 raw)
{
	int multiple = raw & 0x000000FF;
	int divider = (raw & 0x00FF0000) >> 16;
	int half = (raw & 0x01000000) >> 24;
	int pll;

	pll = (multiple * PLLDDR_FREQ_STEPS);
	pll /= divider;

	if (half) {
		pll /= 2;
	}

	return pll;
}

static u32 tmp_val; // this value is used if do_oc_ddr was aborted on PLLDDR calibration

static void do_oc_ddr(int new_val_)
{
	u32 old_val_, val;
	u8 sdmmc_val_base;
	int i;
	int mcdeclk_is_enabled = 0, sdmmcclk_is_enabled = 0;
	int pllddr_freq, pllddr_freq_old;
	int sdmmc_old_divider, sdmmc_new_divider,
	    perx_new_divider,
	    hdmi_new_divider,
	    mcde_new_divider,
	    sxa_new_divider, // used for SIACLK and SVACLK
	    hsirx_new_divider; // used also for uiccclk

	old_val_ = readl(prcmu_base + PRCMU_PLLDDR_REG);

	pllddr_freq = pllarm_freq(new_val_);

	if (!perx_is_calibrated) {

		// Recalibrate PER1CLK - PER6CLK
		perx_new_divider = (pllddr_freq - (pllddr_freq % ORIG_PERXCLK)) / ORIG_PERXCLK;
		//if (pllddr_freq % ORIG_PERXCLK) perx_new_divider++;
		if (perx_new_divider > 15) perx_new_divider = 15;

		prcmu_regs[PER1CLK].boost_value = perx_new_divider;
		prcmu_regs[PER2CLK].boost_value = perx_new_divider;
		prcmu_regs[PER3CLK].boost_value = perx_new_divider;
		prcmu_regs[PER5CLK].boost_value = perx_new_divider;
		prcmu_regs[PER6CLK].boost_value = perx_new_divider;

		// Recalibrate MCDECLK
		mcde_new_divider = (pllddr_freq - (pllddr_freq % ORIG_MCDECLK)) / ORIG_MCDECLK;
		//if (pllddr_freq % ORIG_MCDECLK) mcde_new_divider++;
		if (mcde_new_divider > 15) mcde_new_divider = 15;

		prcmu_regs[MCDECLK].boost_value = mcde_new_divider;

		// Recalibrate HSIRXCLK && UICCCLK
		hsirx_new_divider = (pllddr_freq - (pllddr_freq % ORIG_HSIRXCLK)) / ORIG_HSIRXCLK;
		if (hsirx_new_divider > 15) hsirx_new_divider = 15;

		prcmu_regs[HSIRXCLK].boost_value = hsirx_new_divider;
		prcmu_regs[UICCCLK].boost_value = hsirx_new_divider;

		// Recalibrate HDMICLK
		hdmi_new_divider = (pllddr_freq - (pllddr_freq % ORIG_HDMICLK)) / ORIG_HDMICLK;
		if (hdmi_new_divider > 15) hdmi_new_divider = 15;

		prcmu_regs[HDMICLK].boost_value = hdmi_new_divider;

		// Recalibrate SIACLK and SVACLK
		sxa_new_divider = (pllddr_freq - (pllddr_freq % ORIG_SXACLK)) / ORIG_SXACLK;
		if (pllddr_freq % ORIG_SXACLK) sxa_new_divider++;
		if (sxa_new_divider > 15) sxa_new_divider = 15;

		prcmu_regs[SIACLK].boost_value = sxa_new_divider;
		prcmu_regs[SVACLK].boost_value = sxa_new_divider;

		mcdeclk_is_enabled = readl(prcmu_base + PRCMU_MCDECLK_REG) & 0x100; 
		sdmmcclk_is_enabled = readl(prcmu_base + PRCMU_SDMMCCLK_REG) & 0x100;  
		if (sdmmcclk_is_enabled || (new_val_>=0x50180 && mcdeclk_is_enabled)) {
			pr_err("[PLLDDR] refused to OC due to enabled SDMMCCLK or MCDECLK\n");
			return;
		}

		pr_info("[PLLDDR] recalibrating some system clocks ...\n");
		ddr_cross_clocks_boost(1); // apply settings above
		perx_is_calibrated = true;
		udelay(50);
	}

	if (!sdmmc_is_calibrated) {
		sdmmc_old_divider = readl(prcmu_base + PRCMU_SDMMCCLK_REG) & 0x0f;
		sdmmc_new_divider = (pllddr_freq - (pllddr_freq % ORIG_SDMMCCLK)) / ORIG_SDMMCCLK;
		if (sdmmc_new_divider > 15) sdmmc_new_divider = 15;

		mcdeclk_is_enabled = readl(prcmu_base + PRCMU_MCDECLK_REG) & 0x100; 
		sdmmcclk_is_enabled = readl(prcmu_base + PRCMU_SDMMCCLK_REG) & 0x100; 
		if (sdmmcclk_is_enabled || (new_val_>=0x50180 && mcdeclk_is_enabled)) {
			pr_err("[PLLDDR] refused to OC due to enabled SDMMCCLK or MCDECLK\n");
			return;
		}

		if (sdmmc_new_divider && (sdmmc_old_divider != sdmmc_new_divider)) {
			pr_info("[PLLDDR] mmc_clk_div %d -> %d\n", sdmmc_old_divider, sdmmc_new_divider);
			sdmmc_val_base = readl(prcmu_base + PRCMU_SDMMCCLK_REG) ^ sdmmc_old_divider;
			writel_relaxed(sdmmc_val_base | sdmmc_new_divider,
					prcmu_base + PRCMU_SDMMCCLK_REG);
		}

		sdmmc_is_calibrated = true;
		udelay(50);
	}

	if (!pllddr_is_calibrated) {
		mcdeclk_is_enabled = readl(prcmu_base + PRCMU_MCDECLK_REG) & 0x100; 
		sdmmcclk_is_enabled = readl(prcmu_base + PRCMU_SDMMCCLK_REG) & 0x100; 
		if (sdmmcclk_is_enabled || (new_val_>=0x50180 && mcdeclk_is_enabled)) {
			pr_err("[PLLDDR] refused to OC due to enabled SDMMCCLK or MCDECLK\n");
			return;
		}

		pllddr_freq_old = pllarm_freq(old_val_);
		pr_info("[PLLDDR] changing PLLDDR %#010x -> %#010x (%d kHz -> %d kHz)\n", old_val_, new_val_, pllddr_freq_old, pllddr_freq);
		preempt_disable();
		local_irq_disable();
		local_fiq_disable();

		pr_info("[PLLDDR] (mcdeclk_is_enabled || sdmmcclk_is_enabled) = %d", (mcdeclk_is_enabled || sdmmcclk_is_enabled));
		for (i = 0; i < 20 && (mcdeclk_is_enabled || sdmmcclk_is_enabled); i++) {
			udelay(100);
			mcdeclk_is_enabled = readl(prcmu_base + PRCMU_MCDECLK_REG) & 0x100; 
			sdmmcclk_is_enabled = readl(prcmu_base + PRCMU_SDMMCCLK_REG) & 0x100;  
		}

		for (val = (tmp_val ? tmp_val : old_val_);
		    (new_val_ > old_val_) ? (val <= new_val_) : (val >= new_val_);
		    (new_val_ > old_val_) ? val++ : val--) {
			if (val == 0x50180) {
				mcdeclk_is_enabled = readl(prcmu_base + PRCMU_MCDECLK_REG) & 0x100; 
				sdmmcclk_is_enabled = readl(prcmu_base + PRCMU_SDMMCCLK_REG) & 0x100;  
				if (mcdeclk_is_enabled || sdmmcclk_is_enabled) {
					pr_err("[PLLDDR] refused to change PLLDDR due to possible reboot\n");
					tmp_val = val;
					local_fiq_enable();
					local_irq_enable();
					preempt_enable();
					return;
				}
			}
			writel_relaxed(val, prcmu_base + PRCMU_PLLDDR_REG);
			udelay(pllddr_oc_delay_us);
		}
		local_fiq_enable();
		local_irq_enable();
		preempt_enable();

		pllddr_is_calibrated = true;
		tmp_val = 0;
		udelay(50);
	}
}

static void do_oc_ddr_fn(struct work_struct *work);
static DECLARE_DELAYED_WORK(do_oc_ddr_delayedwork, do_oc_ddr_fn);
static void do_oc_ddr_fn(struct work_struct *work)
{
	if (ddr_oc_on_suspend) {
		if (!wake_lock_active(&pllddr_oc_lock))
			wake_lock(&pllddr_oc_lock);
	}

	if (!(perx_is_calibrated && sdmmc_is_calibrated && pllddr_is_calibrated)) {
		do_oc_ddr(pending_pllddr_val);
		schedule_delayed_work(&do_oc_ddr_delayedwork, msecs_to_jiffies(ddr_oc_delay_ms));
	} else {
		perx_is_calibrated = false;
		sdmmc_is_calibrated = false;
		pllddr_is_calibrated = false;
		pending_pllddr_val = 0;
		pending_pllddr_freq = 0;

		if (ddr_oc_on_suspend) {
			if (wake_lock_active(&pllddr_oc_lock))
				wake_unlock(&pllddr_oc_lock);

			ddr_oc_on_suspend = false;
		}
	}
}

static struct early_suspend early_suspend;

static void pllddr_early_suspend(struct early_suspend *h)
{
	is_suspend = true;

	if (pending_pllddr_val && ddr_oc_on_suspend) {
		schedule_delayed_work(&do_oc_ddr_delayedwork, msecs_to_jiffies(ddr_oc_delay_ms));
	}
}

static void pllddr_late_resume(struct early_suspend *h)
{
	is_suspend = false;

	if (pending_pllddr_val && ddr_oc_on_suspend) {
		cancel_delayed_work(&do_oc_ddr_delayedwork);
		if (wake_lock_active(&pllddr_oc_lock))
			wake_unlock(&pllddr_oc_lock);
	}
}

static struct kobject *pllddr_kobject;

static ssize_t pllddr_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	u32 val, count;

	val = readl(prcmu_base + PRCMU_PLLDDR_REG);

	if (unlikely(pending_pllddr_val > 0)) {
		char tmp[75];
		count = sprintf(tmp, "pending_pllddr_val: %#010x (%d kHz)\n", pending_pllddr_val,
							pending_pllddr_freq);
		count += sprintf(buf, "PLLDDR: %#010x (%d kHz)\n%s", val,  pllarm_freq(val), tmp);
	} else {
		count += sprintf(buf, "PLLDDR: %#010x (%d kHz)\n", val,  pllarm_freq(val));
	}


	return count;
}

static ssize_t pllddr_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	u32 new_val, old_val;
	int freq = 0, div, mul;
	int ret = 0;

	if (unlikely(pending_pllddr_val > 0)) {
		pr_warn("%s: PLLDDR OC is already scheduled.\n", __func__);
		return -EBUSY;
	}

	ret = sscanf(buf, "%d", &freq);

	// check for bogus values - retry with hexademical input
	if ((!freq) || (freq <= 50199)) {
		ret = sscanf(buf, "%x", &freq);

		if ((freq >= 0x50101) && (freq <= 0x501ff)) {
			new_val = freq;
			freq = pllarm_freq(freq);
			goto schedule;
		} else
			goto inval_input;
	}

	if (!ret || !freq) {
inval_input:
		pr_err("[PLLDDR] invalid input\n");
		return -EINVAL;
	}

	old_val = readl(prcmu_base + PRCMU_PLLDDR_REG);
	mul = (old_val & 0x000000FF);
	div = (old_val & 0x00FF0000) >> 16;

	new_val = 0x0050100 + (freq * 5 / 38400);

schedule:
	pending_pllddr_val = new_val;
	pending_pllddr_freq = freq;

	schedule_delayed_work(&do_oc_ddr_delayedwork, msecs_to_jiffies(ddr_oc_delay_ms));

	return count;
}
ATTR_RW(pllddr);

static ssize_t pllddr_oc_delay_us_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", pllddr_oc_delay_us);
}

static ssize_t pllddr_oc_delay_us_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret, val;
	
	ret = sscanf(buf, "%d", &val);
		
	if (!ret)
		return -EINVAL;
	
	pllddr_oc_delay_us = val;
	
	return count;
}
ATTR_RW(pllddr_oc_delay_us);

static int pllddr_cross_clk_freq(int pllddr_freq, u32 reg_raw)
{
	int reg_freq, reg_div;
	
	reg_div = reg_raw & 0xf;
	if (!reg_div) reg_div = reg_raw & 0x10;
	if (!reg_div) return 0;
	reg_freq = (pllddr_freq - (pllddr_freq % reg_div)) / reg_div;
	
	return reg_freq;
}

static ssize_t pllddr_cross_clocks_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	u32 pllddr_value, reg_value;
	int i, pllddr_freq;
	
	pllddr_value = readl(prcmu_base + PRCMU_PLLDDR_REG);
	pllddr_freq = pllarm_freq(pllddr_value);

	sprintf(buf, "Clocks boost: %s\n", ddr_clocks_boost ? "on" : "off");

	sprintf(buf, "%sPLLDDR: %d kHz\n\n", buf, pllddr_freq);
	
	sprintf(buf, "%sBoost settings (OPP100)\n", buf);
	
	for (i = 0; i < ARRAY_SIZE(prcmu_regs); i++) {
	  
		sprintf(buf, "%s%s=%#05x   (%d kHz)\n", buf, prcmu_regs[i].name, 
		        prcmu_regs[i].boost_value, pllddr_cross_clk_freq(pllddr_freq,
									 prcmu_regs[i].boost_value));
	}
	
	sprintf(buf, "%s\nCurrent clocks\n", buf);
	
	for (i = 0; i < ARRAY_SIZE(prcmu_regs); i++) {

		reg_value = readl(prcmu_base + prcmu_regs[i].reg);
		sprintf(buf, "%s%s=%#05x   (%d kHz)\n", buf, prcmu_regs[i].name, 
			reg_value,     pllddr_cross_clk_freq(pllddr_freq, reg_value));
	}

	return strlen(buf);
}

static ssize_t pllddr_cross_clocks_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int i, len;

	u32 val;

	if (!strncmp(&buf[0], "on", 2)) {
		ddr_clocks_boost = true;
		ddr_cross_clocks_boost(ddr_clocks_boost);
	}

	if (!strncmp(&buf[0], "off", 3)) {
		ddr_clocks_boost = false;
		ddr_cross_clocks_boost(ddr_clocks_boost);
	}

	for (i = 0; i < ARRAY_SIZE(prcmu_regs); i++) {
		len = strlen(prcmu_regs[i].name);

		if (!strncmp(&buf[0], prcmu_regs[i].name, len)) {
			if (!sscanf(&buf[len + 1], "%x", &val))
				  goto invalid_input;

			prcmu_regs[i].boost_value = val;

			return count;
		}
	}

	return count;

invalid_input:
	pr_err("[PLLDDR OC] invalid input in %s", __func__);
	return -EINVAL;
}
ATTR_RW(pllddr_cross_clocks);

static ssize_t prcmu_mcdeclk_reg_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int enabled;
	enabled = readl(prcmu_base + PRCMU_MCDECLK_REG) & 0x100;

	return sprintf(buf, "%d\n", !!enabled);
}

static ssize_t prcmu_mcdeclk_reg_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret, val = -1;
	u32 reg_val;

	ret = sscanf(buf, "%d", &val);
	if ((ret < 0) || (val < 0)) {
		pr_err("%s: invalid input: %d, ret=%d\n", __func__, val, ret);
		return -EINVAL;
	}

	reg_val = readl(prcmu_base + PRCMU_MCDECLK_REG) & 0xff;

	if (val) {
		reg_val = 0x100 | reg_val;
	}

	pr_info("[PLLDDR] %#010x\n", reg_val);

	writel(reg_val, prcmu_base + PRCMU_MCDECLK_REG);

	return 0;
}
ATTR_RW(prcmu_mcdeclk_reg);

static ssize_t prcmu_sdmmcclk_reg_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int enabled;
	enabled = readl(prcmu_base + PRCMU_SDMMCCLK_REG) & 0x100;

	return sprintf(buf, "%d\n", !!enabled);
}

static ssize_t prcmu_sdmmcclk_reg_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret, val = -1;
	u32 reg_val;

	ret = sscanf(buf, "%d", &val);
	if ((ret < 0) || (val < 0)) {
		pr_err("%s: invalid input: %d, ret=%d\n", __func__, val, ret);
		return -EINVAL;
	}

	reg_val = readl(prcmu_base + PRCMU_SDMMCCLK_REG) & 0xff;

	if (val) {
		reg_val = 0x100 | reg_val;
	}

	pr_info("[PLLDDR] %#010x\n", reg_val);

	writel(reg_val, prcmu_base + PRCMU_SDMMCCLK_REG);

	return 0;
}
ATTR_RW(prcmu_sdmmcclk_reg);

static struct attribute *pllddr_attrs[] = {
	&pllddr_interface.attr,
	&pllddr_oc_delay_us_interface.attr,
	&pllddr_cross_clocks_interface.attr,
	&prcmu_mcdeclk_reg_interface.attr,
	&prcmu_sdmmcclk_reg_interface.attr,
	NULL,
};

static struct attribute_group pllddr_interface_group = {
	.attrs = pllddr_attrs,
};

static int __init pllddr_oc_start(void)
{
	int ret = 0;

	pr_info("[PLLDDR] OC driver initialized");

	prcmu_base = __io_address(U8500_PRCMU_BASE);

	pllddr_kobject = kobject_create_and_add("pllddr", kernel_kobj);
	if (!pllddr_kobject) {
		pr_err("[PLLDDR OC] Failed to create kobject interface\n");
	}
	ret = sysfs_create_group(pllddr_kobject, &pllddr_interface_group);
	if (ret)
		kobject_put(pllddr_kobject);

	if (prcmu_qos_add_requirement(PRCMU_QOS_APE_OPP,
			"PLLDDR_OC", PRCMU_QOS_DEFAULT_VALUE)) {
		pr_err("pcrm_qos_add APE failed\n");
	}

	if (prcmu_qos_add_requirement(PRCMU_QOS_DDR_OPP,
			"PLLDDR_OC", PRCMU_QOS_DEFAULT_VALUE)) {
		pr_err("pcrm_qos_add DDR failed\n");
	}

	wake_lock_init(&pllddr_oc_lock, WAKE_LOCK_IDLE, "PLLDDR_OC");

	early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	early_suspend.suspend = pllddr_early_suspend;
	early_suspend.resume = pllddr_late_resume;

	register_early_suspend(&early_suspend);

	return ret;
}
static void __exit pllddr_oc_end(void)
{
	if (wake_lock_active(&pllddr_oc_lock))
		wake_unlock(&pllddr_oc_lock);

	wake_lock_destroy(&pllddr_oc_lock);
	unregister_early_suspend(&early_suspend);
	prcmu_qos_remove_requirement(PRCMU_QOS_DDR_OPP,
                        "PLLDDR_OC");
	prcmu_qos_remove_requirement(PRCMU_QOS_APE_OPP,
                        "PLLDDR_OC");

	sysfs_remove_group(pllddr_kobject, &pllddr_interface_group);
	kobject_put(pllddr_kobject);
}

module_init(pllddr_oc_start);
module_exit(pllddr_oc_end);

MODULE_AUTHOR("Shilin Victor <chrono.monochrome@gmail.com>");
MODULE_DESCRIPTION("PLLDDR overclock driver for DB8500 PRCMU");
MODULE_LICENSE("GPL v2");
