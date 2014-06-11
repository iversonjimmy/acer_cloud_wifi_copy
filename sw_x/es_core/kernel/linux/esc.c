/*
 * Copyright 2010 iGware Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License and no later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 * NON INFRINGEMENT. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/random.h>
#include <linux/slab.h>

#include "esc_ioc.h"
#include "esc_data.h"
#include "escsvc.h"

#define DEVICE_NAME	"esc"

static int major;
static struct class *esc_class;
static struct device *esc_dev;

esc_device_id_t esc_dev_id;  // referenced from esc_ioc.c
static int dev_id_attr_is_setup;
static int dev_type_attr_is_setup;
static int dev_cert_attr_is_setup;

//
// The following set up the associated character device that provides
// ioctl() support.
//

static int
escdev_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int
escdev_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations escdev_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = escdev_ioctl,
	.open = escdev_open,
	.release = escdev_release,
};

static ssize_t _dev_id_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%08x\n", esc_dev_id.device_id);
}

static struct device_attribute dev_id_attr = {
	.attr = { .name = "device_id", .mode = S_IRUGO},
	.show = _dev_id_show,
};

static ssize_t _dev_type_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%08x\n", esc_dev_id.device_type);
}

static struct device_attribute dev_type_attr = {
	.attr = { .name = "device_type", .mode = S_IRUGO},
	.show = _dev_type_show,
};

static ssize_t _dev_cert_read(struct kobject *kobj, 
			      struct bin_attribute *bap,
			      char *buf, loff_t off,
			      size_t len)
{
	size_t avail;
	size_t xfer_size;

	avail = (off >= sizeof(esc_dev_id.device_cert)) 
		? 0 : (sizeof(esc_dev_id.device_cert) - off); 

	xfer_size = (avail < len) ? avail : len;
	memcpy(buf, &esc_dev_id.device_cert[off], xfer_size);
printk(KERN_INFO "cert read: off 0x%08x len 0x%08x xs 0x%08x\n", (u32)off, len,
	xfer_size);

	return xfer_size;
}

static struct bin_attribute dev_cert_attr = {
	.attr = { .name = "device_cert", .mode = S_IRUGO},
	.read = _dev_cert_read,
};

int
esc_mod_device_info_setup(void)
{
	int	rv;

	// perform a hypercall to pull out the device ID and certificte.
	rv = escsvc_request_sync(ESCSVC_ES_DEVICE_ID_GET, &esc_dev_id,
				 sizeof(esc_dev_id));
	if ( rv ) {
		printk(KERN_ERR "%s: failed to queue request, err=%d\n", __func__, rv);
		rv = -ENOBUFS;
		return rv;
	}
	if ( esc_dev_id.error ) {
		printk(KERN_ERR "Service error %d\n", esc_dev_id.error);
		rv = -EINVAL;
		return rv;
	}

	printk(KERN_INFO "Dev id = 0x%08x\n", esc_dev_id.device_id);

	// Create a text attribute for the device id.
	rv = device_create_file(esc_dev, &dev_id_attr);
	if ( rv ) {
		return rv;
	}
	dev_id_attr_is_setup = 1;

	printk(KERN_INFO "Dev type = 0x%08x\n", esc_dev_id.device_type);

	// Create a text attribute for the device type.
	rv = device_create_file(esc_dev, &dev_type_attr);
	if ( rv ) {
		return rv;
	}
	dev_type_attr_is_setup = 1;

	// Create a binary attribute for the device certificate.
	rv = device_create_bin_file(esc_dev, &dev_cert_attr);
	if ( rv ) {
		return rv;
	}
	dev_cert_attr_is_setup = 1;

	return 0;
}

void
esc_mod_device_info_cleanup(void)
{
        if (dev_cert_attr_is_setup) {
		device_remove_bin_file(esc_dev, &dev_cert_attr);
		dev_cert_attr_is_setup = 0;
	}

        if (dev_type_attr_is_setup) {
		device_remove_file(esc_dev, &dev_type_attr);
		dev_type_attr_is_setup = 0;
	}

        if (dev_id_attr_is_setup) {
		device_remove_file(esc_dev, &dev_id_attr);
		dev_id_attr_is_setup = 0;
	}
}

static int __init
esc_mod_init(void)
{
	extern void core_glue_init(void);  // defined in core_glue.c

	int err = 0;

	printk(KERN_INFO "ESC Module initializing\n");

	esc_class = class_create(THIS_MODULE, "esc");
	if (IS_ERR(esc_class)) {
		err = PTR_ERR(esc_class);
		goto fail;
	}

	major = register_chrdev(0, DEVICE_NAME, &escdev_fops);
	if (major < 0) {
		printk(KERN_ERR "%s: Driver Initialisation failed - %d\n",
		       __FILE__, major);
		err = -EINVAL;
		goto fail;
	}

	esc_dev = device_create(esc_class, NULL, MKDEV(major, 0), NULL,
				 "esc");

	core_glue_init();

	if ( (err = escsvc_init()) ) {
		printk(KERN_ERR "failed to initialize command queue - %d\n", err);
		goto fail;
	}

	return 0;

fail:
	esc_mod_device_info_cleanup();
	escsvc_cleanup();

	return err;
}

static void __exit
esc_mod_exit(void)
{
	extern void core_glue_exit(void);  // defined in core_glue.c

	esc_mod_device_info_cleanup();
	escsvc_cleanup();
	core_glue_exit();
	device_destroy(esc_class, MKDEV(major, 0));
	unregister_chrdev(major, DEVICE_NAME);
	class_destroy(esc_class);
}

module_init(esc_mod_init);
module_exit(esc_mod_exit);

// XXX - I don't think we want this to be GPL at this point.
MODULE_LICENSE("GPL");

// The following lines are needed by Emacs to maintain the format the file started in.
// Local Variables:
// c-basic-offset: 8
// tab-width: 8
// indent-tabs-mode: t
// End:
