/*
 *  drivers/cpufreq/cpufreq_ondemand.c
 *
 *  Copyright (C)  2001 Russell King
 *            (C)  2003 Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>.
 *                      Jun Nakajima <jun.nakajima@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/kconfig.h>

#ifdef CONFIG_CPU_FREQ_SUSPEND
#include <linux/earlysuspend.h>
#endif

#if IS_ENABLED(CONFIG_A2N)
#include <linux/a2n.h>
#endif

extern bool should_boost_cpu_for_gpu;

/*
 * dbs is used in this file as a shortform for demandbased switching
 * It helps to keep variable names smaller, simpler
 */
#define DEF_FREQUENCY_UP_THRESHOLD		(95)
#define DOWN_THRESHOLD_MARGIN			(25)
#define DEF_SAMPLING_DOWN_FACTOR		(50)
#define MAX_SAMPLING_DOWN_FACTOR		(100000)
#define MICRO_FREQUENCY_UP_THRESHOLD		(95)
#define MIN_FREQUENCY_UP_THRESHOLD		(60)
#define MAX_FREQUENCY_UP_THRESHOLD		(100)
#define DEF_BOOST				(0)
#define IO_IS_BUSY				(0)

#define DEF_FREQUENCY_STEP_0		(200000)
#define DEF_FREQUENCY_STEP_1		(400000)
#define DEF_FREQUENCY_STEP_2		(600000)
#define DEF_FREQUENCY_STEP_3		(800000)
#define DEF_FREQUENCY_STEP_4		(1000000)
#define DEF_FREQUENCY_STEP_5		(1100000)
#define DEF_FREQUENCY_STEP_6		(1150000)
#define DEF_FREQUENCY_STEP_7		(1200000)
#define DEF_FREQUENCY_STEP_8		(1250000)
#define DEF_FREQUENCY_STEP_9		(1300000)

/*
 * The polling frequency of this governor depends on the capability of
 * the processor. Default polling frequency is 1000 times the transition
 * latency of the processor. The governor will work on any processor with
 * transition latency <= 10mS, using appropriate sampling
 * rate.
 * For CPUs with transition latency > 10mS (mostly drivers with CPUFREQ_ETERNAL)
 * this governor will not work.
 * All times here are in uS.
 */
#define MIN_SAMPLING_RATE_RATIO			(10)

#define LATENCY_MULTIPLIER			(1000)
#define MIN_LATENCY_MULTIPLIER			(100)
#define TRANSITION_LATENCY_LIMIT		(10 * 1000 * 1000)

static unsigned int min_sampling_rate = 0;
static unsigned int down_threshold = 0;

#ifdef CONFIG_CPU_FREQ_SUSPEND
static unsigned int up_threshold_resume = DEF_FREQUENCY_UP_THRESHOLD;
static unsigned int up_threshold_suspend = 95;
static bool boost_resume = DEF_BOOST;
static bool boost_suspend = false;
static struct early_suspend early_suspend;

extern bool enable_suspend_freqs;
static bool update_gov_tunables = false;
#endif

static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				unsigned int event);

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_ONDEMAND
static
#endif
struct cpufreq_governor cpufreq_gov_ondemand = {
       .name                   = "ondemand",
       .governor               = cpufreq_governor_dbs,
       .max_transition_latency = TRANSITION_LATENCY_LIMIT,
       .owner                  = THIS_MODULE,
};

/* Sampling types */
enum {DBS_NORMAL_SAMPLE, DBS_SUB_SAMPLE};

struct cpu_dbs_info_s {
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_iowait;
	cputime64_t prev_cpu_wall;
	cputime64_t prev_cpu_nice;
	struct cpufreq_policy *cur_policy;
	struct cpufreq_frequency_table *freq_table;
	unsigned int rate_mult;
	int cpu;
	unsigned int sample_type;
	/*
	 * percpu mutex that serializes governor limit change with
	 * do_dbs_timer invocation. We do not want do_dbs_timer to run
	 * when user is changing the governor or limits.
	 */
	struct mutex timer_mutex;
};
static DEFINE_PER_CPU(struct cpu_dbs_info_s, od_cpu_dbs_info);
static DEFINE_PER_CPU(struct delayed_work, ondemand_work);

