#include "mdd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct DeviceList {
    char *uuid;
    char *ipv4;
    char *ipv6;
    struct DeviceList *prev;
    struct DeviceList *next;
};

struct DeviceList *devices = NULL;

static void resolve_cb(MDDInstance inc, MDDResolve *resolve, void *arg)
{
    char *uuid;
    int updated = 0;

    uuid = MDD_XhtGet(resolve->txt_xht, "UUID");
    if (uuid == NULL) {
        return;
    }

    // remove exist one
    if (resolve->ttl == 0) {
        struct DeviceList *cur = devices;
        struct DeviceList *next = NULL;
        while (cur != NULL) {
            next = cur->next;
            if (!strcmp(uuid, cur->uuid)) {
                if (resolve->ipv4 != NULL) {
                    if (!strcmp(resolve->ipv4, cur->ipv4)) {
                        cur->ipv4 = NULL;
                        updated = 1;
                    }
                }
                if (resolve->ipv6_count > 0) {
                    if (!strcmp(resolve->ipv6[0], cur->ipv6)) {
                        cur->ipv6 = NULL;
                        updated = 1;
                    }
                }

                if (cur->ipv4 == NULL && cur->ipv6 == NULL) {
                   // remove the device
                   if (cur == devices) {
                       devices = cur->next;
                       cur->next->prev = NULL;
                   } else {
                       if (cur->prev != NULL) {
                           cur->prev->next = cur->next;
                       }
                       if (cur->next != NULL) {
                           cur->next->prev = cur->prev;
                       }
                   }
                   free(cur);
                }
                break;
            }
            cur = next;
        }
    } else {
        // update or create new one
        int found = 0;
        struct DeviceList *cur = devices;
        while (cur != NULL) {
            if (!strcmp(uuid, cur->uuid)) {
                if (resolve->ipv4 != NULL) {
                    if (cur->ipv4 != NULL) {
                        if (strcmp(resolve->ipv4, cur->ipv4)) {
                            free(cur->ipv4);
                            cur->ipv4 = strdup(resolve->ipv4);
                            updated = 1;
                        }
                    } else {
                        cur->ipv4 = strdup(resolve->ipv4);
                        updated = 1;
                    }
                }
                if (resolve->ipv6_count > 0) {
                    if (cur->ipv6 != NULL) {
                        if (strcmp(resolve->ipv6[0], cur->ipv6)) {
                            free(cur->ipv6);
                            cur->ipv6 = strdup(resolve->ipv6[0]);
                            updated = 1;
                        }
                    } else {
                        cur->ipv6 = strdup(resolve->ipv6[0]);
                        updated = 1;
                    }
                }
                found = 1;
                break;
            }
            cur = cur->next;
        }

        if (!found) {
            struct DeviceList *new = NULL;
            new = malloc(sizeof(struct DeviceList));
            memset(new, 0, sizeof(struct DeviceList));
            new->uuid = strdup(uuid);
            if (resolve->ipv4 != NULL) {
                new->ipv4 = strdup(resolve->ipv4);
            }
            if (resolve->ipv6_count > 0) {
                new->ipv6 = strdup(resolve->ipv6[0]);
            }
            new->next = devices;
            if (devices != NULL) {
                devices->prev = new;
            }
            devices = new;
            updated = 1;
        }
    }

    if (!updated) {
        return;
    }

    // print results
    printf("----------------------------------------------------------------------------\n");
    printf(" %-24s | %-15s | %-30s\n", "UUID", "IPv4", "IPv6");
    printf("____________________________________________________________________________\n");
    if (devices != NULL) {
        struct DeviceList *cur = devices;
        while (cur != NULL) {
            printf(" %-24s | %-15s | %-30s\n", cur->uuid, cur->ipv4 == NULL ? "" : cur->ipv4, cur->ipv6 == NULL ? "" : cur->ipv6);
            cur = cur->next;
        }
    } else {
        printf("No any device found!\n");
    }
    printf("----------------------------------------------------------------------------\n");
}

int main(int argc, char* argv[])
{
    MDDInstance i = MDD_Init();
    if (i != NULL) {
        MDD_Resolve(i, "_acss._tcp.local.", 10, resolve_cb, NULL); // repeat every 10 seconds
        MDD_Start(i);
    } else {
        printf("Fail to init mdd\n");
    }

    return 0;
}
