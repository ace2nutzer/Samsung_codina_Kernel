/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Author: Bengt Jonsson <bengt.jonsson@stericsson.com> for ST-Ericsson,
 *	   Jonas Aaberg <jonas.aberg@stericsson.com> for ST-Ericsson
 *
 * License Terms: GNU General Public License v2
 *
 */

#ifndef DBX500_REGULATOR_H
#define DBX500_REGULATOR_H

#include <linux/platform_device.h>

#ifdef CONFIG_SAMSUNG_PANIC_DISPLAY_DEVICES
extern bool epod_on[NUM_EPOD_ID];
extern bool epod_ramret[NUM_EPOD_ID];
#endif
/**
 * struct dbx500_regulator_info - dbx500 regulator information
 * @dev: device pointer
 * @desc: regulator description
 * @rdev: regulator device pointer
 * @is_enabled: status of the regulator
 * @epod_id: id for EPOD (power domain)
 * @is_ramret: RAM retention switch for EPOD (power domain)
 * @operating_point: operating point (only for vape, to be removed)
 *
 */
struct dbx500_regulator_info {
	struct device *dev;
	struct regulator_desc desc;
	struct regulator_dev *rdev;
	bool is_enabled;
	u16 epod_id;
	bool is_ramret;
	bool exclude_from_power_state;
	unsigned int operating_point;
};

void power_state_active_enable(void);
int power_state_active_disable(void);


#ifdef CONFIG_REGULATOR_DEBUG
int ux500_regulator_debug_init(struct platform_device *pdev,
			       struct dbx500_regulator_info *regulator_info,
			       int num_regulators);

int ux500_regulator_debug_exit(void);
#else

static inline int ux500_regulator_debug_init(struct platform_device *pdev,
			     struct dbx500_regulator_info *regulator_info,
			     int num_regulators)
{
       return 0;
}

static inline int ux500_regulator_debug_exit(void)
{
       return 0;
}
#endif
#endif
