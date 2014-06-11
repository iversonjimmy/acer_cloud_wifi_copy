#ifndef mdd_iface_h
#define mdd_iface_h

#ifdef __cplusplus
extern "C" {
#endif

#include "mdd_net.h"
#include "mdd_iface_platform.h"

typedef MDDIfaceIfindex_platform_t MDDIfaceIfindex_t;

typedef struct {
    MDDIfaceIfindex_t ifindex;
    MDDNetAddr_t addr;
    MDDNetAddr_t netmask;
} MDDIface_t;

typedef enum {
    MDDIfaceState_Add,
    MDDIfaceState_Remove,
} MDDIfaceState_t;

typedef enum {
    MDDIfaceType_Unknown,
    MDDIfaceType_USB3,
    MDDIfaceType_Ethernet,
    MDDIfaceType_Wifi,
} MDDIfaceType_t;

/// Check if the two interfaces are equal
/// return MDD_TRUE if equal, otherwise MDD_FALSE
int MDDIface_Equal(MDDIface_t *a, MDDIface_t *b);

/// Return a list of interfaces include ip address
/// if device has multiple ip address,
/// then multiple items will be returned in the same index but different ip
int MDDIface_Listall(MDDIface_t **iflist, int *count);

/// Monitor the interface changes,
/// if the interface changed, iface_changed will be called
int MDDIface_Monitoriface(int(*iface_changed)(MDDIface_t iface, MDDIfaceState_t state, void *arg), void *arg);

/// Return the type of the specific interface
MDDIfaceType_t MDDIface_Gettype(MDDIface_t iface);

#ifdef __cplusplus
}
#endif

#endif //mdd_iface_h
