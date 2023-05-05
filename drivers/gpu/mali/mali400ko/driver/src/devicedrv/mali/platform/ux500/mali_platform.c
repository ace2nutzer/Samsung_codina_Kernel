/*
 * Copyright (C) ST-Ericsson SA 2011
 *
 * Mali related Ux500 platform initialization
 *
 * Author: Marta Lofstedt <marta.lofstedt@stericsson.com> for ST-Ericsson.
 * Author: Huang Ji (cocafe@xda) <cocafehj@gmail.com>
 * Author: Silvestro Sanfilippo (ace2nutzer@xda) <ace2nutzer@gmail.com>
 * 
 * License terms: GNU General Public License (GPL) version 2.
 */

/**
 * @file mali_platform.c
 * Platform specific Mali driver functions for ST-Ericsson's Ux500 platforms
 */
#include "mali_kernel_common.h"
#include "mali_osk.h"
#include "mali_platform.h"

#include <linux/regulator/consumer.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#include <linux/moduleparam.h>
#include <linux/kobject.h>
#include <linux/kconfig.h>

#if CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
#include <mach/prcmu.h>
#else
#include <linux/mfd/dbx500-prcmu.h>
#endif

#include <linux/delay.h>

#if IS_ENABLED(CONFIG_A2N)
#include <linux/a2n.h>
#endif

#define MIN_FREQ			0
#define MAX_FREQ			2

#define UP_THRESHOLD			95	/* max 100 % */
#define MIN_UP_THRESHOLD		40
#define DOWN_THRESHOLD_MARGIN		25
#define DEF_BOOST			0

#define DEF_FREQUENCY_STEP_300000	0
#define DEF_FREQUENCY_STEP_400000	1
#define DEF_FREQUENCY_STEP_500000	2
#define DEF_FREQUENCY_STEP_600000	3
#define DEF_FREQUENCY_STEP_650000	4
#define DEF_FREQUENCY_STEP_700000	5
#define DEF_FREQUENCY_STEP_750000	6

#define MALI_UX500_VERSION		"2.3"

#define MIN_SAMPLING_TICKS		10
#define SAMPLING_RATE_MS		40

#define SAMPLING_DOWN_FACTOR		50

#define MALI_MAX_UTILIZATION		256

#define PRCMU_SGACLK			0x0014
#define PRCMU_PLLSOC0			0x0080

#define PRCMU_SGACLK_INIT		0x00000121
#define PRCMU_PLLSOC0_INIT		0x00050141

#define AB8500_VAPE_SEL1 		0x0E
#define AB8500_VAPE_SEL2	 	0x0F
#define AB8500_VAPE_STEP_UV		12500
#define AB8500_VAPE_MIN_UV		700000
#define AB8500_VAPE_MAX_UV		1487500

struct mali_dvfs_data
{
	u32	freq;
	u32	freq_raw;
	u32	clkpll;
	u8	vape_raw;
};

static struct mali_dvfs_data mali_dvfs[] = {
	{300000, 299520, 0x00050127, 0x20},
	{400000, 399360, 0x00050134, 0x22},
	{500000, 499200, 0x00050141, 0x28},
	{600000, 599040, 0x0005014E, 0x30},
	{650000, 652800, 0x00050155, 0x3f},
	{700000, 698880, 0x0005015B, 0x3f},
	{750000, 752640, 0x00050162, 0x3f},
};

u32 mali_utilization_sampling_rate = 0;
u32 mali_sampling_rate_ratio = 1;
static u32 mali_sampling_rate_min = 0;
static bool is_running = false;
static bool is_initialized = false;
static u32 mali_last_utilization = 0;

static struct regulator *regulator;
static struct clk *clk_sga;
static struct work_struct mali_utilization_work;
static struct workqueue_struct *mali_utilization_workqueue;

#if CONFIG_HAS_WAKELOCK
static struct wake_lock wakelock;
#endif

static u32 min_freq = MIN_FREQ;
static u32 max_freq = MAX_FREQ;
static u32 last_freq = MIN_FREQ;
static u32 up_threshold = UP_THRESHOLD * 256 / 100;
static u32 down_threshold = 0;
static u32 down_threshold_margin = DOWN_THRESHOLD_MARGIN * 256 / 100;
static bool boost = DEF_BOOST;

