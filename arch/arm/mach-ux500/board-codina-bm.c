/*
 * Copyright (C) ST-Ericsson SA 2011
 *
 * License terms:  GNU General Public License (GPL), version 2
 *
 * U8500 board specific charger and battery initialization parameters.
 *
 * Author: Johan Palsson <johan.palsson@stericsson.com> for ST-Ericsson.
 * Author: Johan Gardsmark <johan.gardsmark@stericsson.com> for ST-Ericsson.
 *
 */

#include <linux/power_supply.h>
#include "board-sec-bm.h"

#ifdef CONFIG_AB8500_BATTERY_THERM_ON_BATCTRL
/*
 * These are the defined batteries that uses a NTC and ID resistor placed
 * inside of the battery pack.
 * Note that the res_to_temp table must be strictly sorted by falling
 * resistance values to work.
 */
static struct res_to_temp temp_tbl_A[] = {
	{-5, 53407},
	{ 0, 48594},
	{ 5, 43804},
	{10, 39188},
	{15, 34870},
	{20, 30933},
	{25, 27422},
	{30, 24347},
	{35, 21694},
	{40, 19431},
	{45, 17517},
	{50, 15908},
	{55, 14561},
	{60, 13437},
	{65, 12500},
};
static struct res_to_temp temp_tbl_B[] = {
	{-5, 165418},
	{ 0, 159024},
	{ 5, 151921},
	{10, 144300},
	{15, 136424},
	{20, 128565},
	{25, 120978},
	{30, 113875},
	{35, 107397},
	{40, 101629},
	{45,  96592},
	{50,  92253},
	{55,  88569},
	{60,  85461},
	{65,  82869},
};
static struct v_to_cap cap_tbl_A[] = {
	{4171,	100},
	{4114,	 95},
	{4009,	 83},
	{3947,	 74},
	{3907,	 67},
	{3863,	 59},
	{3830,	 56},
	{3813,	 53},
	{3791,	 46},
	{3771,	 33},
	{3754,	 25},
	{3735,	 20},
	{3717,	 17},
	{3681,	 13},
	{3664,	  8},
	{3651,	  6},
	{3635,	  5},
	{3560,	  3},
	{3408,    1},
	{3247,	  0},
};
static struct v_to_cap cap_tbl_B[] = {
	{4161,	100},
	{4124,	 98},
	{4044,	 90},
	{4003,	 85},
	{3966,	 80},
	{3933,	 75},
	{3888,	 67},
	{3849,	 60},
	{3813,	 55},
	{3787,	 47},
	{3772,	 30},
	{3751,	 25},
	{3718,	 20},
	{3681,	 16},
	{3660,	 14},
	{3589,	 10},
	{3546,	  7},
	{3495,	  4},
	{3404,	  2},
	{3250,	  0},
};
#endif

/* UNKNOWN Battery | 3.7 V | 1500 mA | 4.2 V */
static struct v_to_cap cap_tbl[] = {
	{4100, 100},
	{4098, 99},
	{4088, 95},
	{4045, 90},
	{4024, 87},
	{3955, 80},
	{3893, 70},
	{3859, 65},
	{3825, 60},
	{3799, 55},
	{3780, 50},
	{3750, 40},
	{3731, 30},
	{3714, 25},
	{3683, 20},
	{3658, 17},
	{3648, 14},
	{3640, 12},
	{3627, 10},
	{3615, 9},
	{3566, 7},
	{3539, 6},
	{3477, 4},
	{3403, 2},
	{3361, 1},
	{3320, 0},
};

/* SAMSUNG Battery | 3.8 V | 1500 mA | 4.35 V */
static struct v_to_cap cap_tbl_5ma[] = {
	{4250,	100},
	{4248,	99},
	{4245,	98},
	{4235,	95},
	{4183,	90},
	{4150,	87},
	{4116,	84},
	{4077,	80},
	{4068,	79},
	{4058,	77},
	{4026,	75},
	{3987,	72},
	{3974,	69},
	{3953,	66},
	{3933,	63},
	{3911,	60},
	{3900,	58},
	{3873,	55},
	{3842,	52},
	{3829,	50},
	{3810,	45},
	{3793,	40},
	{3783,	35},
	{3776,	30},
	{3762,	25},
	{3746,	20},
	{3739,	18},
	{3715,	15},
	{3700,	12},
	{3690,	10},
	{3680,	9},
	{3670,	7},
	{3656,	5},
	{3634,	4},
	{3614,	3},
	{3551,	2},
	{3458,	1},
	{3320,	0},
};

