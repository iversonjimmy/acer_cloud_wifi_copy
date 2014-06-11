
//
// This file contains routines supporting ESMon - the LSM security module
//

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

#include "escsvc.h"
#include "esmon_api.h"

struct esc_devcfm_map_s {
	u32	cfm;
};
typedef struct esc_devcfm_map_s esc_devcfm_map_t;

static esc_devcfm_map_t _esc_devmap[ESMON_MINOR_MAX];

static bool bvd_is_registered = false; 
static esmon_callbacks_t mon_cb;

static bool
_esc_mon_access_check(u32 cur_minor, u32 new_minor)
{
	return true;
}

int
escsvc_bvd_register(u32 major, u32 minor_max)
{
#ifdef USE_ESMON
	int err;
#endif // USE_ESMON

	if ( bvd_is_registered ) {
		return -EINVAL;
	}

	if ( minor_max > ESMON_MINOR_MAX ) {
		return -EINVAL;
	}

	// set up our callback for esmon
	mon_cb.major = major;
	mon_cb.minor_max = minor_max;
	mon_cb._access_check = _esc_mon_access_check;

	// we're now ready to accept requests from esmon
	bvd_is_registered = true;

#ifdef USE_ESMON
	if ( (err = esmon_register(&mon_cb)) ) {
		printk(KERN_INFO "%s: Failed esmon_register()\n", __FUNCTION__);
		bvd_is_registered = false;
		return err;
	}
#endif // USE_ESMON

	printk(KERN_INFO "%s: major %d minors %d\n", __FUNCTION__,
		(s32)major, (s32)minor_max);

	return 0;
}
EXPORT_SYMBOL_GPL(escsvc_bvd_register);


static void
_esc_bvd_minor_unmap(u32 minor)
{
	_esc_devmap[minor].cfm = 0;
#ifdef USE_ESMON
	// grrr
	if ( esmon_minor_unmap(minor) ) {
		printk(KERN_INFO "%s: esmon_minor_unmap(%d) failed\n",
			__FUNCTION__, (s32) minor);
	}
#endif // USE_ESMON
}

int
escsvc_bvd_unregister(void)
{
	u32 i;
#ifdef USE_ESMON
	int err;
#endif // USE_ESMON

	if ( !bvd_is_registered ) {
		return -EINVAL;
	}

	// We need to kill all our mappings because bvd will not do this.
	for( i = 0 ; i < ESMON_MINOR_MAX ; i++ ) {
		if ( _esc_devmap[i].cfm == 0 ) {
			continue;
		}
		_esc_bvd_minor_unmap(i);
	}

#ifdef USE_ESMON
	if ( (err = esmon_unregister()) ) {
		printk(KERN_INFO "%s: esmon_unregister failed\n", __FUNCTION__);
		return err;
	}
#endif // USE_ESMON

	bvd_is_registered = false;

	return 0;
}
EXPORT_SYMBOL_GPL(escsvc_bvd_unregister);

int
escsvc_bvd_dev_map(u32 minor, u32 cfm)
{
#ifdef USE_ESMON
	int err;
#endif // USE_ESMON

	// shouldn't need locks here as register/unregister come from
	// the init/exit entry points of the driver. Which, in theory
	// means we shouldn't even need this check...
	if ( !bvd_is_registered ) {
		return -EINVAL;
	}

	if (  minor >= mon_cb.minor_max ) {
		return -EINVAL;
	}

	// see if there's already a mapping for this minor.
	if ( _esc_devmap[minor].cfm != 0 ) {
		_esc_bvd_minor_unmap(minor);
	}

	// XXX - verify that this is a valid cfm handle, maybe with a
	// query svc call?

#ifdef USE_ESMON
	if ( (err = esmon_minor_map(minor)) ) {
		printk(KERN_INFO "%s: esmon_minor_map(%d) failed\n",
			__FUNCTION__, (s32)minor);
		return err;
	}
#endif // USE_ESMON

	// record this mapping somewhere.
	_esc_devmap[minor].cfm = cfm;
	
	return 0;
}
EXPORT_SYMBOL_GPL(escsvc_bvd_dev_map);

// This one will have to be called internally from the release
// content call since bvd doesn't provide a mechanism to unmap a device.
// It's never told. That's a bit retarded but there you go.
void
escsvc_bvd_cfm_release(u32 cfm)
{
#ifdef USE_ESMON
	int err;
#endif // USE_ESMON
	u32 i;

	// shouldn't need locks here as register/unregister come from
	// the init/exit entry points of the driver. Which, in theory
	// means we shouldn't even need this check...
	if ( !bvd_is_registered ) {
		return;
	}

	// Nothing prevents a content from being mapped multiple times.
	// As far as I can tell.
	for( i = 0 ; i < ESMON_MINOR_MAX ; i++ ) {
		// Make sure there's some kind of mapping here.
		if ( _esc_devmap[i].cfm != cfm ) {
			continue;
		}
		
		// record this mapping somewhere.
		_esc_devmap[i].cfm = 0;

#ifdef USE_ESMON
		// grrr
		if ( (err = esmon_minor_unmap(i)) ) {
			printk(KERN_INFO "%s: esmon_minor_unmap(%d) failed\n",
				__FUNCTION__, (s32)i);
		}
#endif // USE_ESMON
	}
}
