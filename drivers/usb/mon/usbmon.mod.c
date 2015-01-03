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
	{ 0x2b9a3041, "usb_mon_register" },
	{ 0xfc586f8f, "kmem_cache_destroy" },
	{ 0x8c3e09bd, "cdev_del" },
	{ 0xda2a7b07, "kmalloc_caches" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0x27540856, "cdev_init" },
	{ 0xd5b037e1, "kref_put" },
	{ 0x9b388444, "get_zeroed_page" },
	{ 0xfbc74f64, "__copy_from_user" },
	{ 0x528c709d, "simple_read_from_buffer" },
	{ 0xb354541d, "debugfs_create_dir" },
	{ 0x8aedef39, "mem_map" },
	{ 0x67c2fa54, "__copy_to_user" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x9b736001, "page_address" },
	{ 0x3be89d3c, "usb_register_notify" },
	{ 0x257a6841, "usb_debug_root" },
	{ 0x718565d5, "remove_wait_queue" },
	{ 0xea0b2c4a, "no_llseek" },
	{ 0x376c93f0, "device_destroy" },
	{ 0xd5152710, "sg_next" },
	{ 0xbd1d5ca, "mutex_unlock" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x768df56d, "debugfs_create_file" },
	{ 0x41e92619, "__init_waitqueue_head" },
	{ 0xffd5a395, "default_wake_function" },
	{ 0xfa2a45e, "__memzero" },
	{ 0x5f754e5a, "memset" },
	{ 0xace5c0fc, "usb_bus_list" },
	{ 0x74c97f9c, "_raw_spin_unlock_irqrestore" },
	{ 0x2e1b501, "__mutex_init" },
	{ 0x27e1a049, "printk" },
	{ 0x7b11c4ce, "debugfs_remove" },
	{ 0xc2bb24f4, "kmem_cache_free" },
	{ 0x10393ad3, "mutex_lock" },
	{ 0x7150f1c3, "device_create" },
	{ 0xfed11ed1, "usb_mon_deregister" },
	{ 0x9cd6d13a, "__get_page_tail" },
	{ 0x50b6c24c, "contig_page_data" },
	{ 0x123d0609, "cdev_add" },
	{ 0xe9587909, "usb_unregister_notify" },
	{ 0x236f2428, "kmem_cache_alloc" },
	{ 0xbc10dd97, "__put_user_4" },
	{ 0x1000e51, "schedule" },
	{ 0xbd7083bc, "_raw_spin_lock_irqsave" },
	{ 0x3d345e59, "kmem_cache_create" },
	{ 0x5c894bda, "usb_bus_list_lock" },
	{ 0x4302d0eb, "free_pages" },
	{ 0x72542c85, "__wake_up" },
	{ 0x1d2e87c6, "do_gettimeofday" },
	{ 0x3fdacc6f, "add_wait_queue" },
	{ 0x83800bfa, "kref_init" },
	{ 0x37a0cba, "kfree" },
	{ 0x9d669763, "memcpy" },
	{ 0x9775cdc, "kref_get" },
	{ 0xdbb03cc7, "class_destroy" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x701d0ebd, "snprintf" },
	{ 0xa64d4c46, "outer_cache" },
	{ 0x74405bbe, "__class_create" },
	{ 0x29537c9e, "alloc_chrdev_region" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "7D0A9E27EC0CF61A7768770");