/* Battery voltage to Resistance table*/
static struct v_to_res res_tbl[] = {
	{4240,	160},
	{4210,	179},
	{4180,	183},
	{4160,	184},
	{4140,	191},
	{4120,	204},
	{4080,	200},
	{4027,	202},
	{3916,	221},
	{3842,	259},
	{3800,	262},
	{3715,	340},
	{3700,	300},
	{3668,	258},
	{3630,	209},
	{3588,	275},
	{3503,	341},
	{3400,	269},
	{3360,	328},
	{3330,	305},
	{3300,	339},
};

static struct v_to_res chg_res_tbl[] = {
	{4302, 200},
	{4258, 206},
	{4200, 231},
	{4150, 198},
	{4100, 205},
	{4050, 208},
	{4000, 228},
	{3950, 213},
	{3900, 225},
	{3850, 212},
	{3800, 232},
	{3750, 177},
	{3712, 164},
	{3674, 161},
	{3590, 164},
};

/*
 * Note that the res_to_temp table must be strictly sorted by falling
 * resistance values to work.
 */
static struct res_to_temp temp_tbl[] = {
	{-5, 214834},
	{ 0, 162943},
	{ 5, 124820},
	{10,  96520},
	{15,  75306},
	{20,  59254},
	{25,  47000},
	{30,  37566},
	{35,  30245},
	{40,  24520},
	{45,  20010},
	{50,  16432},
	{55,  13576},
	{60,  11280},
	{65,   9425},
};

static struct res_to_temp adc_temp_tbl[] = {
	{-10, 780},
	{-5, 660},
	{ 0, 580},
	{ 5, 483},
	{10, 416},
	{15, 348},
	{20, 282},
	{25, 233},
	{30, 195},
	{35, 160},
	{40, 131},
	{43, 122},
	{45, 111},
	{47, 100},
	{50, 93},
	{55, 77},
	{60, 66},
	{63, 60},
	{65, 55},
	{70, 48},
};

#ifdef CONFIG_AB8500_BATTERY_THERM_ON_BATCTRL
/*
 * Note that the batres_vs_temp table must be strictly sorted by falling
 * temperature values to work.
 */
static struct batres_vs_temp temp_to_batres_tbl[] = {
	{ 40, 120},
	{ 30, 135},
	{ 20, 165},
	{ 10, 230},
	{ 00, 325},
	{-10, 445},
	{-20, 595},
};
#else
/*
 * Note that the batres_vs_temp table must be strictly sorted by falling
 * temperature values to work.
 */
static struct batres_vs_temp temp_to_batres_tbl[] = {
	{ 60, 300},
	{ 30, 300},
	{ 20, 300},
	{ 10, 300},
	{ 00, 300},
	{-10, 300},
	{-20, 300},
};
#endif

