#ifndef __ESC_FUNCS_H__
#define __ESC_FUNCS_H__

#include "esc_data.h"

/* The esc_* functions will simply return -1 if there is an error.  (0 means no error)
 * The exact error is communicated to the caller via a field in the request blob.
 */

int esc_init(esc_init_t *req);
int esc_load_credentials(esc_cred_t *req);
int esc_ticket_import(esc_ticket_t *t);
int esc_ticket_release(esc_resource_rel *req);
int esc_title_import(esc_title_t *t);
int esc_title_release(esc_resource_rel *rel);
int esc_content_import(esc_content_t *req);
int esc_content_import_ind(esc_content_ind_t *req);
int esc_content_release(esc_resource_rel *req);
int esc_block_decrypt(esc_block_t *req);
int esc_ticket_query(esc_ticket_query_t *q);
int esc_title_export(esc_title_export_t *req);
int esc_title_query(esc_title_query_t *q);
int esc_block_encrypt(esc_block_t *blk);
int esc_decrypt(esc_crypt_t *req);
int esc_content_export(esc_content_export_t *req);
int esc_content_query(esc_content_query_t *q);
int esc_gss_create(esc_gss_create_t *req);
int esc_mem_usage(esc_mem_usage_t *req);
int esc_device_idcert_get(esc_device_id_t *req);
int esc_rand_seed_set(esc_randseed_t *rs);

#endif // __ESC_FUNCS_H__