static unsigned int dbs_enable;	/* number of CPUs using this policy */
static ktime_t time_stamp;

/*
 * dbs_mutex protects dbs_enable in governor start/stop.
 */
static DEFINE_MUTEX(dbs_mutex);

static struct dbs_tuners {
	unsigned int sampling_rate;
	unsigned int up_threshold;
	unsigned int ignore_nice;
	unsigned int sampling_down_factor;
	unsigned int io_is_busy;
	bool boost;
} dbs_tuners_ins = {
	.up_threshold = DEF_FREQUENCY_UP_THRESHOLD,
	.sampling_down_factor = DEF_SAMPLING_DOWN_FACTOR,
	.ignore_nice = 0,
	.boost = DEF_BOOST,
};

static inline cputime64_t get_cpu_idle_time_jiffy(unsigned int cpu,
							cputime64_t *wall)
{
	cputime64_t idle_time;
	cputime64_t cur_wall_time;
	cputime64_t busy_time;

	cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());
	busy_time = cputime64_add(kstat_cpu(cpu).cpustat.user,
			kstat_cpu(cpu).cpustat.system);

	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.irq);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.softirq);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.steal);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.nice);

	idle_time = cputime64_sub(cur_wall_time, busy_time);
	if (wall)
		*wall = (cputime64_t)jiffies_to_usecs(cur_wall_time);

	return (cputime64_t)jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
	u64 idle_time = get_cpu_idle_time_us(cpu, NULL);

	if (idle_time == -1ULL)
		return get_cpu_idle_time_jiffy(cpu, wall);
	else
		idle_time += get_cpu_iowait_time_us(cpu, wall);

	return idle_time;
}

static inline cputime64_t get_cpu_iowait_time(unsigned int cpu, cputime64_t *wall)
{
	u64 iowait_time = get_cpu_iowait_time_us(cpu, wall);

	if (iowait_time == -1ULL)
		return 0;

	return iowait_time;
}

static void update_down_threshold(void)
{
	down_threshold = ((dbs_tuners_ins.up_threshold * DEF_FREQUENCY_STEP_0 / DEF_FREQUENCY_STEP_1) - DOWN_THRESHOLD_MARGIN);
	pr_info("[%s] for CPU: new value: %u\n",__func__, down_threshold);
}

/************************** sysfs interface ************************/

static ssize_t show_sampling_rate_min(struct kobject *kobj,
				      struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", min_sampling_rate);
}

define_one_global_ro(sampling_rate_min);

/* cpufreq_ondemand Governor Tunables */
#define show_one(file_name, object)					\
static ssize_t show_##file_name						\
(struct kobject *kobj, struct attribute *attr, char *buf)              \
{									\
	return sprintf(buf, "%u\n", dbs_tuners_ins.object);		\
}
show_one(sampling_rate, sampling_rate);
show_one(io_is_busy, io_is_busy);
show_one(up_threshold, up_threshold);
show_one(sampling_down_factor, sampling_down_factor);
show_one(ignore_nice_load, ignore_nice);
show_one(boost, boost);

static ssize_t store_sampling_rate(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.sampling_rate = max(input, min_sampling_rate);
	return count;
}

static ssize_t store_io_is_busy(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.io_is_busy = !!input;
	return count;
}

static ssize_t store_up_threshold(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;

#if IS_ENABLED(CONFIG_A2N)
	if (!a2n_allow) {
		pr_err("[%s] a2n: unprivileged access !\n",__func__);
		return -EINVAL;
	}
#endif

	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_FREQUENCY_UP_THRESHOLD ||
			input < MIN_FREQUENCY_UP_THRESHOLD) {
		return -EINVAL;
	}
	dbs_tuners_ins.up_threshold = input;