static const struct battery_type bat_type[] = {
	[BATTERY_UNKNOWN] = {
		/* First element always represent the UNKNOWN battery */
		.name = POWER_SUPPLY_TECHNOLOGY_LION,
		.resis_high = 0,
		.resis_low = 0,
		.battery_resistance = 100,
#ifdef CONFIG_SAMSUNG_CHARGER_SPEC
		.line_impedance = 36,
		.battery_resistance_for_charging = 200,
#endif
		.charge_full_design = 1500,
		.nominal_voltage = 3700,
		.termination_vol = 4200,
#ifdef CONFIG_SAMSUNG_CHARGER_SPEC
		.termination_curr_1st = 155,
		.termination_curr_2nd = 150,
		.recharge_vol = 4100,
#else
		.termination_curr = 150,
#endif
		.normal_cur_lvl = 1500,
		.normal_vol_lvl = 4200,
		.maint_a_cur_lvl = 1000,
		.maint_a_vol_lvl = 4200,
		.maint_a_chg_timer_h = 60,
		.maint_b_cur_lvl = 800,
		.maint_b_vol_lvl = 4200,
		.maint_b_chg_timer_h = 200,
		.low_high_cur_lvl = 500,
		.low_high_vol_lvl = 4200,
#ifdef CONFIG_MEASURE_TEMP_BY_ADC_TABLE
		.n_temp_tbl_elements = ARRAY_SIZE(adc_temp_tbl),
		.r_to_t_tbl = adc_temp_tbl,
#else
		.n_temp_tbl_elements = ARRAY_SIZE(temp_tbl),
		.r_to_t_tbl = temp_tbl,
#endif
		.n_v_cap_tbl_elements = ARRAY_SIZE(cap_tbl),
		.v_to_cap_tbl = cap_tbl,
#ifdef CONFIG_SAMSUNG_CHARGER_SPEC
		.n_v_res_tbl_elements = ARRAY_SIZE(res_tbl),
		.v_to_res_tbl = res_tbl,
		.n_v_chg_res_tbl_elements = ARRAY_SIZE(chg_res_tbl),
		.v_to_chg_res_tbl = chg_res_tbl,
			/* specification is 25 +/- 5 seconds. 30*HZ */
		.timeout_chargeoff_time = 21*HZ,
			/* 6 hours for first charge attempt */
		.initial_timeout_time = HZ*3600*6,
			/* 1.5 hours for second and later attempts */
		.subsequent_timeout_time = HZ*60*90,
			/* After an error stop charging for a minute. */
		.error_charge_stoptime = HZ*60,
		.over_voltage_threshold =  4300,
#else
		.n_batres_tbl_elements = ARRAY_SIZE(temp_to_batres_tbl),
		.batres_tbl = temp_to_batres_tbl,
#endif
	},

/*
 * These are the batteries that doesn't have an internal NTC resistor to
 * measure its temperature. The temperature in this case is measure with a NTC
 *  placed near the battery but on the PCB.
 */


/*
	This battery entry is the real 1650/1500 mAh battery
	to be fitted to Codina identified by a 1.5K resistor
*/
	{
		.name = POWER_SUPPLY_TECHNOLOGY_LION,
		.resis_high = 7990,		/* 1500 * 1.7, +70% */
		.resis_low = 0,		/* 1500 * 0.3, -70% */
		.battery_resistance = 100,  /* mOhms,ESR:100+LineImpedance:36 */
#ifdef CONFIG_SAMSUNG_CHARGER_SPEC
		.line_impedance = 36,
		.battery_resistance_for_charging = 200,
#endif
		.charge_full_design = 1500,
		.nominal_voltage = 3800,
		.termination_vol =  4350,
#ifdef CONFIG_SAMSUNG_CHARGER_SPEC
		.termination_curr_1st = 155,
		.termination_curr_2nd = 150,
		.recharge_vol = 4250,
#else
		.termination_curr = 150,
#endif
		.normal_cur_lvl = 1500,
		.normal_vol_lvl = 4350,
		.maint_a_cur_lvl = 1000,
		.maint_a_vol_lvl = 4350,
		.maint_a_chg_timer_h = 60,
		.maint_b_cur_lvl = 800,
		.maint_b_vol_lvl = 4350,
		.maint_b_chg_timer_h = 200,
		.low_high_cur_lvl = 500,
		.low_high_vol_lvl = 4350,
#ifdef CONFIG_MEASURE_TEMP_BY_ADC_TABLE
		.n_temp_tbl_elements = ARRAY_SIZE(adc_temp_tbl),
		.r_to_t_tbl = adc_temp_tbl,
#else
		.n_temp_tbl_elements = ARRAY_SIZE(temp_tbl),
		.r_to_t_tbl = temp_tbl,
#endif
		.n_v_cap_tbl_elements = ARRAY_SIZE(cap_tbl_5ma),
		.v_to_cap_tbl = cap_tbl_5ma,
#ifdef CONFIG_SAMSUNG_CHARGER_SPEC
		.n_v_res_tbl_elements = ARRAY_SIZE(res_tbl),
		.v_to_res_tbl = res_tbl,
		.n_v_chg_res_tbl_elements = ARRAY_SIZE(chg_res_tbl),
		.v_to_chg_res_tbl = chg_res_tbl,
			/* specification is 25 +/- 5 seconds. 30*HZ */
		.timeout_chargeoff_time = 21*HZ,
			/* 5 hours for first charge attempt */
		.initial_timeout_time = HZ*3600*5,
			/* 1.5 hours for second and later attempts */
		.subsequent_timeout_time = HZ*60*90,
			/* After an error stop charging for a minute. */
		.error_charge_stoptime = HZ*60,
		.over_voltage_threshold =  4450,
#else
		.n_batres_tbl_elements = ARRAY_SIZE(temp_to_batres_tbl),
		.batres_tbl = temp_to_batres_tbl,
#endif
	},
};

static char *ab8500_charger_supplied_to[] = {
	"ab8500_chargalg",
	"ab8500_fg",
	"ab8500_btemp",
};

static char *ab8500_btemp_supplied_to[] = {
	"ab8500_chargalg",
	"ab8500_fg",
};

static char *ab8500_fg_supplied_to[] = {
	"ab8500_chargalg",
	"ab8500_usb",
};

static char *ab8500_chargalg_supplied_to[] = {
	"ab8500_fg",
};

struct ab8500_charger_platform_data ab8500_charger_plat_data = {
	.supplied_to = ab8500_charger_supplied_to,
	.num_supplicants = ARRAY_SIZE(ab8500_charger_supplied_to),
	.autopower_cfg = true,
	.ac_enabled = true,
	.usb_enabled = true,
};

