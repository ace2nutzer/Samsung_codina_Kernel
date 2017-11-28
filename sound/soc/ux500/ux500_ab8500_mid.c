#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/abx500/ab8500-gpadc.h>
#include <linux/mfd/dbx500-prcmu.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/jack.h>
#include <sound/pcm_params.h>
#include <mach/hardware.h>

#define MUTE_IC	95

/* Mute IC Control */
static int mute_IC_enable_control_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return gpio_get_value(MUTE_IC);
}

static int mute_IC_enable_control_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	pr_info ("[MID]MUTE IC Control=%d\n", ucontrol->value.integer.value[0]);

	if (ucontrol->value.integer.value[0] == 1)
		gpio_set_value(MUTE_IC, 1);
	else
		gpio_set_value(MUTE_IC, 0);

	return 1;
}

static const char * const enum_dis_ena[] = {"Disabled", "Enabled"};

static SOC_ENUM_SINGLE_EXT_DECL(soc_enum_muteIC, enum_dis_ena);

static const struct snd_kcontrol_new mid_widget_control[] = {
	SOC_ENUM_EXT("Mute IC Enable Switch",
		soc_enum_muteIC,
		mute_IC_enable_control_get,
		mute_IC_enable_control_put),
};

int ab8500_add_mid_widget(struct snd_soc_codec *p_codec)
{
	pr_info("[MID]MID WIDGET");
	snd_soc_add_controls(p_codec, mid_widget_control,
			ARRAY_SIZE(mid_widget_control));

	return 0;
}
EXPORT_SYMBOL_GPL(ab8500_add_mid_widget);