static int vape_voltage(u8 raw)
{
	if (raw <= 0x3e) {
		return (AB8500_VAPE_MIN_UV + (raw * AB8500_VAPE_STEP_UV));
	} else {
		return AB8500_VAPE_MAX_UV;
	}
}

static int pllsoc0_freq(u32 raw)
{
	int multiple = raw & 0x000000FF;
	int divider = (raw & 0x00FF0000) >> 16;
	int half = (raw & 0x01000000) >> 24;
	int pll;

	pll = (multiple * 38400);
	pll /= divider;

	if (half) {
		pll /= 2;
	}

	return pll;
}

static u32 sgaclk_freq(void)
{
	u32 soc0pll = prcmu_read(PRCMU_PLLSOC0);
	u32 sgaclk = prcmu_read(PRCMU_SGACLK);

	if (!(sgaclk & BIT(5)))
		return 0;

	if (!(sgaclk & 0xf))
		return 0;
	
	return (pllsoc0_freq(soc0pll) / (sgaclk & 0xf));
}

static void mali_min_freq_apply(u32 freq)
{
	u8 vape;
	u32 pll;

	vape = mali_dvfs[freq].vape_raw;
	pll = mali_dvfs[freq].clkpll;

	prcmu_write(PRCMU_PLLSOC0, pll);

	udelay(20);

	prcmu_abb_write(AB8500_REGU_CTRL2, 
			AB8500_VAPE_SEL1, 
			&vape, 
			1);

	last_freq = freq;
}

static void mali_max_freq_apply(u32 freq)
{
	u8 vape;
	u32 pll;

	vape = mali_dvfs[freq].vape_raw;
	pll = mali_dvfs[freq].clkpll;

	prcmu_abb_write(AB8500_REGU_CTRL2, 
			AB8500_VAPE_SEL1, 
			&vape, 
			1);

	udelay(80);

	prcmu_write(PRCMU_PLLSOC0, pll);

	last_freq = freq;
}

static void mali_boost_init(void)
{
	if (sgaclk_freq() != mali_dvfs[MIN_FREQ].freq_raw)
		mali_min_freq_apply(MIN_FREQ);

	pr_info("[Mali] Dynamic governor min %u kHz - max %u kHz\n", 
			mali_dvfs[MIN_FREQ].freq, 
			mali_dvfs[MAX_FREQ].freq);
}

static _mali_osk_errcode_t mali_platform_powerdown(void)
{
	int ret = 0;

	if (is_running) {
		mali_min_freq_apply(MIN_FREQ);
#if CONFIG_HAS_WAKELOCK
		wake_unlock(&wakelock);
#endif
		clk_disable(clk_sga);
		if (regulator) {
			ret = regulator_disable(regulator);
			if (ret < 0) {
				MALI_DEBUG_PRINT(2, ("%s: Failed to disable regulator %s\n", __func__, "v-mali"));
				is_running = false;
				MALI_ERROR(_MALI_OSK_ERR_FAULT);
			}
		}
		is_running = false;
	}
	MALI_DEBUG_PRINT(4, ("mali_platform_powerdown is_running: %u\n", is_running));
	MALI_SUCCESS;
}

static _mali_osk_errcode_t mali_platform_powerup(void)
{
	int ret = 0;

	if (!is_running) {
		ret = regulator_enable(regulator);
		if (ret < 0) {
			MALI_DEBUG_PRINT(2, ("%s: Failed to enable regulator %s\n", __func__, "v-mali"));
			goto error;
		}

		ret = clk_enable(clk_sga);
		if (ret < 0) {
			regulator_disable(regulator);
			MALI_DEBUG_PRINT(2, ("%s: Failed to enable clock %s\n", __func__, "mali"));
			goto error;
		}

#if CONFIG_HAS_WAKELOCK
		wake_lock(&wakelock);
#endif
		is_running = true;
	}
	MALI_DEBUG_PRINT(4, ("mali_platform_powerup is_running:%u\n", is_running));
	MALI_SUCCESS;
error:
	MALI_DEBUG_PRINT(1, ("Failed to power up.\n"));
	MALI_ERROR(_MALI_OSK_ERR_FAULT);
}