#ifdef CONFIG_CPU_FREQ_SUSPEND
	up_threshold_resume = input;
#endif
	/* update down_threshold */
	update_down_threshold();
	return count;
}

static ssize_t store_sampling_down_factor(struct kobject *a,
			struct attribute *b, const char *buf, size_t count)
{
	unsigned int input, j;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_DOWN_FACTOR || input < 1)
		return -EINVAL;
	dbs_tuners_ins.sampling_down_factor = input;

	/* Reset down sampling multiplier in case it was active */
	for_each_online_cpu(j) {
		struct cpu_dbs_info_s *dbs_info;
		dbs_info = &per_cpu(od_cpu_dbs_info, j);
		dbs_info->rate_mult = 1;
	}
	return count;
}

static ssize_t store_ignore_nice_load(struct kobject *a, struct attribute *b,
				      const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	unsigned int j;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input > 1)
		input = 1;

	if (input == dbs_tuners_ins.ignore_nice) { /* nothing to do */
		return count;
	}
	dbs_tuners_ins.ignore_nice = input;

	/* we need to re-evaluate prev_cpu_idle */
	for_each_online_cpu(j) {
		struct cpu_dbs_info_s *dbs_info;
		dbs_info = &per_cpu(od_cpu_dbs_info, j);
		dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&dbs_info->prev_cpu_wall);
		if (dbs_tuners_ins.ignore_nice)
			dbs_info->prev_cpu_nice = kstat_cpu(j).cpustat.nice;

	}
	return count;
}

static ssize_t store_boost(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;

#if IS_ENABLED(CONFIG_A2N)
	if (!a2n_allow) {
		pr_err("[%s] a2n: unprivileged access !\n",__func__);
		goto err;
	}
#endif

	ret = sscanf(buf, "%u", &input);
	if (ret != 1 || input < 0 || input > 1)
		goto err;
	dbs_tuners_ins.boost = input;

#ifdef CONFIG_CPU_FREQ_SUSPEND
	boost_resume = input;
#endif

	return count;

err:
	pr_err("[%s] invalid cmd\n",__func__);
	return -EINVAL;
}

define_one_global_rw(sampling_rate);
define_one_global_rw(io_is_busy);
define_one_global_rw(up_threshold);
define_one_global_rw(sampling_down_factor);
define_one_global_rw(ignore_nice_load);
define_one_global_rw(boost);

static struct attribute *dbs_attributes[] = {
	&sampling_rate_min.attr,
	&sampling_rate.attr,
	&up_threshold.attr,
	&sampling_down_factor.attr,
	&ignore_nice_load.attr,
	&io_is_busy.attr,
	&boost.attr,
	NULL
};

static struct attribute_group dbs_attr_group = {
	.attrs = dbs_attributes,
	.name = "ondemand",
};

/************************** sysfs end ************************/

static bool dbs_sw_coordinated_cpus(void)
{
	struct cpu_dbs_info_s *dbs_info;
	struct cpufreq_policy *policy;
	int i = 0;
	int j;

	dbs_info = &per_cpu(od_cpu_dbs_info, 0);
	policy = dbs_info->cur_policy;

	for_each_cpu(j, policy->cpus) {
		i++;
	}

	if (i > 1)
		return true; /* Dependant CPUs */
	else
		return false;
}