struct ab8500_btemp_platform_data ab8500_btemp_plat_data = {
	.supplied_to = ab8500_btemp_supplied_to,
	.num_supplicants = ARRAY_SIZE(ab8500_btemp_supplied_to),
};

struct ab8500_fg_platform_data ab8500_fg_plat_data = {
	.supplied_to = ab8500_fg_supplied_to,
	.num_supplicants = ARRAY_SIZE(ab8500_fg_supplied_to),
};

struct ab8500_chargalg_platform_data ab8500_chargalg_plat_data = {
	.supplied_to = ab8500_chargalg_supplied_to,
	.num_supplicants = ARRAY_SIZE(ab8500_chargalg_supplied_to),
};

static const struct ab8500_bm_capacity_levels cap_levels = {
	.critical	= 2,
	.low		= 10,
	.normal		= 70,
	.high		= 95,
	.full		= 100,
};

static const struct ab8500_fg_parameters fg = {
	.recovery_sleep_timer = 10,
	.recovery_total_time = 100,
	.init_timer = 1,
	.init_discard_time = 5,
	.init_total_time = 40,
	.high_curr_time = 60,
	.accu_charging = 20,
	.accu_high_curr = 20,
	.high_curr_threshold = 50,
	.lowbat_threshold = 3300,
	.battok_raising_th_sel1 = 2860,
	.battok_falling_th_sel0 = 2710,
	.user_cap_limit = 15,
	.maint_thres = 97,
};

static const struct ab8500_maxim_parameters maxi_params = {
	.ena_maxi = true,
	.chg_curr = 1500,
	.wait_cycles = 10,
	.charger_curr_step = 100,
};

static const struct ab8500_bm_charger_parameters chg = {
	.usb_volt_max		= 5500,
	.usb_curr_max		= 500,
	/* When power supply set as 7000mV (OVP SPEC above 6.8V)
	   SET read it as .ac_volt_max.
	   After charging is disabled, SET read the voltage
	   as .ac_volt_max_recovery.
	   This value should be modified according to the model
	   experimentally.
	   There's distinct difference between ac voltage when charging
	   and ac voltage when discharging.
	*/
	.ac_volt_max		= 6650,
	.ac_curr_max		= 1500,
#ifdef CONFIG_SAMSUNG_CHARGER_SPEC
	.ac_volt_max_recovery	= 6800,
	.usb_volt_max_recovery	= 5700,
#endif
};

struct ab8500_bm_data ab8500_bm_data = {
	.temp_under		= -5,
	.temp_low		= 0,
	.temp_high		= 51,
	.temp_over		= 52,
	.main_safety_tmr_h	= 4,
	.temp_interval_chg	= 20,
	.temp_interval_nochg	= 120,
#if defined( CONFIG_USB_SWITCHER ) || defined( CONFIG_INPUT_AB8505_MICRO_USB_DETECT )
	.ta_chg_current		= 800,
	.ta_chg_current_input	= 1500,
	.ta_chg_voltage		= 4200,

	.usb_chg_current	= 500,
	.usb_chg_current_input	= 500,
	.usb_chg_voltage	= 4200,
#endif
	.main_safety_tmr_h	= 4,
	.usb_safety_tmr_h	= 4,
	.bkup_bat_v		= BUP_VCH_SEL_3P1V,
	.bkup_bat_i		= BUP_ICH_SEL_50UA,
	.no_maintenance		= true,
#ifdef CONFIG_AB8500_BATTERY_THERM_ON_BATCTRL
	.adc_therm		= ADC_THERM_BATCTRL,
#else
	.adc_therm		= ADC_THERM_BATTEMP,
#endif
	.chg_unknown_bat	= true,
	.enable_overshoot	= false,
	/* Please find the real setting for fg_res
	   in the ab8500_fg.c probe function  */
	.fg_res			= 133,
	.cap_levels		= &cap_levels,
	.bat_type		= bat_type,
	.n_btypes		= ARRAY_SIZE(bat_type),
	.batt_id		= 0,
	.interval_charging	= 5,
	.interval_not_charging	= 120,
	.temp_hysteresis	= 22,		/* turn back on if temp < 43 */
	.maxi			= &maxi_params,
	.chg_params		= &chg,
	.fg_params		= &fg,
#ifdef CONFIG_SAMSUNG_CHARGER_SPEC
	.charge_state		= 1,
	.batt_res		= 0,
	.low_temp_hysteresis	= 3,
	.use_safety_timer	= 0 ,
#endif
};