void mali_utilization_function(struct work_struct *ptr)
{
	u32 new_freq = 0;

	/* nothing to do */
	if ((min_freq == max_freq) && (last_freq == max_freq))
		return;

	if (mali_last_utilization >= up_threshold) {

		/* if we are already at full speed then break out early */
		if (last_freq == max_freq)
			return;

		if (!boost) {
			if (last_freq == DEF_FREQUENCY_STEP_300000)
				new_freq = DEF_FREQUENCY_STEP_400000;
			else if (last_freq == DEF_FREQUENCY_STEP_400000)
				new_freq = DEF_FREQUENCY_STEP_500000;
			else if (last_freq == DEF_FREQUENCY_STEP_500000)
				new_freq = DEF_FREQUENCY_STEP_600000;
			else if (last_freq == DEF_FREQUENCY_STEP_600000)
				new_freq = DEF_FREQUENCY_STEP_650000;
			else if (last_freq == DEF_FREQUENCY_STEP_650000)
				new_freq = DEF_FREQUENCY_STEP_700000;
			else
				new_freq = max_freq;

			if (new_freq > max_freq)
				new_freq = max_freq;
		} else {
			/* Boost */
			new_freq = max_freq;
		}
		/* If switching to max speed, apply sampling_rate_ratio */
		if (new_freq == max_freq)
			mali_sampling_rate_ratio = SAMPLING_DOWN_FACTOR;

		mali_max_freq_apply(new_freq);

		return;
	}

	/* No longer fully busy, reset sampling_rate_ratio */
	mali_sampling_rate_ratio = 1;

	/* if we cannot reduce the frequency anymore, break out early */
	if (last_freq == min_freq)
		return;

	/* Check for frequency decrease */
	if (mali_last_utilization < down_threshold) {

		if (last_freq == DEF_FREQUENCY_STEP_750000)
			new_freq = DEF_FREQUENCY_STEP_700000;
		else if (last_freq == DEF_FREQUENCY_STEP_700000)
			new_freq = DEF_FREQUENCY_STEP_650000;
		else if (last_freq == DEF_FREQUENCY_STEP_650000)
			new_freq = DEF_FREQUENCY_STEP_600000;
		else if (last_freq == DEF_FREQUENCY_STEP_600000)
			new_freq = DEF_FREQUENCY_STEP_500000;
		else if (last_freq == DEF_FREQUENCY_STEP_500000)
			new_freq = DEF_FREQUENCY_STEP_400000;
		else
			new_freq = min_freq;

		if (new_freq < min_freq)
			new_freq = min_freq;

		mali_min_freq_apply(new_freq);
	}
}

static void update_down_threshold(void)
{
	down_threshold = ((up_threshold * mali_dvfs[DEF_FREQUENCY_STEP_300000].freq / mali_dvfs[DEF_FREQUENCY_STEP_400000].freq) - down_threshold_margin);
	pr_info("[%s] for GPU: new value: %u\n",__func__, (down_threshold * 100 / 256));
}

#define ATTR_RO(_name)	\
	static struct kobj_attribute _name##_interface = __ATTR(_name, 0444, _name##_show, NULL);

#define ATTR_WO(_name)	\
	static struct kobj_attribute _name##_interface = __ATTR(_name, 0220, NULL, _name##_store);

#define ATTR_RW(_name)	\
	static struct kobj_attribute _name##_interface = __ATTR(_name, 0644, _name##_show, _name##_store);

static ssize_t version_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "DB8500 GPU OC Driver (%s), cocafe, ace2nutzer\n", MALI_UX500_VERSION);
}
ATTR_RO(version);

static ssize_t cur_freq_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u kHz\n", sgaclk_freq());
}
ATTR_RO(cur_freq);

static ssize_t vape_50_opp_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	u8 value;

	prcmu_abb_read(AB8500_REGU_CTRL2,
			AB8500_VAPE_SEL2,
			&value,
			1);

	return sprintf(buf, "%u uV - 0x%x\n", vape_voltage(value), value);
}