static void dbs_check_cpu(struct cpu_dbs_info_s *this_dbs_info)
{
	struct cpufreq_policy *policy;
	unsigned int j;
	unsigned int load;
	unsigned int requested_freq;
	policy = this_dbs_info->cur_policy;

	/*
	 * Every sampling_rate, we check, if current idle time is less
	 * than 20% (default), then we try to increase frequency
	 * Every sampling_rate, we look for a the lowest
	 * frequency which can sustain the load while keeping idle time over
	 * 30%. If such a frequency exist, we try to decrease to this frequency.
	 *
	 * Any frequency increase takes it to the maximum frequency.
	 * Frequency reduction happens at minimum steps of
	 * 5% (default) of current frequency
	 */

	/* Get Absolute Load */
	for_each_cpu(j, policy->cpus) {
		struct cpu_dbs_info_s *j_dbs_info;
		cputime64_t cur_wall_time, cur_idle_time, cur_iowait_time;
		unsigned int idle_time, wall_time, iowait_time;

		j_dbs_info = &per_cpu(od_cpu_dbs_info, j);

		cur_idle_time = get_cpu_idle_time(j, &cur_wall_time);
		cur_iowait_time = get_cpu_iowait_time(j, &cur_wall_time);

		wall_time = (unsigned int) cputime64_sub(cur_wall_time,
				j_dbs_info->prev_cpu_wall);
		j_dbs_info->prev_cpu_wall = cur_wall_time;

		idle_time = (unsigned int) cputime64_sub(cur_idle_time,
				j_dbs_info->prev_cpu_idle);
		j_dbs_info->prev_cpu_idle = cur_idle_time;

		iowait_time = (unsigned int) cputime64_sub(cur_iowait_time,
				j_dbs_info->prev_cpu_iowait);
		j_dbs_info->prev_cpu_iowait = cur_iowait_time;

		if (dbs_tuners_ins.ignore_nice) {
			cputime64_t cur_nice;
			unsigned long cur_nice_jiffies;

			cur_nice = cputime64_sub(kstat_cpu(j).cpustat.nice,
					 j_dbs_info->prev_cpu_nice);
			/*
			 * Assumption: nice time between sampling periods will
			 * be less than 2^32 jiffies for 32 bit sys
			 */
			cur_nice_jiffies = (unsigned long)
					cputime64_to_jiffies64(cur_nice);

			j_dbs_info->prev_cpu_nice = kstat_cpu(j).cpustat.nice;
			idle_time += jiffies_to_usecs(cur_nice_jiffies);
		}

		/*
		 * For the purpose of ondemand, waiting for disk IO is an
		 * indication that you're performance critical, and not that
		 * the system is actually idle. So subtract the iowait time
		 * from the cpu idle time.
		 */

		if (dbs_tuners_ins.io_is_busy && idle_time >= iowait_time)
			idle_time -= iowait_time;

		if (unlikely(!wall_time || wall_time < idle_time))
			continue;

		load = 100 * (wall_time - idle_time) / wall_time;
	}

	/* Check for frequency increase */
	if (load >= dbs_tuners_ins.up_threshold) {

		/* if we are already at full speed then break out early */
		if (policy->cur == policy->max)
			return;

		if (!dbs_tuners_ins.boost) {
			if (policy->cur == DEF_FREQUENCY_STEP_0)
				requested_freq = DEF_FREQUENCY_STEP_1;
			else if (policy->cur == DEF_FREQUENCY_STEP_1)
				requested_freq = DEF_FREQUENCY_STEP_2;
			else if (policy->cur == DEF_FREQUENCY_STEP_2)
				requested_freq = DEF_FREQUENCY_STEP_3;
			else if (policy->cur == DEF_FREQUENCY_STEP_3)
				requested_freq = DEF_FREQUENCY_STEP_4;
			else if (policy->cur == DEF_FREQUENCY_STEP_4)
				requested_freq = DEF_FREQUENCY_STEP_5;
			else if (policy->cur == DEF_FREQUENCY_STEP_5)
				requested_freq = DEF_FREQUENCY_STEP_6;
			else if (policy->cur == DEF_FREQUENCY_STEP_6)
				requested_freq = DEF_FREQUENCY_STEP_7;
			else if (policy->cur == DEF_FREQUENCY_STEP_7)
				requested_freq = DEF_FREQUENCY_STEP_8;
			else
				requested_freq = policy->max;

			if (requested_freq > policy->max)
				requested_freq = policy->max;

		} else {
			/* Boost */
			requested_freq = policy->max;
		}

		/* If switching to max speed, apply sampling_down_factor */
		if (requested_freq == policy->max)
			this_dbs_info->rate_mult =
				dbs_tuners_ins.sampling_down_factor;

		__cpufreq_driver_target(policy, requested_freq,
			CPUFREQ_RELATION_H);

		return;
	}

	/* Gaming mode: don't reduce CPU Freq */
	if (should_boost_cpu_for_gpu)
		return;

	/* No longer fully busy, reset rate_mult */
	this_dbs_info->rate_mult = 1;

	/*
	 * if we cannot reduce the frequency anymore, break out early
	 */
	if (policy->cur == policy->min)
		return;

	/* Check for frequency decrease */
	if (load < down_threshold) {
		if (policy->cur >= DEF_FREQUENCY_STEP_9)
			requested_freq = DEF_FREQUENCY_STEP_8;
		else if (policy->cur == DEF_FREQUENCY_STEP_8)
			requested_freq = DEF_FREQUENCY_STEP_7;
		else if (policy->cur == DEF_FREQUENCY_STEP_7)
			requested_freq = DEF_FREQUENCY_STEP_6;
		else if (policy->cur == DEF_FREQUENCY_STEP_6)
			requested_freq = DEF_FREQUENCY_STEP_5;
		else if (policy->cur == DEF_FREQUENCY_STEP_5)
			requested_freq = DEF_FREQUENCY_STEP_4;
		else if (policy->cur == DEF_FREQUENCY_STEP_4)
			requested_freq = DEF_FREQUENCY_STEP_3;
		else if (policy->cur == DEF_FREQUENCY_STEP_3)
			requested_freq = DEF_FREQUENCY_STEP_2;
		else if (policy->cur == DEF_FREQUENCY_STEP_2)
			requested_freq = DEF_FREQUENCY_STEP_1;
		else
			requested_freq = policy->min;

		if (requested_freq < policy->min)
			requested_freq = policy->min;

		__cpufreq_driver_target(policy, requested_freq,
				CPUFREQ_RELATION_L);
	}
}

