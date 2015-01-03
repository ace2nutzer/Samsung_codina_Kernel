#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xe390e4d4, "module_layout" },
	{ 0x4c307574, "usb_deregister" },
	{ 0x27e1a049, "printk" },
	{ 0xb4fa7931, "put_tty_driver" },
	{ 0x15b7f41d, "tty_unregister_driver" },
	{ 0x1fa4711b, "usb_register_driver" },
	{ 0xfe6ca26b, "tty_register_driver" },
	{ 0x55d93954, "tty_set_operations" },
	{ 0x67b27ec1, "tty_std_termios" },
	{ 0xedf3db3, "alloc_tty_driver" },
	{ 0x7acfde0c, "tty_port_close_end" },
	{ 0xcaf007ca, "tty_port_close_start" },
	{ 0x660d5ecf, "tty_port_hangup" },
	{ 0x64460d79, "tty_wakeup" },
	{ 0x66e588af, "tty_flip_buffer_push" },
	{ 0x97b695a1, "tty_insert_flip_string_fixed_flag" },
	{ 0x60d5c33b, "tty_port_block_til_ready" },
	{ 0x17d46508, "usb_autopm_put_interface" },
	{ 0xc1f9adf4, "usb_autopm_get_interface" },
	{ 0x4e331ff, "tty_port_tty_set" },
	{ 0xb90817bd, "usb_autopm_get_interface_async" },
	{ 0x71c90087, "memcmp" },
	{ 0x2308c8d, "tty_get_baud_rate" },
	{ 0x56bcd626, "tty_register_device" },
	{ 0x8f1fc0f5, "usb_get_intf" },
	{ 0x79f23f5, "usb_driver_claim_interface" },
	{ 0xe9f8e922, "_dev_info" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0xd9d17149, "device_create_file" },
	{ 0xaf77a3b1, "dev_warn" },
	{ 0xda8c4eb1, "usb_alloc_urb" },
	{ 0x6b6990bb, "usb_alloc_coherent" },
	{ 0xa6f2c052, "tty_port_init" },
	{ 0x2e1b501, "__mutex_init" },
	{ 0x236f2428, "kmem_cache_alloc" },
	{ 0xda2a7b07, "kmalloc_caches" },
	{ 0x719e0fde, "usb_ifnum_to_if" },
	{ 0xa7e7b52b, "usb_control_msg" },
	{ 0x8949858b, "schedule_work" },
	{ 0x74c97f9c, "_raw_spin_unlock_irqrestore" },
	{ 0xbd7083bc, "_raw_spin_lock_irqsave" },
	{ 0x28e3a986, "usb_driver_release_interface" },
	{ 0xe8629f6d, "dev_set_drvdata" },
	{ 0x11168c0d, "device_remove_file" },
	{ 0x37a0cba, "kfree" },
	{ 0x36aa9c44, "usb_free_urb" },
	{ 0x33393001, "usb_put_intf" },
	{ 0xb0fe93bb, "tty_unregister_device" },
	{ 0x9d669763, "memcpy" },
	{ 0x3c2c5af5, "sprintf" },
	{ 0x9f1bd040, "usb_free_coherent" },
	{ 0x311b7963, "_raw_spin_unlock" },
	{ 0xc2161e33, "_raw_spin_lock" },
	{ 0x4205ad24, "cancel_work_sync" },
	{ 0xc7b801a8, "usb_kill_urb" },
	{ 0xbd1d5ca, "mutex_unlock" },
	{ 0x10393ad3, "mutex_lock" },
	{ 0x46983d32, "dev_get_drvdata" },
	{ 0x676bbc0f, "_set_bit" },
	{ 0x2a3aa678, "_test_and_clear_bit" },
	{ 0xc212a112, "_raw_spin_unlock_irq" },
	{ 0xd59daefe, "_raw_spin_lock_irq" },
	{ 0xd96153a4, "usb_autopm_put_interface_async" },
	{ 0x531e4eb3, "tty_hangup" },
	{ 0x8e333048, "tty_kref_put" },
	{ 0x5260cfa9, "tty_port_tty_get" },
	{ 0x7d11c268, "jiffies" },
	{ 0x5f6d18, "dev_err" },
	{ 0x2e44e6df, "usb_submit_urb" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("usb:v0870p0001d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v0E8Dp0003d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v0E8Dp3329d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v0482p0203d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v079Bp000Fd*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v0ACEp1602d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v0ACEp1608d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v0ACEp1611d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v22B8p7000d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v0803p3095d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v0572p1321d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v0572p1324d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v0572p1328d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v22B8p6425d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v22B8p2D91d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v22B8p2D92d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v22B8p2D93d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v22B8p2D95d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v22B8p2D96d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v22B8p2D97d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v22B8p2D99d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v22B8p2D9Ad*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v0572p1329d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v0572p1340d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v05F9p4002d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v1BBBp0003d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v1576p03B1d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v0421p042Dd*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p04D8d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p04C9d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0419d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p044Dd*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0001d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0475d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0508d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0418d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0425d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0486d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p04DFd*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p000Ed*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0445d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p042Fd*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p048Ed*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0420d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p04E6d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p04B2d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0134d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p046Ed*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p002Fd*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0088d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p00FCd*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0042d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p00B0d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p00ABd*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0481d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0007d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0071d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p04F0d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0070d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p00E9d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0099d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0128d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p008Fd*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p00A0d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p007Bd*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0094d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p003Ad*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p00E9d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0108d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p01F5d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p02E3d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0178d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p010Ed*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p02D9d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p01D0d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0223d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0275d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p026Cd*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0154d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p04CEd*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p01D4d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0302d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p0335d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v0421p03CDd*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v04E7p6651d*dc*dsc*dp*ic02isc02ipFF*");
MODULE_ALIAS("usb:v03EBp0030d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v0694pFF00d*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v04D8p000Bd*dc*dsc*dp*ic*isc*ip*");
MODULE_ALIAS("usb:v*p*d*dc*dsc*dp*ic02isc02ip00*");
MODULE_ALIAS("usb:v*p*d*dc*dsc*dp*ic02isc02ip01*");
MODULE_ALIAS("usb:v*p*d*dc*dsc*dp*ic02isc02ip02*");
MODULE_ALIAS("usb:v*p*d*dc*dsc*dp*ic02isc02ip03*");
MODULE_ALIAS("usb:v*p*d*dc*dsc*dp*ic02isc02ip04*");
MODULE_ALIAS("usb:v*p*d*dc*dsc*dp*ic02isc02ip05*");
MODULE_ALIAS("usb:v*p*d*dc*dsc*dp*ic02isc02ip06*");

MODULE_INFO(srcversion, "BE4F7FE948F97BAE78799F3");