static ssize_t vape_50_opp_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int val;
	u8 vape;

	if (sscanf(buf, "%x", &val)) {
		vape = val;
		prcmu_abb_write(AB8500_REGU_CTRL2,
			AB8500_VAPE_SEL2,
			&vape,
			1);
		return count;
	}

	return -EINVAL;
}
ATTR_RW(vape_50_opp);

static ssize_t fullspeed_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	/*
	 * Check APE OPP status, on OPP50, clock is half.
	 */
	return sprintf(buf, "%s\n", (prcmu_get_ape_opp() == APE_100_OPP) ? "1" : "0");
}
ATTR_RO(fullspeed);

static ssize_t cur_load_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u %%\n", mali_last_utilization * 100 / 256);
}
ATTR_RO(cur_load);

static ssize_t vape_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	u8 value;
	bool opp50;

	/*
	 * cocafe:
	 * Display Vape Seletion 1 only, 
	 * In APE 50OPP, Vape uses SEL2. 
	 * And the clocks are half.
	 */
	opp50 = (prcmu_get_ape_opp() != APE_100_OPP);
	prcmu_abb_read(AB8500_REGU_CTRL2, 
			opp50 ? AB8500_VAPE_SEL2 : AB8500_VAPE_SEL1,
			&value, 
			1);

	return sprintf(buf, "%u uV - 0x%x (OPP:%d)\n", vape_voltage(value), value, opp50 ? 50 : 100);
}
ATTR_RO(vape);

static ssize_t min_freq_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	sprintf(buf, "%sDVFS idx: %u\n", buf, min_freq);
	sprintf(buf, "%sFrequency: %u kHz\n", buf, mali_dvfs[min_freq].freq);
	sprintf(buf, "%sRaw Frequency: %u kHz\n", buf, mali_dvfs[min_freq].freq_raw);
	sprintf(buf, "%sVape: %u uV\n", buf, vape_voltage(mali_dvfs[min_freq].vape_raw));

	return strlen(buf);
}

static ssize_t min_freq_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	u32 val, i;

#if IS_ENABLED(CONFIG_A2N)
	if (!a2n_allow) {
		pr_err("[%s] a2n: unprivileged access !\n",__func__);
		goto err;
	}
#endif

	if (sscanf(buf, "idx=%u", &val)) {
		if (val >= ARRAY_SIZE(mali_dvfs))
			goto err;
		min_freq = val;
		goto out;
	}

	if (sscanf(buf, "%u", &val)) {
		for (i = 0; i < ARRAY_SIZE(mali_dvfs); i++) {
			if (mali_dvfs[i].freq == val) {
				min_freq = i;
				break;
			}
		}
		goto out;
	}

	pr_err("[%s] invalid cmd\n",__func__);
	return -EINVAL;

err:
	pr_err("[%s] invalid cmd\n",__func__);
	return -EINVAL;

out:
	pr_info("[Mali] new min freqs is %u kHz\n", 
			mali_dvfs[min_freq].freq);

	return count;
}
ATTR_RW(min_freq);

static ssize_t max_freq_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	sprintf(buf, "%sDVFS idx: %u\n", buf, max_freq);
	sprintf(buf, "%sFrequency: %u kHz\n", buf, mali_dvfs[max_freq].freq);
	sprintf(buf, "%sRaw Frequency: %u kHz\n", buf, mali_dvfs[max_freq].freq_raw);
	sprintf(buf, "%sVape: %u uV\n", buf, vape_voltage(mali_dvfs[max_freq].vape_raw));

	return strlen(buf);
}

static ssize_t max_freq_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	u32 val, i;

#if IS_ENABLED(CONFIG_A2N)
	if (!a2n_allow) {
		pr_err("[%s] a2n: unprivileged access !\n",__func__);
		goto err;
	}
#endif

	if (sscanf(buf, "idx=%u", &val)) {
		if (val >= ARRAY_SIZE(mali_dvfs))
			goto err;
		max_freq = val;
		goto out;
	}

	if (sscanf(buf, "%u", &val)) {
		for (i = 0; i < ARRAY_SIZE(mali_dvfs); i++) {
			if (mali_dvfs[i].freq == val) {
				max_freq = i;
				break;
			}
		}
		goto out;
	}

err:
	pr_err("[%s] invalid cmd\n",__func__);
	return -EINVAL;

