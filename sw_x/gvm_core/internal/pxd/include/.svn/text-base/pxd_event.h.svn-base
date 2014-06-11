//
//  Copyright 2013 Acer Cloud Technology
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF Acer Cloud Technology.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF Acer Cloud Technology.
//

#ifndef PXD_EVENT_H
#define PXD_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pxd_event_s pxd_event_t;

extern void pxd_ping        (pxd_event_t *,  const char *);
extern void pxd_check_event (pxd_event_t *,  int, VPLSocket_poll_t *);
extern void pxd_event_wait  (pxd_event_t *,  int64_t);
extern int  pxd_create_event(pxd_event_t **);
extern void pxd_free_event  (pxd_event_t **);

extern VPLSocket_t  pxd_socket(pxd_event_t *);

#ifdef __cplusplus
}
#endif

#endif /* PXD_EVENT_H */