static void do_dbs_timer(struct work_struct *work)
{
	struct cpu_dbs_info_s *dbs_info;
	unsigned int cpu = smp_processor_id();
	int sample_type;
	int delay;
	bool sample = true;

	/* If SW dependant CPUs, use CPU 0 as leader */
	if (dbs_sw_coordinated_cpus()) {

		ktime_t time_now;
		s64 delta_us;

		dbs_info = &per_cpu(od_cpu_dbs_info, 0);
		mutex_lock(&dbs_info->timer_mutex);

		time_now = ktime_get();
		delta_us = ktime_us_delta(time_now, time_stamp);

		/* Do nothing if we recently have sampled */
		if (delta_us < (s64)(dbs_tuners_ins.sampling_rate / 2))
			sample = false;
		else
			time_stamp = time_now;
	} else {
		dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
		mutex_lock(&dbs_info->timer_mutex);
	}

	sample_type = dbs_info->sample_type;

	/* Common NORMAL_SAMPLE setup */
	dbs_info->sample_type = DBS_NORMAL_SAMPLE;
	if (sample_type == DBS_NORMAL_SAMPLE) {
		if (sample)
			dbs_check_cpu(dbs_info);
		/* We want all CPUs to do sampling nearly on
		 * same jiffy
		 */
		delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate
				* dbs_info->rate_mult);

		if (num_online_cpus() > 1)
			delay -= jiffies % delay;
	}
	schedule_delayed_work_on(cpu, &per_cpu(ondemand_work, cpu), delay);
	mutex_unlock(&dbs_info->timer_mutex);
}

static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info, int cpu)
{
	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);

	if (num_online_cpus() > 1)
		delay -= jiffies % delay;

	dbs_info->sample_type = DBS_NORMAL_SAMPLE;
	cancel_delayed_work_sync(&per_cpu(ondemand_work, cpu));
	schedule_delayed_work_on(cpu, &per_cpu(ondemand_work, cpu), delay);
}