out:
	pr_info("[Mali] new max freqs is %u kHz\n", 
			mali_dvfs[max_freq].freq);

	return count;
}
ATTR_RW(max_freq);

static ssize_t up_threshold_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u %%\n", up_threshold * 100 / 256);
}

static ssize_t up_threshold_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	u32 val;

#if IS_ENABLED(CONFIG_A2N)
	if (!a2n_allow) {
		pr_err("[%s] a2n: unprivileged access !\n",__func__);
		goto err;
	}
#endif

	if (sscanf(buf, "%u", &val)) {
		if (val > 100 || val < MIN_UP_THRESHOLD)
			goto err;
		up_threshold = val * 256 / 100;
		/* update down_threshold */
		update_down_threshold();
		goto out;
	}

err:
	pr_err("[%s] invalid cmd\n",__func__);
	return -EINVAL;

out:
	return count;
}
ATTR_RW(up_threshold);

static ssize_t sampling_rate_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	sprintf(buf, "%s min_sampling_rate: %u ms\n", buf, mali_sampling_rate_min);
	sprintf(buf, "%s sampling_rate: %u ms\n", buf, mali_utilization_sampling_rate);
	return strlen(buf);
}

static ssize_t sampling_rate_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	u32 val;

#if IS_ENABLED(CONFIG_A2N)
	if (!a2n_allow) {
		pr_err("[%s] a2n: unprivileged access !\n",__func__);
		return -EINVAL;
	}
#endif

	if (sscanf(buf, "%u", &val)) {
		if (val < mali_sampling_rate_min)
			val = mali_sampling_rate_min;

		mali_utilization_sampling_rate = val;
		return count;
	}

	return -EINVAL;
}
ATTR_RW(sampling_rate);

static ssize_t dvfs_config_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	u32 i;

	sprintf(buf, "idx freq   freq_raw  clkpll     Vape\n");

	for (i = 0; i < ARRAY_SIZE(mali_dvfs); i++) {
		sprintf(buf, "%s%3u%7u%7u  %#010x%8u %#04x\n", 
			buf, 
			i, 
			mali_dvfs[i].freq, 
			pllsoc0_freq(mali_dvfs[i].clkpll), 
			mali_dvfs[i].clkpll, 
			vape_voltage(mali_dvfs[i].vape_raw), 
			mali_dvfs[i].vape_raw);
	}

	return strlen(buf);
}

static ssize_t dvfs_config_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int idx, val;

#if IS_ENABLED(CONFIG_A2N)
	if (!a2n_allow) {
		pr_err("[%s] a2n: unprivileged access !\n",__func__);
		goto err;
	}
#endif

	if (sscanf(buf, "%u pll=%x", &idx, &val) == 2) {
		mali_dvfs[idx].clkpll = val;
		goto out;
	}

	if (sscanf(buf, "%u vape=%x", &idx, &val) == 2) {
		mali_dvfs[idx].vape_raw = val;
		goto out;
	}

err:
	pr_err("[%s] invalid cmd\n",__func__);
	return -EINVAL;

out:
	return count;;
}
ATTR_RW(dvfs_config);

static ssize_t available_frequencies_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(mali_dvfs); i++) {
		sprintf(buf, "%s%u\n", buf, mali_dvfs[i].freq);
	}

	return strlen(buf);
}
ATTR_RO(available_frequencies);

static ssize_t boost_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", boost);
}

static ssize_t boost_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	u32 val;

#if IS_ENABLED(CONFIG_A2N)
	if (!a2n_allow) {
		pr_err("[%s] a2n: unprivileged access !\n",__func__);
		goto err;
	}
#endif

	if (sscanf(buf, "%u", &val)) {
		if (val > 1 || val < 0)
			goto err;
		boost = val;
		goto out;
	}

err:
	pr_err("[%s] invalid cmd\n",__func__);
	return -EINVAL;

out:
	return count;
}
ATTR_RW(boost);

