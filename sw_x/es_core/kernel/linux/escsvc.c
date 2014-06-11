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
//#include "esc_pers.h"
#include <esc_data.h>
#include <esc_funcs.h>

/*
 * Ring buffer node used to communicate esc requests between
 * the ioctl front end and the servicing thread.
 */
struct escsvc_request_desc {
	u64 addr;
	u32 length;
	s16 error;
	u8 svc;
	u8 state:2;
	u8 _reserved:6;
};

static struct escsvc_completion **completions = NULL;
static struct escsvc_request_desc *ring = NULL;
static struct task_struct *thread = NULL;
static int queue_depth_cur;
static int queue_depth_max;

static spinlock_t lock;
static wait_queue_head_t escsvc_wait;

static int
dispatch_request(struct escsvc_request_desc *req)
{
	if (req == NULL)
		return -1;

	switch (req->svc) {
	case ESCSVC_ES_INIT:
		return esc_init((esc_init_t*)(u32)req->addr);
		break;
	case ESCSVC_ES_TICKET_IMPORT:
		return esc_ticket_import((esc_ticket_t*)(u32)req->addr);
		break;
	case ESCSVC_ES_TICKET_RELEASE:
		return esc_ticket_release((esc_resource_rel*)(u32)req->addr);
		break;
	case ESCSVC_ES_TICKET_QUERY:
		return esc_ticket_query((esc_ticket_query_t*)(u32)req->addr);
		break;
	case ESCSVC_ES_TITLE_IMPORT:
		return esc_title_import((esc_title_t*)(u32)req->addr);
		break;
	case ESCSVC_ES_TITLE_RELEASE:
		return esc_title_release((esc_resource_rel*)(u32)req->addr);
		break;
	case ESCSVC_ES_TITLE_QUERY:
		return esc_title_query((esc_title_query_t*)(u32)req->addr);
		break;
	case ESCSVC_ES_TITLE_EXPORT:
		return esc_title_export((esc_title_export_t*)(u32)req->addr);
		break;
	case ESCSVC_ES_CONTENT_IMPORT_ID:
		return esc_content_import((esc_content_t*)(u32)req->addr);
		break;
	case ESCSVC_ES_CONTENT_IMPORT_IDX:
		return esc_content_import_ind((esc_content_ind_t*)(u32)req->addr);
		break;
	case ESCSVC_ES_CONTENT_RELEASE: {
		// This bit of magic is to notify ES Mon that a device mapping
		// has been torn down since bvd isn't told when it's mappings
		// are no good. (Crazy)
		esc_resource_rel *rel = (esc_resource_rel *)(u32)req->addr;

		escsvc_bvd_cfm_release(rel->handle);
		return esc_content_release(rel);
		break;
	}
	case ESCSVC_ES_CONTENT_QUERY:
		return esc_content_query((esc_content_query_t*)(u32)req->addr);
		break;
	case ESCSVC_ES_CONTENT_EXPORT:
		return esc_content_export((esc_content_export_t*)(u32)req->addr);
		break;
	case ESCSVC_ES_BLOCK_DECRYPT:
		return esc_block_decrypt((esc_block_t*)(u32)req->addr);
		break;
	case ESCSVC_ES_BLOCK_ENCRYPT:
		return esc_block_encrypt((esc_block_t*)(u32)req->addr);
		break;
	case ESCSVC_ES_DEVICE_ID_GET:
		return esc_device_idcert_get((esc_device_id_t*)(u32)req->addr);
		break;
	case ESCSVC_ES_GSS_CREATE:
		return esc_gss_create((esc_gss_create_t*)(u32)req->addr);
		break;
	default:
		printk(KERN_WARNING "unknown ESCSVC cmd %u ignored\n", req->svc);
	}

	return 0;
}