static inline void dbs_timer_exit(int cpu)
{
	cancel_delayed_work_sync(&per_cpu(ondemand_work, cpu));
}

static void dbs_timer_exit_per_cpu(struct work_struct *dummy)
{
	dbs_timer_exit(smp_processor_id());
}

static int __cpuinit cpu_callback(struct notifier_block *nfb,
				  unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;
	struct sys_device *sys_dev;
	struct cpu_dbs_info_s *dbs_info;

	if (dbs_sw_coordinated_cpus())
		dbs_info = &per_cpu(od_cpu_dbs_info, 0);
	else
		dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

	sys_dev = get_cpu_sysdev(cpu);
	if (sys_dev) {
		switch (action) {
		case CPU_ONLINE:
		case CPU_ONLINE_FROZEN:
			dbs_timer_init(dbs_info, cpu);
			break;
		case CPU_DOWN_PREPARE:
		case CPU_DOWN_PREPARE_FROZEN:
			dbs_timer_exit(cpu);
			break;
		case CPU_DOWN_FAILED:
		case CPU_DOWN_FAILED_FROZEN:
			dbs_timer_init(dbs_info, cpu);
			break;
		}
	}
	return NOTIFY_OK;
}

static struct notifier_block __refdata ondemand_cpu_notifier = {
	.notifier_call = cpu_callback,
};

static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				   unsigned int event)
{
	unsigned int cpu = policy->cpu;
	struct cpu_dbs_info_s *this_dbs_info;
	unsigned int j;
	int rc;

	this_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

	switch (event) {
	case CPUFREQ_GOV_START:
		if ((!cpu_online(cpu)) || (!policy->cur))
			return -EINVAL;

		mutex_lock(&dbs_mutex);

		dbs_enable++;
		for_each_cpu(j, policy->cpus) {
			struct cpu_dbs_info_s *j_dbs_info;
			j_dbs_info = &per_cpu(od_cpu_dbs_info, j);
			j_dbs_info->cur_policy = policy;

			j_dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&j_dbs_info->prev_cpu_wall);
			if (dbs_tuners_ins.ignore_nice) {
				j_dbs_info->prev_cpu_nice =
						kstat_cpu(j).cpustat.nice;
			}

			mutex_init(&j_dbs_info->timer_mutex);
			INIT_DELAYED_WORK_DEFERRABLE(&per_cpu(ondemand_work, j),
						     do_dbs_timer);

			j_dbs_info->rate_mult = 1;
		}

		this_dbs_info->cpu = cpu;
		/*
		 * Start the timerschedule work, when this governor
		 * is used for first time
		 */
		if (dbs_enable == 1) {
			unsigned int latency;

			rc = sysfs_create_group(cpufreq_global_kobject,
						&dbs_attr_group);
			if (rc) {
				mutex_unlock(&dbs_mutex);
				return rc;
			}

			/* policy latency is in nS. Convert it to uS first */
			latency = policy->cpuinfo.transition_latency / 1000;
			if (latency == 0)
				latency = 1;
			/* Bring kernel and HW constraints together */
			min_sampling_rate = max(min_sampling_rate,
					MIN_LATENCY_MULTIPLIER * latency);
			dbs_tuners_ins.sampling_rate =
				max(min_sampling_rate,
				    latency * LATENCY_MULTIPLIER);
			dbs_tuners_ins.io_is_busy = IO_IS_BUSY;
		}
		mutex_unlock(&dbs_mutex);

		/* If SW coordinated CPUs then register notifier */
		if (dbs_sw_coordinated_cpus()) {
			register_hotcpu_notifier(&ondemand_cpu_notifier);

			for_each_cpu(j, policy->cpus) {
				struct cpu_dbs_info_s *j_dbs_info;

				j_dbs_info = &per_cpu(od_cpu_dbs_info, 0);
				dbs_timer_init(j_dbs_info, j);
			}

			/* Initiate timer time stamp */
			time_stamp = ktime_get();


		} else
			dbs_timer_init(this_dbs_info, cpu);
		break;

	case CPUFREQ_GOV_STOP:

		dbs_timer_exit(cpu);

		mutex_lock(&dbs_mutex);
		mutex_destroy(&this_dbs_info->timer_mutex);
		dbs_enable--;
		mutex_unlock(&dbs_mutex);
		if (!dbs_enable) {
			sysfs_remove_group(cpufreq_global_kobject,
					   &dbs_attr_group);

			if (dbs_sw_coordinated_cpus()) {
				/*
				 * Make sure all pending timers/works are
				 * stopped.
				 */
				schedule_on_each_cpu(dbs_timer_exit_per_cpu);
				unregister_hotcpu_notifier(&ondemand_cpu_notifier);
			}
		}
		break;

	case CPUFREQ_GOV_LIMITS:
		mutex_lock(&this_dbs_info->timer_mutex);
		if (policy->max < this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(this_dbs_info->cur_policy,
				policy->max, CPUFREQ_RELATION_H);
		else if (policy->min > this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(this_dbs_info->cur_policy,
				policy->min, CPUFREQ_RELATION_L);
		mutex_unlock(&this_dbs_info->timer_mutex);
		break;
	}
	return 0;
}

#ifdef CONFIG_CPU_FREQ_SUSPEND
static void cpufreq_od_early_suspend(struct early_suspend *h)
{
	if (!enable_suspend_freqs)
		return;

	dbs_tuners_ins.up_threshold = up_threshold_suspend;
	dbs_tuners_ins.boost = boost_suspend;
	update_gov_tunables = true;
}

static void cpufreq_od_late_resume(struct early_suspend *h)
{
	if (update_gov_tunables) {
		dbs_tuners_ins.up_threshold = up_threshold_resume;
		dbs_tuners_ins.boost = boost_resume;
		update_gov_tunables = false;
	}
}
#endif

static int __init cpufreq_gov_dbs_init(void)
{
	u64 idle_time;
	int cpu = get_cpu();

	idle_time = get_cpu_idle_time_us(cpu, NULL);
	put_cpu();
	if (idle_time != -1ULL) {
		/* Idle micro accounting is supported. Use finer thresholds */
		dbs_tuners_ins.up_threshold = MICRO_FREQUENCY_UP_THRESHOLD;

#ifdef CONFIG_CPU_FREQ_SUSPEND
		up_threshold_resume = MICRO_FREQUENCY_UP_THRESHOLD;
#endif

		/*
		 * In no_hz/micro accounting case we set the minimum frequency
		 * not depending on HZ, but fixed (very low). The deferred
		 * timer might skip some samples if idle/sleeping as needed.
		*/
		min_sampling_rate = jiffies_to_usecs(MIN_SAMPLING_RATE_RATIO);
	} else {
		/* For correct statistics, we need 10 ticks for each measure */
		min_sampling_rate = jiffies_to_usecs(MIN_SAMPLING_RATE_RATIO);
	}

#ifdef CONFIG_CPU_FREQ_SUSPEND
	early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	early_suspend.suspend = cpufreq_od_early_suspend;
	early_suspend.resume = cpufreq_od_late_resume;

	register_early_suspend(&early_suspend);
#endif

	update_down_threshold();

	return cpufreq_register_governor(&cpufreq_gov_ondemand);
}

static void __exit cpufreq_gov_dbs_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_ondemand);
}


MODULE_AUTHOR("Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>");
MODULE_AUTHOR("Alexey Starikovskiy <alexey.y.starikovskiy@intel.com>");
MODULE_DESCRIPTION("'cpufreq_ondemand' - A dynamic cpufreq governor for "
	"Low Latency Frequency Transition capable processors");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_ONDEMAND
fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);