static struct attribute *mali_attrs[] = {
	&version_interface.attr, 
	&cur_freq_interface.attr, 
	&fullspeed_interface.attr, 
	&cur_load_interface.attr, 
	&vape_interface.attr, 
	&vape_50_opp_interface.attr,
	&min_freq_interface.attr, 
	&max_freq_interface.attr, 
	&dvfs_config_interface.attr, 
	&available_frequencies_interface.attr, 
	&up_threshold_interface.attr, 
	&sampling_rate_interface.attr, 
	&boost_interface.attr, 
	NULL,
};

static struct attribute_group mali_interface_group = {
	 /* .name  = "governor", */ /* Not using subfolder now */
	.attrs = mali_attrs,
};

static struct kobject *mali_kobject;

_mali_osk_errcode_t mali_platform_init()
{
	int ret;

	is_running = false;
	mali_sampling_rate_min = jiffies_to_msecs(MIN_SAMPLING_TICKS);
	mali_utilization_sampling_rate = SAMPLING_RATE_MS;

	if (!is_initialized) {
		update_down_threshold();
		prcmu_write(PRCMU_PLLSOC0, PRCMU_PLLSOC0_INIT);
		prcmu_write(PRCMU_SGACLK,  PRCMU_SGACLK_INIT);
		mali_boost_init();

		mali_utilization_workqueue = create_singlethread_workqueue("mali_utilization_workqueue");
		if (NULL == mali_utilization_workqueue) {
			MALI_DEBUG_PRINT(2, ("%s: Failed to setup workqueue %s\n", __func__, "mali_utilization_workqueue"));
			goto error;
		}

		INIT_WORK(&mali_utilization_work, mali_utilization_function);

		regulator = regulator_get(NULL, "v-mali");
		if (IS_ERR(regulator)) {
			MALI_DEBUG_PRINT(2, ("%s: Failed to get regulator %s\n", __func__, "v-mali"));
			goto error;
		}

		clk_sga = clk_get_sys("mali", NULL);
		if (IS_ERR(clk_sga)) {
			regulator_put(regulator);
			MALI_DEBUG_PRINT(2, ("%s: Failed to get clock %s\n", __func__, "mali"));
			goto error;
		}

#if CONFIG_HAS_WAKELOCK
		wake_lock_init(&wakelock, WAKE_LOCK_SUSPEND, "mali_wakelock");
#endif

		mali_kobject = kobject_create_and_add("mali", kernel_kobj);
		if (!mali_kobject) {
			pr_err("[Mali] Failed to create kobject interface\n");
		}

		ret = sysfs_create_group(mali_kobject, &mali_interface_group);
		if (ret) {
			kobject_put(mali_kobject);
		}

		pr_info("[Mali] DB8500 GPU OC Initialized (%s)\n", MALI_UX500_VERSION);

		is_initialized = true;
	}

	MALI_SUCCESS;
error:
	MALI_DEBUG_PRINT(1, ("SGA initialization failed.\n"));
	MALI_ERROR(_MALI_OSK_ERR_FAULT);
}

_mali_osk_errcode_t mali_platform_deinit()
{
	destroy_workqueue(mali_utilization_workqueue);
	regulator_put(regulator);
	clk_put(clk_sga);

#if CONFIG_HAS_WAKELOCK
	wake_lock_destroy(&wakelock);
#endif
	kobject_put(mali_kobject);
	is_running = false;
	mali_last_utilization = 0;
	is_initialized = false;
	MALI_DEBUG_PRINT(2, ("SGA terminated.\n"));
	MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_power_mode_change(mali_power_mode power_mode)
{
	if (MALI_POWER_MODE_ON == power_mode)
		return mali_platform_powerup();

	/*We currently don't make any distinction between MALI_POWER_MODE_LIGHT_SLEEP and MALI_POWER_MODE_DEEP_SLEEP*/
	return mali_platform_powerdown();
}

void mali_gpu_utilization_handler(u32 utilization)
{
	mali_last_utilization = utilization;
	/*
	* We should not cancel the potentially not yet run old work
	* in favor of a new work.
	* Since the utilization value will change,
	* the mali_utilization_function will evaluate based on
	* what is the utilization now and not on what it was
	* when it was scheduled.
	*/
	queue_work(mali_utilization_workqueue, &mali_utilization_work);
}

void set_mali_parent_power_domain(void *dev)
{
	MALI_DEBUG_PRINT(2, ("This function should not be called since we are not using run time pm\n"));
}