static int
do_escsvc(void *arg)
{
	int i = 0;
	int err = ESCSVC_ERR_OK;

	while (!kthread_should_stop() || ring[i].state != 0) {
		wait_event_interruptible(escsvc_wait,
					 kthread_should_stop()
					 || ring[i].state != 0);

		if ( 1 ) {
			if (ring[i].state == 0) {
				continue;
			}

			// TODO dispatch command here
			err = dispatch_request(&ring[i]);

			ring[i].state = 2;
			if ( err ) {
				ring[i].error = err;
			}
		}

		while (ring[i].state == 2) {
			// stats - May want to disable this with an ifdef
			spin_lock(&lock);
			queue_depth_cur--;
			spin_unlock(&lock);

			if (completions[i]) {
				completions[i]->callback(completions[i],
							 ring[i].error);
				completions[i] = NULL;
			}

			wmb();
			ring[i].state = 0;

			i = (i + 1) % ESCSVC_RING_SIZE;
		}
	}

	return 0;
}

int
escsvc_init(void)
{
	int err = 0;

	spin_lock_init(&lock);

	ring = kcalloc(ESCSVC_RING_SIZE, sizeof *ring, GFP_KERNEL);
	completions =
	    kcalloc(ESCSVC_RING_SIZE, sizeof *completions, GFP_KERNEL);

	if (ring == NULL || completions == NULL) {
		err = -ENOMEM;
		goto fail;
	}

	init_waitqueue_head(&escsvc_wait);

	thread = kthread_create(do_escsvc, NULL, "escsvc");

	if (IS_ERR(thread)) {
		err = PTR_ERR(thread);
		goto fail;
	}
	kthread_bind(thread, 0);

	wake_up_process(thread);

	return 0;

 fail:
	escsvc_cleanup();
	return err;
}

void
escsvc_cleanup(void)
{
	if (thread != NULL)
		kthread_stop(thread);
	if (ring != NULL)
		kfree(ring);
	if (completions != NULL)
		kfree(completions);
}

int
escsvc_request_async(int svc, void *buffer, u32 length,
		     struct escsvc_completion *comp)
{
	static int next = 0;
	int i;

	spin_lock(&lock);

	i = next;

	if (ring[i].state != 0) {
		spin_unlock(&lock);
		return ESCSVC_ERR_RING_FULL;
	}

	next = (i + 1) % ESCSVC_RING_SIZE;

	completions[i] = comp;

	ring[i].svc = svc;
	ring[i].length = length;
	ring[i].addr = (u32)buffer;

	queue_depth_cur++;
	if (queue_depth_cur > queue_depth_max) {
		queue_depth_max = queue_depth_cur;
	}

	wmb();
	ring[i].state = 1;

	spin_unlock(&lock);

	wake_up(&escsvc_wait);

	return 0;
}
EXPORT_SYMBOL_GPL(escsvc_request_async);

struct escsvc_wait_context {
	struct task_struct *thread;
	int error;
	int done;
	struct escsvc_completion comp;
};

static void
escsvc_sync_done(struct escsvc_completion *comp, int err)
{
	struct escsvc_wait_context *ctx =
	    container_of(comp, struct escsvc_wait_context, comp);

	ctx->error = err;
	ctx->done = 1;
	wake_up_process(ctx->thread);
}

static inline void
init_escsvc_completion(struct escsvc_completion *comp,
			  escsvc_callback_t *callback)
{
	comp->callback = callback;
}

/* Queue request in the ring buffer for execution and wait for response.
 * Returns non-zero value if the function fails to queue the request.
 * Note that the function will return 0 if it was able to queue the request but an error is found later.
 * Any error inside the handler will be returned via the request object.
 */
int escsvc_request_sync(int svc, void *buffer, u32 length)
{
	int err;

	struct escsvc_wait_context ctx = { current, -1, 0, };

	init_escsvc_completion(&ctx.comp, escsvc_sync_done);

	err = escsvc_request_async(svc, buffer, length, &ctx.comp);
	if (err < 0) {
		printk(KERN_INFO "failed async escsvc!\n");
		return err;
	}
	// Avoid a race condition in the the hypercall completing before the
	// context goes to sleep.
	for (;;) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		if (ctx.done) {
			break;
		}
		schedule();
	}
	set_current_state(TASK_RUNNING);

	return 0;
}

// The following lines are needed by Emacs to maintain the format the file started in.
// Local Variables:
// c-basic-offset: 8
// tab-width: 8
// indent-tabs-mode: t
// End:
