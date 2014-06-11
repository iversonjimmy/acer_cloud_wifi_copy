#include "mdnsd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdtxt.h"
#include "mdd_net.h"
#include "mdd_utils.h"

#include "log.h"

// size of query/publish hashes
#define SPRIME 108
// size of cache hash
#define LPRIME 1009
// brute force garbage cleanup frequency, rarely needed (daily default)
#define GC 86400

// default expire time to check and report the expired cache
#define DEFAULT_PTR_EXPIRE_TIME 600 // 10 minutes

// unicast expire time
#define UNICAST_EXPIRE_TIME 600 // 10 minutes

// max tries for publish
#define MAX_PUBLISH_TRIES 3
// max tries for query
#define MAX_QUERY_TRIES 3
// interval for trying to publish
#define PUBLISH_RETRY_INTERVAL 2 // 2 seconds

/* messy, but it's the best/simplest balance I can find at the moment
Some internal data types, and a few hashes: querys, answers, cached, and records (published, unique and shared)
Each type has different semantics for processing, both for timeouts, incoming, and outgoing I/O
They inter-relate too, like records affect the querys they are relevant to
Nice things about MDNS: we only publish once (and then ask asked), and only query once, then just expire records we've got cached
*/

struct response_addr
{
    int sent;
    MDDSocketAddr_t addr;
    MDDIface_t iface;
    struct timeval last_response;
    struct response_addr *next;
};

struct query
{
    char *name;
    int type;
    MDDIface_t iface;
    unsigned long int nexttry;
    int tries;
    unsigned long int nextforce;
    unsigned int force_rinterval; // seconds
    int (*answer)(mdnsda, void *);
    int (*resolve)(mdnsda, MDDIface_t, void *);
    void *arg;
    int invalid;
    struct query *next, *list;
    struct response_addr *raddr;
};

struct unicast
{
    int id;
    MDDSocketAddr_t to_addr;
    MDDIface_t iface;
    mdnsdr r;
    struct unicast *next;
    // used for repeat publish
    int published;
    int invalid;
    struct timeval last_query;
};

struct cached
{
    struct mdnsda_struct rr;
    MDDIface_t iface;
    struct query *q;
    struct cached *next;
    struct ipinfo *related_ipinfo; // for PTR cache only
    int remote_conn_type; // for PTR cache only
};

struct ipinfo
{
    unsigned long int ip; // A
    unsigned char **ipv6; // AAAA
    unsigned int ipv6_count;
};

struct mdnsdr_struct
{
    struct mdnsda_struct rr;
    char unique; // # of checks performed to ensure
    int tries;
    MDDIface_t iface;
    void (*conflict)(char *, int, void *);
    int (*append_txt)(xht txt_xht, MDDIface_t *iface, void *); // return 1 to apply changes, otherwise return 0
    void *arg;
    int published;
    int invalid;
    struct mdnsdr_struct *next, *list, *rp_list;
};

struct mdnsd_struct
{
    char shutdown;
    unsigned long int expireall, checkqlist, expireptr;
    struct timeval now, sleep, pause, probe, publish, rpublish;
    int class_, frame, expire_ptr_cindex;
    struct cached *cache[LPRIME];
    struct mdnsdr_struct *published[SPRIME], *probing, *a_now, *a_pause, *a_publish, *a_rpublish;
    struct unicast *uanswers;
    struct unicast *ruanswers; // unicasts for repeat publish
    struct query *queries[SPRIME], *qlist;
    // used for publish
    int conflict_detected;
    int start_repeat_publish;
    int temp_shutdown_service; // send out PTR with ttl = 0
    unsigned int current_publish_rinterval; // seconds
    unsigned int max_publish_rinterval; // seconds
    int (*conn_type_resolver)(xht txt_xht, void *arg);
    void *arg;
};

int _c_expire_ptr_related(mdnsd d, struct cached *c_ptr);
void _gc(mdnsd d);

static int _ipinfo_match(struct ipinfo *a, struct ipinfo *b)
{
    unsigned int i = 0, j = 0;
    unsigned int ipv6_match_count = 0;

    if (!a || !b) {
        return 0;
    }

    // match ipv4
    if (a->ip != b->ip) {
        return 0;
    }

    // match ipv6
    if (a->ipv6_count != b->ipv6_count) {
        return 0;
    }

    for (i = 0; i < a->ipv6_count; i++) {
        for (j = 0; j < b->ipv6_count; j++) {
            if (!memcmp(a->ipv6[i], b->ipv6[j], 16)) {
                ipv6_match_count++;
                break;
            }
        }
    }

    if (ipv6_match_count != a->ipv6_count) {
        return 0;
    }

    return 1;
}

static void _free_ipv6_addrs(unsigned char **ipv6, unsigned int ipv6_count)
{
    unsigned int i;

    if (!ipv6 || ipv6_count == 0) {
        return;
    }

    for (i = 0; i < ipv6_count; i++) {
        free(ipv6[i]);
    }
    free(ipv6);
}

static int _namehash(const char *s)
{
    const unsigned char *name = (const unsigned char *)s;
    unsigned long h = 0, g;

    while (*name) {
        /* do some fancy bitwanking on the string */
        h = (h << 4) + (unsigned long)(*name++);
        if ((g = (h & 0xF0000000UL))!=0)
            h ^= (g >> 24);
        h &= ~g;
    }

    return (int)h;
}

// basic linked list and hash primitives
static struct query *_q_next(mdnsd d, struct query *q, const char *host, int type, MDDIface_t *iface)
{
    if (q == 0) {
        q = d->queries[_namehash(host) % SPRIME];
    } else {
        q = q->next;
    }
    for (; q != 0; q = q->next) {
        if (q->type == type && strcmp(q->name, host) == 0 && MDDIface_Equal(&q->iface, iface)) {
            return q;
        }
    }
    return 0;
}

static struct cached *_c_next(mdnsd d, struct cached *c, const char *host, int type, MDDIface_t *iface)
{
    if (c == 0)
        c = d->cache[_namehash(host) % LPRIME];
    else
        c = c->next;
    for (; c != 0; c = c->next)
        if ((type == c->rr.type || type == 255) && strcmp(c->rr.name, host) == 0 && MDDIface_Equal(&c->iface, iface))
            return c;
    return 0;
}

static mdnsdr _r_next(mdnsd d, mdnsdr r, char *host, int type, MDDIface_t *iface)
{
    if (r == 0)
        r = d->published[_namehash(host) % SPRIME];
    else
        r = r->next;
    for (; r != 0; r = r->next)
        if (type == r->rr.type && strcmp(r->rr.name, host) == 0 && (iface == NULL || MDDIface_Equal(&r->iface, iface)))
            return r;
    return 0;
}

static int _rr_len(mdnsda rr)
{
    int len = 12; // name is always compressed (dup of earlier), plus normal stuff
    if (rr->rdata)
        len += rr->rdlen;
    if (rr->rdname)
        len += strlen(rr->rdname); // worst case
    if (rr->ip)
        len += 4;
    if (rr->type == QTYPE_PTR)
        len += 6; // srv record stuff
    return len;
}

static int _a_match(mdnsd d, struct resource *r, mdnsda a, MDDIface_t *iface)
{
    mdnsdr r_find = 0;
    // compares new rdata with known a, painfully
    if (strcmp(r->name, a->name) || r->type != a->type)
        return 0;

    if (r->type == QTYPE_SRV && !strcmp(r->known.srv.name,a->rdname) &&
            a->srv.port == r->known.srv.port &&
            a->srv.weight == r->known.srv.weight &&
            a->srv.priority == r->known.srv.priority)
        return 1;

    if ((r->type == QTYPE_PTR || r->type == QTYPE_NS || r->type == QTYPE_CNAME) &&
            !strcmp(a->rdname,r->known.ns.name))
        return 1;

    // compare all ipv4 addresses on all interface
    if (r->type == QTYPE_A) {
        r_find = _r_next(d, 0, r->name, QTYPE_A, NULL);
        while (r_find) {
            if (r->rdlength == r_find->rr.rdlen && !memcmp(r->rdata, r_find->rr.rdata, r->rdlength))
                return 1;
            r_find = _r_next(d, r_find, r->name, QTYPE_A, NULL);
        }
        return 0;
    }

    // compare all ipv6 addresses on all interface
    if (r->type == QTYPE_AAAA) {
        r_find = _r_next(d, 0, r->name, QTYPE_AAAA, NULL);
        while (r_find) {
            if (r->rdlength == r_find->rr.rdlen && !memcmp(r->rdata, r_find->rr.rdata, r->rdlength))
                return 1;
            r_find = _r_next(d, r_find, r->name, QTYPE_AAAA, NULL);
        }

        // DEBUG only
        {
            LOG_WARN("_a_match: Failed to match ipv6 addr for interface: %d\n", iface->ifindex);
            LOG_WARN("_a_match: Try to match addr: %x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x\n",
                r->rdata[0], r->rdata[1], r->rdata[2], r->rdata[3], r->rdata[4], r->rdata[5], r->rdata[6], r->rdata[7],
                r->rdata[8], r->rdata[9], r->rdata[10], r->rdata[11], r->rdata[12], r->rdata[13], r->rdata[14], r->rdata[15]);
            r_find = _r_next(d, 0, r->name, QTYPE_AAAA, iface);
            if (r_find == NULL) {
                LOG_WARN("_a_match: Does not have any ipv6 addr\n");
            }
            while (r_find) {
                LOG_WARN("_a_match: local ipv6 addr: %x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x\n",
                    r_find->rr.rdata[0], r_find->rr.rdata[1], r_find->rr.rdata[2], r_find->rr.rdata[3],
                    r_find->rr.rdata[4], r_find->rr.rdata[5], r_find->rr.rdata[6], r_find->rr.rdata[7],
                    r_find->rr.rdata[8], r_find->rr.rdata[9], r_find->rr.rdata[10], r_find->rr.rdata[11],
                    r_find->rr.rdata[12], r_find->rr.rdata[13], r_find->rr.rdata[14], r_find->rr.rdata[15]);
                r_find = _r_next(d, r_find, r->name, QTYPE_AAAA, iface);
            }
        }
        return 0;
    }

    // txt is changed dynamicly, so we don't compare the data
    if (r->type == QTYPE_TXT) {
        return 1;
    }

    if (r->rdlength == a->rdlen && !memcmp(r->rdata, a->rdata, r->rdlength))
        return 1;

    return 0;
}

// compare time values easily
static int _tvdiff(struct timeval old, struct timeval new)
{
    int udiff = 0;
    if (old.tv_sec != new.tv_sec)
        udiff = (new.tv_sec - old.tv_sec) * 1000000;
    return (new.tv_usec - old.tv_usec) + udiff;
}

// make sure not already on the list, then insert
static void _r_push(mdnsdr *list, mdnsdr r)
{
    mdnsdr cur;
    for (cur = *list; cur != 0; cur = cur->list) {
        if (cur == r)
            return;
    }
    r->list = *list;
    *list = r;
}

// set this r to probing, set next probe time
//static void _r_probe(mdnsd d, mdnsdr r)
//{
//    //TODO
//}

// force any r out right away, if valid
static void _r_publish(mdnsd d, mdnsdr r)
{
    if (r->unique && r->unique < 5)
        return; // probing already
    r->tries = 0;
    d->publish.tv_sec = d->now.tv_sec;
    d->publish.tv_usec = d->now.tv_usec;
    _r_push(&d->a_publish, r);
}

// send r out asap
static void _r_send(mdnsd d, mdnsdr r)
{
    if (r->tries < MAX_PUBLISH_TRIES) {
        // being published, make sure that happens soon
        d->publish.tv_sec = d->now.tv_sec;
        d->publish.tv_usec = d->now.tv_usec;
        return;
    }
    if (r->unique) {
        // known unique ones can be sent asap
        _r_push(&d->a_now, r);
        return;
    }
    // set d->pause.tv_usec to random 20-120 msec
    d->pause.tv_sec = d->now.tv_sec;
    d->pause.tv_usec = d->now.tv_usec + (d->now.tv_usec % 100) + 20;
    _r_push(&d->a_pause, r);
}

// create generic unicast response struct
static void _u_push(mdnsd d, mdnsdr r, int id, MDDSocketAddr_t *to_addr, MDDIface_t *iface)
{
    struct unicast *u;
    u = (struct unicast *)malloc(sizeof(struct unicast));
    memset(u, 0, sizeof(struct unicast));
    u->r = r;
    u->id = id;
    u->to_addr = *to_addr;
    u->iface = *iface;
    u->next = d->uanswers;
    d->uanswers = u;
}

// add a unicast response struct for repeat publishing
static void _ru_push(mdnsd d, mdnsdr r, int id, MDDSocketAddr_t *to_addr, MDDIface_t *iface)
{
    struct unicast *u;

    // we only care PTR type
    if (r->rr.type != QTYPE_PTR) {
        return;
    }

    u = d->ruanswers;
    while (u) {
        if (!u->invalid &&
            MDDIface_Equal(&u->iface, iface) &&
            u->to_addr.family == to_addr->family &&
            u->to_addr.port == to_addr->port &&
            ((u->to_addr.family == MDDNet_INET && u->to_addr.addr.ip.ipv4 == to_addr->addr.ip.ipv4) ||
            (u->to_addr.family == MDDNet_INET6 && !memcmp(u->to_addr.addr.ip.ipv6, to_addr->addr.ip.ipv6, 16)))) {
            // already in repeat unicast list, update last query time
            MDDTime_Gettimeofday(&u->last_query);
            return;
        }
        u = u->next;
    }

    u = (struct unicast *)malloc(sizeof(struct unicast));
    memset(u, 0, sizeof(struct unicast));
    u->r = r;
    u->id = id;
    u->to_addr = *to_addr;
    u->iface = *iface;
    MDDTime_Gettimeofday(&u->last_query);
    u->next = d->ruanswers;
    d->ruanswers = u;
}

static void _q_raddr(mdnsd d, struct query *q, MDDSocketAddr_t *addr)
{
    struct response_addr *raddr = 0;

    raddr = q->raddr;
    while (raddr) {
        if (raddr->addr.family == addr->family &&
            raddr->addr.port == addr->port &&
            ((raddr->addr.family == MDDNet_INET && raddr->addr.addr.ip.ipv4 == addr->addr.ip.ipv4) ||
            (raddr->addr.family == MDDNet_INET6 && !memcmp(raddr->addr.addr.ip.ipv6, addr->addr.ip.ipv6, 16)))) {
            // already in response list, update last response time
            MDDTime_Gettimeofday(&raddr->last_response);
            return;
        }
        raddr = raddr->next;
    }

    raddr = (struct response_addr *)malloc(sizeof(struct response_addr));
    memset(raddr, 0, sizeof(struct response_addr));
    raddr->addr = *addr;
    MDDTime_Gettimeofday(&raddr->last_response);
    raddr->next = q->raddr;
    q->raddr = raddr;
}

static void _q_reset(mdnsd d, struct query *q)
{
    //struct cached *cur = 0;
    q->nexttry = 0;
    q->tries = 0;
    // Disable re-send query after ttl timeout
    // because cloud node will automatically send publishing repeatly
    /*
    while ((cur = _c_next(d, cur, q->name, q->type, &q->iface))) {
        if (q->nexttry == 0 || cur->rr.ttl - 7 < q->nexttry)
            q->nexttry = cur->rr.ttl - 7;
    }
    if (q->nexttry != 0 && q->nexttry < d->checkqlist)
        d->checkqlist = q->nexttry;
    */
}

static void _q_done(mdnsd d, struct query *q)
{
    // no more query, update all it's cached entries, remove from lists
    struct cached *c = 0;
    struct query *cur;
    int i = _namehash(q->name) % SPRIME;
    while ((c = _c_next(d, c, q->name, q->type, &q->iface)))
        c->q = 0;
    if (d->qlist == q) {
        d->qlist = q->list;
    } else {
        for (cur = d->qlist; cur->list != q; cur = cur->list);
        cur->list = q->list;
    }
    if (d->queries[i] == q) {
        d->queries[i] = q->next;
    } else {
        for (cur = d->queries[i]; cur->next != q; cur = cur->next);
        cur->next = q->next;
    }
    if (q->raddr) {
        struct response_addr *raddr = 0, *del_raddr = 0;
        raddr = q->raddr;
        while (raddr) {
            del_raddr = raddr;
            raddr = raddr->next;
            free(del_raddr);
        }
    }
    free(q->name);
    free(q);
}

static void _r_done(mdnsd d, mdnsdr r)
{
    // buh-bye, remove from hash and free
    mdnsdr cur = 0;
    int i = _namehash(r->rr.name) % SPRIME;
    if (d->published[i] == r) {
        d->published[i] = r->next;
    } else {
        for (cur = d->published[i]; cur && cur->next != r; cur = cur->next);
        if (cur)
            cur->next = r->next;
    }
    free(r->rr.name);
    free(r->rr.rdata);
    free(r->rr.rdname);
    xht_free(r->rr.txt_xht);
    _free_ipv6_addrs(r->rr.ipv6, r->rr.ipv6_count);
    free(r);
}

static void _q_resolve(mdnsd d, struct cached *c_ptr)
{
    struct cached *c_tmp = 0;
    char *domain = c_ptr->rr.rdname;
    char *host = 0;
    unsigned short port = 0;
    mdnsda report = 0;
    int find_ip = 0;

    if (!c_ptr->q || !c_ptr->q->resolve || !c_ptr->related_ipinfo || !domain) {
        return;
    }

    if ((c_tmp = _c_next(d, 0, domain, QTYPE_SRV, &c_ptr->q->iface))) {
        host = c_tmp->rr.rdname;
        port = c_tmp->rr.srv.port;
        if (host != NULL) {
            //ASSERT(port>=0); // Always true due to datatype.
            report = malloc(sizeof(struct mdnsda_struct));
            memset(report, 0, sizeof(struct mdnsda_struct));

            report->ttl = c_ptr->rr.ttl;
            report->name = c_ptr->rr.rdname;
            report->remote_conn_type = c_ptr->remote_conn_type;
            report->rdname = c_tmp->rr.rdname;
            report->srv.priority = c_tmp->rr.srv.priority;
            report->srv.weight = c_tmp->rr.srv.weight;
            report->srv.port = c_tmp->rr.srv.port;
            // try to get txt info
            if ((c_tmp = _c_next(d, 0, domain, QTYPE_TXT, &c_ptr->q->iface))) {
                report->txt_xht = c_tmp->rr.txt_xht;
            }
            // try to get ipv4 address
            if (c_ptr->related_ipinfo->ip != 0) {
                c_tmp = _c_next(d, 0, host, QTYPE_A, &c_ptr->q->iface);
                while (c_tmp) {
                    if (c_tmp->rr.ip == c_ptr->related_ipinfo->ip) {
                        report->ip = c_tmp->rr.ip;
                        find_ip = 1;
                        break;
                    }
                    c_tmp = _c_next(d, c_tmp, host, QTYPE_A, &c_ptr->q->iface);
                }
            }
            // try to get ipv6 address
            if (c_ptr->related_ipinfo->ipv6_count > 0) {
                c_tmp = _c_next(d, 0, host, QTYPE_AAAA, &c_ptr->q->iface);
                while (c_tmp) {
                    unsigned int i = 0, j = 0;
                    unsigned int match = 0;
                    // the cache needs to match all ipv6 address on releated_ipinfo
                    for (i = 0; i < c_tmp->rr.ipv6_count; i++) {
                        for (j = 0; j < c_ptr->related_ipinfo->ipv6_count; j++) {
                            if (!memcmp(c_tmp->rr.ipv6[i], c_ptr->related_ipinfo->ipv6[j], 16)) {
                                match++;
                                break;
                            }
                        }
                    }
                    if (match == c_tmp->rr.ipv6_count) {
                        report->ipv6_count = c_tmp->rr.ipv6_count;
                        report->ipv6 = c_tmp->rr.ipv6;
                        find_ip = 1;
                        break;
                    }
                    c_tmp = _c_next(d, c_tmp, host, QTYPE_AAAA, &c_ptr->q->iface);
                }
            }
            // call the resolve function with this cached entry
            if (find_ip && c_ptr->q->resolve(report, c_ptr->q->iface, c_ptr->q->arg) == -1)
                _q_done(d, c_ptr->q);

            free(report);
        }
    }
}

static int _q_resolve_expire(mdnsd d, struct cached *c_ptr)
{
    int find_other = 0;
    struct cached *c_tmp = NULL;

    if (c_ptr == NULL || c_ptr->rr.type != QTYPE_PTR || c_ptr->related_ipinfo == NULL) {
        return 0;
    }

    c_tmp = _c_next(d, 0, c_ptr->rr.name, c_ptr->rr.type, &c_ptr->iface);
    while (c_tmp) {
        if (!strcmp(c_ptr->rr.rdname, c_tmp->rr.rdname)) {
            // check ipinfo
            if (c_tmp->related_ipinfo != NULL &&
                    _ipinfo_match(c_tmp->related_ipinfo, c_ptr->related_ipinfo)) {
                find_other = 1;
                break;
            }
        }
        c_tmp = _c_next(d, c_tmp, c_ptr->rr.name, c_ptr->rr.type, &c_ptr->iface);
    }

    if (!find_other) {
        // if no other cache exist, report service expired
        _q_resolve(d, c_ptr);
        // also expire the cached related to the PTR
        return _c_expire_ptr_related(d, c_ptr);
    }
    return 0;
}

static void _q_answer(mdnsd d, struct cached *c)
{
    // call the answer function with this cached entry
    if (c->rr.ttl <= (unsigned)d->now.tv_sec)
        c->rr.ttl = 0;
    if (c->q->answer(&c->rr, c->q->arg) == -1)
        _q_done(d, c->q);
}

static void _conflict(mdnsd d, mdnsdr r)
{
    r->conflict(r->rr.name, r->rr.type, r->arg);
    mdnsd_done(d, r);
    d->conflict_detected = 1;
}

static void _c_expire(mdnsd d, struct cached **list)
{
    // expire any old entries in this list
    struct cached *next, *cur = *list, *last = 0;
    int need_gc = 0;
    while (cur != 0) {
        next = cur->next;
        if ((unsigned)d->now.tv_sec >= cur->rr.ttl) {
            if (last)
                last->next = next;
            if (*list == cur)
                *list = next; // update list pointer if the first one expired
            if (cur->q) {
                cur->rr.ttl = 0; // Set ttl to 0 to info the cache expired
                _q_answer(d, cur);
                if (cur->rr.type == QTYPE_PTR) {
                    if (_q_resolve_expire(d, cur)) {
                        if (!need_gc) {
                            need_gc = 1;
                        }
                    }
                }
            }
            free(cur->rr.name);
            free(cur->rr.rdata);
            free(cur->rr.rdname);
            xht_free(cur->rr.txt_xht);
            _free_ipv6_addrs(cur->rr.ipv6, cur->rr.ipv6_count);
            if (cur->related_ipinfo != NULL) {
                if (cur->related_ipinfo->ipv6_count > 0 && cur->related_ipinfo->ipv6 != NULL) {
                    _free_ipv6_addrs(cur->related_ipinfo->ipv6, cur->related_ipinfo->ipv6_count);
                }
                free(cur->related_ipinfo);
            }
            free(cur);
        } else {
            last = cur;
        }
        cur = next;
    }

    if (need_gc) {
        _gc(d);
    }
}

int _c_expire_ptr_related(mdnsd d, struct cached *c_ptr)
{
    struct cached *c_srv = 0;
    struct cached *c_tmp = 0;
    char *domain = 0;
    char *host = 0;

    if (c_ptr == NULL || c_ptr->rr.type != QTYPE_PTR || !c_ptr->related_ipinfo) {
        return 0;
    }

    domain = c_ptr->rr.rdname;

    if (domain == NULL) {
        return 0;
    }

    c_srv = _c_next(d, 0, domain, QTYPE_SRV, &c_ptr->q->iface);
    while (c_srv) {
        host = c_srv->rr.rdname;
        if (host != NULL) {
            // expire ipv4 address
            c_tmp = _c_next(d, 0, host, QTYPE_A, &c_ptr->q->iface);
            while (c_tmp) {
                if (c_tmp->rr.ip == c_ptr->related_ipinfo->ip) {
                    c_tmp->rr.ttl = 0;
                }
                c_tmp = _c_next(d, c_tmp, host, QTYPE_A, &c_ptr->q->iface);
            }
            // expire ipv6 address
            c_tmp = _c_next(d, 0, host, QTYPE_AAAA, &c_ptr->q->iface);
            while (c_tmp) {
                unsigned int i = 0, j = 0;
                int match = 0;
                for (i = 0; i < c_tmp->rr.ipv6_count; i++) {
                    for (j = 0; j < c_ptr->related_ipinfo->ipv6_count; j++) {
                        if (!memcmp(c_tmp->rr.ipv6[i], c_ptr->related_ipinfo->ipv6[i], 16)) {
                            match++;
                            break;
                        }
                    }
                }
                if (match == c_tmp->rr.ipv6_count) {
                    c_tmp->rr.ttl = 0;
                }
                c_tmp = _c_next(d, c_tmp, host, QTYPE_AAAA, &c_ptr->q->iface);
            }
        }
        // should NOT expire srv cache here, all PTR (both ipv4 and ipv6) uses the same SRV cache, and other PTR cache may need it
        //c_srv->rr.ttl = 0;
        c_srv = _c_next(d, c_srv, domain, QTYPE_SRV, &c_ptr->q->iface);
    }
    // should NOT expire txt info here, all PTR (both ipv4 and ipv6) uses the same TXT cache, and other PTR cache may need it
    /*
    c_tmp = _c_next(d, 0, domain, QTYPE_TXT, &c_ptr->q->iface);
    while (c_tmp) {
        c_tmp->rr.ttl = 0;
        c_tmp = _c_next(d, c_tmp, domain, QTYPE_TXT, &c_ptr->q->iface);
    }
    */
    return 1;
}

static void _cal_next_expire_ptr(mdnsd d)
{
    int i;
    struct cached *cur = 0;

    d->expire_ptr_cindex = -1;
    d->expireptr = d->now.tv_sec + DEFAULT_PTR_EXPIRE_TIME;
    for (i = 0; i < LPRIME; i++) {
        if (d->cache[i]) {
            cur = d->cache[i];
            while (cur) {
                if (cur->rr.type == QTYPE_PTR &&
                        cur->rr.ttl < d->expireptr) {
                    d->expire_ptr_cindex = i;
                    d->expireptr = cur->rr.ttl + 1;
                }
                cur = cur->next;
            }
        }
    }
}

// brute force expire any old cached records
void _gc(mdnsd d)
{
    int i;
    for (i = 0; i < LPRIME; i++) {
        if (d->cache[i])
            _c_expire(d, &d->cache[i]);
    }
    d->expireall = d->now.tv_sec + GC;
}

static void _update_ttl(mdnsd d, struct resource *r, struct cached *c)
{
    if (d == NULL || r == NULL || c == NULL) {
        return;
    }

    if (c->rr.type == QTYPE_PTR) {
        // for PTR, add 5 seconds buffer
        c->rr.ttl = d->now.tv_sec + r->ttl + 5;
    } else {
        // for other type, we need to set a buffer that larger than PTR
        // we need to make sure the non-PTR cache will not expire before PTR
        // because when PTR expired, we need to report with other information on those caches like ip address
        c->rr.ttl = d->now.tv_sec + r->ttl + 305;
    }
}

static int _cache(mdnsd d, struct resource *r, MDDIface_t *iface, struct cached **ret_cache)
{
    struct cached *c = 0;
    int i = _namehash(r->name) % LPRIME;
    int sent_answer = 0;

    // skip the invalid resource
    if (r->type == 0 || r->rdata == NULL) {
        return 0;
    }

    if (r->class_ == 32768 + d->class_) {
        // ipv6 support multiple addresses, so we don't flush the saved one
        if (r->type != QTYPE_AAAA) {
            // cache flush
            while ((c = _c_next(d, c, r->name, r->type, iface))) {
                if (r->type == QTYPE_A) {
                    if (c->rr.ip == r->known.a.ip) {
                        c->rr.ttl = 0;
                    }
                } else {
                    c->rr.ttl = 0;
                }
            }
            _c_expire(d, &d->cache[i]);
        } else {
            // FIXME: Disable to support multiple ipv6 address, because we could not handle ttl issue for different ipv6 addresses
            /*
            if ((c = _c_next(d, c, r->name, r->type, iface))) {
                if (c->rr.ipv6 == NULL) {
                    // should not be here, flush the item
                    c->rr.ttl = 0;
                    _c_expire(d, &d->cache[i]);
                } else {
                    int found = 0;
                    int n, i;

                    // check if the address already in the list
                    for (n = 0; n < c->rr.ipv6_count; n++) {
                        for (i = 0; i < 16; i++) {
                            if (c->rr.ipv6[n][i] != r->known.aaaa.ipv6[i])
                                break;
                            if (i == 15)
                                found = 1;
                        }
                        if (found) {
                            break;
                        }
                    }

                    if (!found) {
                        c->rr.ipv6_count++;
                        c->rr.ipv6 = realloc(c->rr.ipv6, sizeof(char *) * c->rr.ipv6_count);
                        c->rr.ipv6[c->rr.ipv6_count - 1] = malloc(sizeof(char) * 16);
                        memcpy(c->rr.ipv6[c->rr.ipv6_count - 1], r->known.aaaa.ipv6, 16);
                    }

                    if ((c->q = _q_next(d, 0, r->name, r->type, iface))) {
                        _q_answer(d, c);
                        sent_answer = 1;
                    }
                    return sent_answer;
                }
            }
            */

            // cache flush the old one, need to remove if we want to support multiple ipv6 addresses
            while ((c = _c_next(d, c, r->name, r->type, iface))) {
                unsigned int i = 0;
                for (i = 0; i < c->rr.ipv6_count; i++) {
                    if (!memcmp(c->rr.ipv6[i], r->known.aaaa.ipv6, 16)) {
                        c->rr.ttl = 0;
                    }
                }
            }
            _c_expire(d, &d->cache[i]);
        }
    }

    if (r->ttl == 0) {
        c = 0;
        // set ttl to 0 for the same items in cache
        while ((c = _c_next(d, c, r->name, r->type, iface))) {
            if (_a_match(d, r, &c->rr, iface)) {
                c->rr.ttl = 0;
            }
        }
    }

    c = (struct cached *)malloc(sizeof(struct cached));
    *ret_cache = c;
    memset(c, 0, sizeof(struct cached));
    c->rr.name = MDDUtils_Strdup(r->name);
    c->rr.type = r->type;
    if (r->ttl != 0) {
        //c->rr.ttl = d->now.tv_sec + (r->ttl / 2) + 8; // XXX hack for now, BAD SPEC, start retrying just after half-waypoint, then expire
        _update_ttl(d, r, c);
    } else {
        c->rr.ttl = 0;
    }
    c->rr.rdlen = r->rdlength;
    c->rr.rdata = (unsigned char *)malloc(r->rdlength);
    c->iface = *iface;
    memcpy(c->rr.rdata, r->rdata, r->rdlength);
    switch (r->type) {
    case QTYPE_A:
        c->rr.ip = r->known.a.ip;
        break;
    case QTYPE_NS:
    case QTYPE_CNAME:
    case QTYPE_PTR:
        c->rr.rdname = MDDUtils_Strdup(r->known.ns.name);
        break;
    case QTYPE_TXT:
        c->rr.txt_xht = txt2sd(r->known.txt.txt_raw, r->rdlength);
        break;
    case QTYPE_AAAA:
        c->rr.ipv6_count = 1;
        c->rr.ipv6 = malloc(sizeof(char *));
        c->rr.ipv6[0] = malloc(sizeof(char) * 16);
        memcpy(c->rr.ipv6[0], r->known.aaaa.ipv6, 16);
        break;
    case QTYPE_SRV:
        c->rr.rdname = MDDUtils_Strdup(r->known.srv.name);
        c->rr.srv.port = r->known.srv.port;
        c->rr.srv.weight = r->known.srv.weight;
        c->rr.srv.priority = r->known.srv.priority;
        break;
    }
    c->next = d->cache[i];
    d->cache[i] = c;
    // calculate PTR expired time
    if (c->rr.type == QTYPE_PTR &&
            c->rr.ttl < d->expireptr) {
        d->expire_ptr_cindex = i;
        d->expireptr = c->rr.ttl + 1; // add 1 second buffer
    }
    if ((c->q = _q_next(d, 0, r->name, r->type, iface))) {
        _q_answer(d, c);
        sent_answer = 1;
    }

    return sent_answer;
}

static void _a_copy(struct message *m, mdnsda a)
{
    // copy the data bits only
    if (a->rdata) {
        message_rdata_raw(m, a->rdata, a->rdlen);
        return;
    }
    if (a->ip)
        message_rdata_long(m, a->ip);
    if (a->type == QTYPE_SRV)
        message_rdata_srv(m, a->srv.priority, a->srv.weight, a->srv.port, a->rdname);
    else if (a->rdname)
        message_rdata_name(m, a->rdname);
}

static void _r_append_message(mdnsd d, struct message *m, mdnsdr r)
{
    unsigned long int ttl = r->rr.ttl;

    if (r->rr.type == QTYPE_PTR) {
        if (d->temp_shutdown_service) {
            ttl = 0;
        }
    }

    if (r->unique) {
        message_an(m, r->rr.name, r->rr.type, d->class_ + 32768, ttl);
    } else {
        message_an(m, r->rr.name, r->rr.type, d->class_, ttl);
    }
    _a_copy(m, &r->rr);
}

static void _r_out_ptr(mdnsd d, struct message *m, mdnsdr r)
{
    mdnsdr append_rr = 0;
    char *host_name = 0;
    xht xht = 0;
    int raw_len = 0;

    if (!r || r->rr.type != QTYPE_PTR) {
        return;
    }

    // SRV
    append_rr = _r_next(d, 0, r->rr.rdname, QTYPE_SRV, &r->iface);
    if (!append_rr || !append_rr->rr.rdname) {
        // Could not find any service
        return;
    }
    host_name = append_rr->rr.rdname;
    _r_append_message(d, m, append_rr);

    // try to append TXT
    append_rr = _r_next(d, 0, r->rr.rdname, QTYPE_TXT, &r->iface);
    if (append_rr) {
        if (r->append_txt) {
            xht = txt2sd(append_rr->rr.rdata, append_rr->rr.rdlen);
            if (r->append_txt(xht, &r->iface, r->arg)) {
                // txt changed, replace by new one
                free(append_rr->rr.rdata);
                append_rr->rr.rdata = sd2txt(xht, &raw_len);
                append_rr->rr.rdlen = raw_len;
            }
            xht_free(xht);
        }
        if (append_rr->rr.rdlen > 0) {
            _r_append_message(d, m, append_rr);
        }
    }

    // try to append A
    append_rr = _r_next(d, 0, host_name, QTYPE_A, &r->iface);
    if (append_rr) {
        _r_append_message(d, m, append_rr);
    }

    // try to append AAAA
    append_rr = _r_next(d, 0, host_name, QTYPE_AAAA, &r->iface);
    if (append_rr) {
        _r_append_message(d, m, append_rr);
    }
}

static int _r_out(mdnsd d, struct message *m, mdnsdr *list, MDDIface_t *out_if)
{
    // copy a published record into an outgoing message
    mdnsdr r;
    int ret = 0;
    while ((r = *list) != 0 && message_packet_len(m) + _rr_len(&r->rr) < d->frame) {
        if (out_if->ifindex == 0) {
            *out_if = r->iface;
        } else if (!MDDIface_Equal(out_if, &r->iface)) {
            return ret;
        }
        *list = r->list;
        ret++;
        _r_append_message(d, m, r);
        // when publish ptr, append necessary info
        if (r->rr.type == QTYPE_PTR) {
            _r_out_ptr(d, m, r);
        }
        if (r->rr.ttl == 0)
            _r_done(d, r);
    }
    return ret;
}

mdnsd mdnsd_new(int class_, int frame)
{
    mdnsd d;
    d = (mdnsd)malloc(sizeof(struct mdnsd_struct));
    memset(d, 0, sizeof(struct mdnsd_struct));
    MDDTime_Gettimeofday(&d->now);
    d->expireall = d->now.tv_sec + GC;
    d->expireptr = d->now.tv_sec + DEFAULT_PTR_EXPIRE_TIME;
    d->class_ = class_;
    d->frame = frame;
    return d;
}

void mdnsd_shutdown(mdnsd d)
{
    // shutting down, zero out ttl and push out all records
    int i;
    mdnsdr cur,next;
    d->a_now = 0;
    for (i = 0; i < SPRIME; i++) {
        for (cur = d->published[i]; cur != 0;) {
            next = cur->next;
            cur->rr.ttl = 0;
            cur->list = d->a_now;
            d->a_now = cur;
            cur = next;
        }
    }
    d->shutdown = 1;
}

void mdnsd_flush(mdnsd d)
{
    //TODO
    // set all querys to 0 tries
    // free whole cache
    // set all mdnsdr to probing
    // reset all answer lists
}

void mdnsd_free(mdnsd d)
{
    int i;
    // expire all cache
    for (i = 0; i < LPRIME; i++) {
        if (d->cache[i]) {
            struct cached *cur = d->cache[i];
            while (cur != 0) {
                cur->rr.ttl = 0;
                cur = cur->next;
            }
            _c_expire(d, &d->cache[i]);
        }
    }
    // delete all query
    for (i = 0; i < SPRIME; i++) {
        struct query *cur;
        while ((cur = d->queries[i]) != 0) {
            _q_done(d, cur);
        }
    }
    // delete all publish
    for (i = 0; i < SPRIME; i++) {
        mdnsdr cur;
        while ((cur = d->published[i]) != 0) {
            _r_done(d, cur);
        }
    }
    // delete all unicast publish
    if (d->ruanswers) {
        struct unicast *u, *next;
        u = d->ruanswers;
        while (u) {
            next = u->next;
            free(u);
            u = next;
        }
    }
    free(d);
}

void mdnsd_in(mdnsd d, struct message *m, MDDSocketAddr_t *from_addr, MDDIface_t *from_iface)
{
    int i, j;
    mdnsdr r = 0;
    struct cached *ptr_cache = 0;
    struct cached *cur_cache = 0;
    // saved for ptr related_ipinfo
    unsigned long int ip = 0;
    unsigned int ipv6_count = 0;
    unsigned char **ipv6 = 0;
    int remote_conn_type = 0;

    if (d->shutdown)
        return;

    MDDTime_Gettimeofday(&d->now);

    if (m->header.qr == 0) {
        for (i = 0; i < m->qdcount; i++) {
            // process each query
            if (m->qd[i].class_ != d->class_ || (r = _r_next(d, 0, m->qd[i].name, m->qd[i].type, from_iface)) == 0)
                continue;
            // send the matching unicast reply
            if (MDDNet_Ntohs(from_addr->port) != MDNS_MULTICAST_PORT)
                _u_push(d, r, m->id, from_addr, from_iface);
            for (; r != 0; r = _r_next(d, r, m->qd[i].name, m->qd[i].type, from_iface)) {
                // check all of our potential answers
                if (r->unique && r->unique < 5) {
                    // probing state, check for conflicts
                    for (j = 0; j < m->nscount; j++) {
                        // check all to-be answers against our own
                        if (m->qd[i].type != m->an[j].type || strcmp(m->qd[i].name, m->an[j].name))
                            continue;
                        if (!_a_match(d, &m->an[j], &r->rr, from_iface)) {
                            _conflict(d, r); // this answer isn't ours, conflict!
                            break;
                        }
                    }
                    continue;
                }
                for (j = 0; j < m->ancount; j++) {
                    // check the known answers for this question
                    if (m->qd[i].type != m->an[j].type || strcmp(m->qd[i].name, m->an[j].name))
                        continue;
                    if (_a_match(d, &m->an[j], &r->rr, from_iface))
                        break; // they already have this answer
                }
                if (j == m->ancount) {
                    _r_send(d, r);
                    // save the address for non authority query, used for sending unicast packet
                    if (m->nscount == 0) {
                        _ru_push(d, r, m->id, from_addr, from_iface);
                    }
                }
            }
        }
        return;
    }

    for (i = 0; i < m->ancount; i++) {
        // skip the invalid answer
        if (m->an[i].name == NULL || m->an[i].type == 0) {
            continue;
        }
        // process each answer, check for a conflict, and cache
        if ((r = _r_next(d, 0, m->an[i].name, m->an[i].type, from_iface)) != 0 && r->unique && _a_match(d, &m->an[i], &r->rr, from_iface) == 0) {
            // check if the name has been published
            if (r->unique < 5) {
                _conflict(d, r);
            } else {
                LOG_WARN("Conflict detected, name %s, type: %d\n", m->an[i].name, m->an[i].type);
            }
        }

        cur_cache = NULL;
        // check whether needs to resolve the service
        if (_cache(d, &m->an[i], from_iface, &cur_cache)) {
            if (m->an[i].type == QTYPE_PTR) {
                if (ptr_cache) {
                    if (strcmp(m->an[i].name, ptr_cache->rr.name)) {
                        LOG_WARN("Receive two different PTR at the same time, the first PTR will not be reported, name: %s\n", ptr_cache->rr.name);
                    }
                }
                ptr_cache = cur_cache;
            }
        }

        // save ip info for PTR cache
        if (cur_cache != NULL) {
            if (cur_cache->rr.type == QTYPE_A) {
                if (ip != 0) {
                    LOG_DEBUG("Receive multiple A answer in the one message, overwrite previous one\n");
                }
                ip = cur_cache->rr.ip;
            } else if (cur_cache->rr.type == QTYPE_AAAA) {
                if (ipv6_count != 0 || ipv6 != NULL) {
                    LOG_DEBUG("Receive multiple AAAA answer in the one message, overwrite previous one\n");
                }
                ipv6_count = cur_cache->rr.ipv6_count;
                ipv6 = cur_cache->rr.ipv6;
            } else if (cur_cache->rr.type == QTYPE_TXT) {
                if (d->conn_type_resolver) {
                    xht xht = 0;
                    xht = txt2sd(cur_cache->rr.rdata, cur_cache->rr.rdlen);
                    // resolve the connection type
                    remote_conn_type = d->conn_type_resolver(xht, d->arg);
                    xht_free(xht);
                }
            }
        }
    }

    // create related ip info for PTR cache
    if (ptr_cache != NULL) {
        ptr_cache->remote_conn_type = remote_conn_type;
        ptr_cache->related_ipinfo = malloc(sizeof(struct ipinfo));
        memset(ptr_cache->related_ipinfo, 0, sizeof(struct ipinfo));
        if (ip > 0) {
            ptr_cache->related_ipinfo->ip = ip;
        }
        if (ipv6_count > 0 && ipv6 != NULL) {
            unsigned int i = 0;
            ptr_cache->related_ipinfo->ipv6_count = ipv6_count;
            ptr_cache->related_ipinfo->ipv6 = malloc(sizeof(char *) * ipv6_count);
            for (i = 0; i < ipv6_count; i++) {
                ptr_cache->related_ipinfo->ipv6[i] = malloc(sizeof(char) * 16);
                memcpy(ptr_cache->related_ipinfo->ipv6[i], ipv6[i], 16);
            }
        }

        // resolve the service if needed
        if (ptr_cache->q && ptr_cache->q->resolve) {
            if (ptr_cache->rr.ttl != 0) {
                _q_raddr(d, ptr_cache->q, from_addr);
                _q_resolve(d, ptr_cache);
            } else {
                // expire function will call resolve callback before delete the cache
                _c_expire(d, &d->cache[_namehash(ptr_cache->rr.name) % LPRIME]);
            }
        }
    }
}

int mdnsd_out(mdnsd d, struct message *m, MDDSocketAddr_t *to_addr, MDDIface_t *to_iface)
{
    mdnsdr r;
    int ret = 0;
    MDDIface_t out_if;

    MDDTime_Gettimeofday(&d->now);

    memset(&out_if, 0, sizeof(MDDIface_t));
    memset(m, 0, sizeof(struct message));

    // if we're in shutdown, we're done
    if (d->shutdown)
        return ret;

    // defaults, multicast
    to_addr->port = MDDNet_Htons(MDNS_MULTICAST_PORT);
    // default assign ipv4 multicast address for out going interface
    to_addr->family = MDDNet_INET;
    if (MDDNet_Inet_pton(MDDNet_INET, MDNS_MULTICAST_IPV4_ADDR, &to_addr->addr) < 0) {
        LOG_ERROR("Error when convert ipv4 address\n");
        return 0;
    }

    m->header.qr = 1;
    m->header.aa = 1;

    if (d->uanswers) {
        // send out individual unicast answers
        struct unicast *u = d->uanswers;
        d->uanswers = u->next;
        *to_addr = u->to_addr;
        *to_iface = u->iface;
        m->id = u->id;
        message_qd(m, u->r->rr.name, u->r->rr.type, d->class_);
        if (u->r->rr.type == QTYPE_PTR && d->temp_shutdown_service) {
            message_an(m, u->r->rr.name, u->r->rr.type, d->class_, 0);
        } else {
            message_an(m, u->r->rr.name, u->r->rr.type, d->class_, u->r->rr.ttl);
        }
        _a_copy(m, &u->r->rr);
        if (u->r->rr.type == QTYPE_PTR) {
            _r_out_ptr(d, m, u->r);
        }
        free(u);
        return 1;
    }

    // default assign ipv4 multicast address for out going interface
    if (MDDNet_Inet_pton(MDDNet_INET, MDNS_MULTICAST_IPV4_ADDR, &to_addr->addr) < 0) {
        LOG_ERROR("Error when convert ipv4 address\n");
        return 0;
    }

    //printf("OUT: probing %X now %X pause %X publish %X\n",d->probing,d->a_now,d->a_pause,d->a_publish);

    // accumulate any immediate responses
    if (d->a_now)
        ret += _r_out(d, m, &d->a_now, &out_if);

    if (d->a_publish) {
        if (abs(d->publish.tv_sec - d->now.tv_sec) > PUBLISH_RETRY_INTERVAL + 1) {
            // detect abnormal time, maybe system time changed
            // reset the publish to let it can publish now
            LOG_DEBUG("Abnormal time detected, reset the publish time\n");
            d->publish.tv_sec = d->now.tv_sec - 1;
        }
    }

    if (d->a_publish && _tvdiff(d->now, d->publish) <= 0) {
        // check to see if it's time to send the publish retries (and unlink if done)
        mdnsdr next, cur = d->a_publish, last = 0;
        while (cur && message_packet_len(m) + _rr_len(&cur->rr) < d->frame) {
            // remove the invalid items
            if (cur->invalid) {
                next = cur->list;
                if (d->a_publish == cur)
                    d->a_publish = next;
                if (last)
                    last->list = next;
                _r_done(d, cur);
                cur = next;
                continue;
            }

            // check to see if it has been published at this run
            if (cur->published) {
                next = cur->list;
                last = cur;
                cur = next;
                continue;
            }

            // check interface
            if (out_if.ifindex == 0) {
                out_if = cur->iface;
            } else if (!MDDIface_Equal(&out_if, &cur->iface)) {
                break;
            }

            next = cur->list;
            ret++;
            cur->tries++;
            _r_append_message(d, m, cur);
            // when publish ptr, append necessary info
            if (cur->rr.type == QTYPE_PTR) {
                _r_out_ptr(d, m, cur);
            }
            cur->published = 1;
            if (cur->rr.ttl != 0 && cur->tries < MAX_PUBLISH_TRIES) {
                last = cur;
                cur = next;
                continue;
            }

            // remove the publishing
            if (d->a_publish == cur)
                d->a_publish = next;
            if (last)
                last->list = next;

            // insert PTR to repeat publishing list
            if (cur->rr.type == QTYPE_PTR && cur->rr.ttl != 0) {
                cur->published = 0;
                cur->rp_list = d->a_rpublish;
                d->a_rpublish = cur;
            }

            if (cur->rr.ttl == 0) {
                _r_done(d, cur);
            }
            cur = next;
        }

        if (!cur && d->a_publish) {
            // calculate retry timeout
            d->publish.tv_sec = d->now.tv_sec + PUBLISH_RETRY_INTERVAL;
            d->publish.tv_usec = d->now.tv_usec;
            // reset published flag
            cur = d->a_publish;
            while (cur) {
                cur->published = 0;
                cur = cur->list;
            }
        }

        // finish all publish tries, start to publishing periodically
        if (!d->a_publish) {
            d->start_repeat_publish = 1;
            d->rpublish.tv_sec = d->now.tv_sec + d->current_publish_rinterval;
            d->rpublish.tv_usec = d->now.tv_usec;
        }
    }

    if (d->start_repeat_publish) {
        if ((unsigned)abs(d->rpublish.tv_sec - d->now.tv_sec) > d->current_publish_rinterval + 1) {
            // detect abnormal time, maybe system time changed
            // reset the rpublish to let it can publish now
            LOG_DEBUG("Abnormal time detected, reset the rpublish time\n");
            d->rpublish.tv_sec = d->now.tv_sec - 1;
        }
    }

    // check for repeat publishing
    if (d->start_repeat_publish && d->a_rpublish && _tvdiff(d->now, d->rpublish) <= 0) {
        // check to see if it's time to send the publish retries (and unlink if done)
        mdnsdr next, cur = d->a_rpublish, last = 0;
        struct unicast *u = 0;
        while (cur && message_packet_len(m) + _rr_len(&cur->rr) < d->frame) {
            // remove the invalid items
            if (cur->invalid) {
                next = cur->rp_list;
                if (d->a_rpublish == cur)
                    d->a_rpublish = next;
                if (last)
                    last->rp_list = next;
                _r_done(d, cur);
                cur = next;
                continue;
            }

            // check to see if it has been published at this run
            if (cur->published) {
                next = cur->rp_list;
                last = cur;
                cur = next;
                continue;
            }

            // check interface
            if (out_if.ifindex == 0) {
                out_if = cur->iface;
            } else if (!MDDIface_Equal(&out_if, &cur->iface)) {
                break;
            }

            next = cur->rp_list;
            ret++;
            _r_append_message(d, m, cur);
            // when publish ptr, append necessary info
            if (cur->rr.type == QTYPE_PTR) {
                _r_out_ptr(d, m, cur);
            }
            cur->published = 1;

            last = cur;
            cur = next;
        }

        u = d->ruanswers;
        if (!cur && !ret) {
            struct unicast *next, *last = 0;
            while (u) {
                next = u->next;
                if (u->last_query.tv_sec > d->now.tv_sec) {
                    LOG_DEBUG("Abnormal time detect, reset the unicast query time to now\n");
                    u->last_query.tv_sec = d->now.tv_sec;
                }

                if ((d->now.tv_sec - u->last_query.tv_sec) > UNICAST_EXPIRE_TIME) {
                    // timeout occur, invalid the unicast
                    u->invalid = 1;
                }

                if (u->invalid) {
                    if (d->ruanswers == u)
                        d->ruanswers = next;
                    if (last)
                        last->next = next;
                    free(u);
                    u = next;
                    continue;
                }

                if (u->published) {
                    last = u;
                    u = u->next;
                    continue;
                }

                ret++;
                *to_addr = u->to_addr;
                *to_iface = u->iface;
                m->id = u->id;
                _r_append_message(d, m, u->r);
                if (u->r->rr.type == QTYPE_PTR) {
                    _r_out_ptr(d, m, u->r);
                }
                u->published = 1;
                return 1;
            }
        }

        if (!cur && d->a_rpublish && !u) {
            // calculate repeat timeout
            if (d->max_publish_rinterval > 0) {
                d->rpublish.tv_sec = d->now.tv_sec + d->current_publish_rinterval;
                d->rpublish.tv_usec = d->now.tv_usec;
                if (d->current_publish_rinterval < d->max_publish_rinterval) {
                    d->current_publish_rinterval *= 2; // exponential backoff to max interval
                    if (d->current_publish_rinterval > d->max_publish_rinterval)
                        d->current_publish_rinterval = d->max_publish_rinterval;
                }
            } else {
                // stop to publish, clear the list
                d->a_rpublish = 0;
                d->start_repeat_publish = 0;
            }

            // reset published flag
            cur = d->a_rpublish;
            while (cur) {
                cur->published = 0;
                cur = cur->rp_list;
            }
            u = d->ruanswers;
            while (u) {
                u->published = 0;
                u = u->next;
            }
        }
    }

    // check if a_pause is ready
    if (d->a_pause && _tvdiff(d->now, d->pause) <= 0)
        ret += _r_out(d, m, &d->a_pause, &out_if);

    // now process questions
    if (ret) {
        *to_iface = out_if;
        if (out_if.addr.family == MDDNet_INET6) {
            to_addr->family = MDDNet_INET6;
            if (MDDNet_Inet_pton(MDDNet_INET6, MDNS_MULTICAST_IPV6_ADDR, &to_addr->addr) < 0) {
                LOG_ERROR("Error when convert ipv6 address\n");
                return 0;
            }
        }
        return ret;
    }
    m->header.qr = 0;
    m->header.aa = 0;

    if (d->probing && _tvdiff(d->now, d->probe) <= 0) {
        mdnsdr last = 0;
        for (r = d->probing; r != 0;) {
            // remove the invalid items
            if (r->invalid) {
                mdnsdr next = r->list;
                if (d->probing == r)
                    d->probing = r->list;
                else if (last)
                    last->list = r->list;
                _r_done(d, r);
                r = next;
                continue;
            }

            // scan probe list to ask questions and process published
            if (r->unique == 4) {
                // done probing, publish
                mdnsdr next = r->list;
                if (d->probing == r)
                    d->probing = r->list;
                else
                    last->list = r->list;
                r->list = 0;
                r->unique = 5;
                r->published = 0;
                _r_publish(d, r);
                r = next;
                continue;
            }

            if (r->published) {
                last = r;
                r = r->list;
                continue;
            }
            // check interface
            if (out_if.ifindex == 0) {
                out_if = r->iface;
            } else if (!MDDIface_Equal(&out_if, &r->iface)) {
                break;
            }

            message_qd(m, r->rr.name, r->rr.type, d->class_);
            r->published = 1;
            last = r;
            r = r->list;
        }
        for (r = d->probing; r != 0; last = r, r = r->list) {
            if (r->published && MDDIface_Equal(&out_if, &r->iface)) {
                // scan probe list again to append our to-be answers
                r->unique++;
                message_ns(m, r->rr.name, r->rr.type, d->class_, r->rr.ttl);
                _a_copy(m, &r->rr);
                ret++;
            }
        }
        if (ret) {
            *to_iface = out_if;
            if (out_if.addr.family == MDDNet_INET6) {
                to_addr->family = MDDNet_INET6;
                if (MDDNet_Inet_pton(MDDNet_INET6, MDNS_MULTICAST_IPV6_ADDR, &to_addr->addr) < 0) {
                    LOG_ERROR("Error when convert ipv6 address\n");
                    return 0;
                }
            }
            return ret;
        } else {
            // process probes again in the future
            d->probe.tv_sec = d->now.tv_sec;
            d->probe.tv_usec = d->now.tv_usec + 250000;
            // reset published flag
            for (r = d->probing; r != 0; last = r, r = r->list) {
                r->published = 0;
            }
        }
    }

    if (d->checkqlist && (unsigned)d->now.tv_sec >= d->checkqlist) {
        // process qlist for retries or expirations
        struct query *q, *next;
        struct cached *c;
        int cal_nextbest = 0;
        unsigned long int nextbest = 0;

        // ask questions first, track nextbest time
        q = d->qlist;
        while (q != 0) {
            // remove the invalid items
            if (q->invalid) {
                next = q->list;
                _q_done(d, q);
                q = next;
                continue;
            }

            cal_nextbest = 0;
            if ((q->nexttry > 0 &&
                 q->nexttry <= (unsigned)d->now.tv_sec &&
                 q->tries < MAX_QUERY_TRIES) ||
                    (q->nextforce > 0 &&
                     q->nextforce <= (unsigned)d->now.tv_sec))
            {
                // check interface
                if (out_if.ifindex == 0 || MDDIface_Equal(&out_if, &q->iface)) {
                    int unicast = 0;
                    struct response_addr *raddr = 0;
                    out_if = q->iface;
                    message_qd(m, q->name, q->type, d->class_);
                    if (q->raddr) {
                        struct response_addr *next = 0, *last = 0;
                        raddr = q->raddr;
                        while (raddr) {
                            next = raddr->next;

                            if (raddr->last_response.tv_sec > d->now.tv_sec) {
                                LOG_DEBUG("Abnormal time detect, reset the last response time to now\n");
                                raddr->last_response.tv_sec = d->now.tv_sec;
                            }

                            if ((d->now.tv_sec - raddr->last_response.tv_sec) > UNICAST_EXPIRE_TIME) {
                                // timeout occur, invalid the unicast
                                if (raddr == q->raddr) {
                                    q->raddr = next;
                                }
                                if (last) {
                                    last->next = next;
                                }
                                free(raddr);
                                raddr = next;
                                continue;
                            }

                            if (!raddr->sent) {
                                unicast = 1;
                                raddr->sent = 1;
                                *to_addr = raddr->addr;
                                break;
                            }
                            last = raddr;
                            raddr = next;
                        }
                    }

                    if (!unicast) {
                        // unicast finished, reset unicast flag
                        raddr = q->raddr;
                        while (raddr) {
                            raddr->sent = 0;
                            raddr = raddr->next;
                        }
                    } else {
                        *to_iface = out_if;
                        // send out unicast
                        return 1;
                    }
                } else {
                    cal_nextbest = 1;
                }
            } else {
                cal_nextbest = 1;
            }

            if (cal_nextbest) {
                if (q->nexttry > 0 && (nextbest == 0 || q->nexttry < nextbest)) {
                    nextbest = q->nexttry;
                }
                if (q->nextforce > 0 && (nextbest == 0 || q->nextforce < nextbest)) {
                    nextbest = q->nextforce;
                }
            }
            q = q->list;
        }

        // include known answers, update questions
        for (q = d->qlist; q != 0; q = q->list) {
            if ((q->nexttry == 0 || q->nexttry > (unsigned)d->now.tv_sec) &&
                    (q->nextforce == 0 || q->nextforce > (unsigned)d->now.tv_sec)) {
                continue;
            }
            if (q->tries == MAX_QUERY_TRIES) {
                // done retrying, expire and reset
                _c_expire(d, &d->cache[_namehash(q->name) % LPRIME]);
                _q_reset(d, q);
                if (q->nextforce == 0 || q->nextforce > (unsigned)d->now.tv_sec) {
                    continue;
                }
            }
            if (!MDDIface_Equal(&out_if, &q->iface)) {
                continue;
            }
            ret++;

            if (q->nexttry > 0 && q->nexttry <= (unsigned)d->now.tv_sec) {
                q->nexttry = d->now.tv_sec + ++q->tries;
            }
            // check if we are doing force query, force query should not append any answer
            if (q->nextforce > 0 && q->nextforce <= (unsigned)d->now.tv_sec) {
                if (q->force_rinterval > 0) {
                    q->nextforce = d->now.tv_sec + q->force_rinterval;
                } else {
                    q->nextforce = 0;
                }
            } else {
                // if room, add all known good entries
                c = 0;
                while ((c = _c_next(d, c, q->name, q->type, &q->iface)) != 0 &&
                        c->rr.ttl > (unsigned)d->now.tv_sec + 8 &&
                        message_packet_len(m) + _rr_len(&c->rr) < d->frame) {
                    message_an(m, q->name, q->type, d->class_, c->rr.ttl - d->now.tv_sec);
                    _a_copy(m, &c->rr);
                }
            }

            if (q->nexttry > 0 && (nextbest == 0 || q->nexttry < nextbest))
                nextbest = q->nexttry;
            if (q->nextforce > 0 && (nextbest == 0 || q->nextforce < nextbest))
                nextbest = q->nextforce;
        }
        d->checkqlist = nextbest;
        if (ret > 0) {
            *to_iface = out_if;
            if (out_if.addr.family == MDDNet_INET6) {
                to_addr->family = MDDNet_INET6;
                if (MDDNet_Inet_pton(MDDNet_INET6, MDNS_MULTICAST_IPV6_ADDR, &to_addr->addr) < 0) {
                    LOG_ERROR("Error when convert ipv6 address\n");
                    return 0;
                }
            }
        }
    }

    if ((unsigned)d->now.tv_sec >= d->expireptr) {
        if (d->expire_ptr_cindex >= 0 && d->expire_ptr_cindex < LPRIME) {
            if (d->cache[d->expire_ptr_cindex]) {
                _c_expire(d, &d->cache[d->expire_ptr_cindex]);
            }
        }

        // calculate next expire item
        _cal_next_expire_ptr(d);
    }

    if ((unsigned)d->now.tv_sec > d->expireall)
        _gc(d);

    return ret;
}

struct timeval *mdnsd_sleep(mdnsd d)
{
    int sec, usec;
    d->sleep.tv_sec = d->sleep.tv_usec = 0;

    #define TO_SEC while (d->sleep.tv_usec > 1000000) { d->sleep.tv_sec++; d->sleep.tv_usec -= 1000000; }

    // if conflict detected, return immediately
    if (d->conflict_detected) {
        d->conflict_detected = 0;
        return &d->sleep;
    }

    // first check for any immediate items to handle
    if (d->uanswers || d->a_now)
        return &d->sleep;

    MDDTime_Gettimeofday(&d->now);

    if (d->a_pause) {
        // then check for paused answers
        if ((usec = _tvdiff(d->now, d->pause)) > 0)
            d->sleep.tv_usec = usec;
        TO_SEC;
        return &d->sleep;
    }

    if (d->probing) {
        // now check for probe retries
        if ((usec = _tvdiff(d->now, d->probe)) > 0)
            d->sleep.tv_usec = usec;
        TO_SEC;
        return &d->sleep;
    }

    if (d->a_publish) {
        // now check for publish retries
        if ((usec = _tvdiff(d->now, d->publish)) > 0)
            d->sleep.tv_usec = usec;
        TO_SEC;
        return &d->sleep;
    }

    // now calculate the minimum timeout for the following items (compare second only)

    if (d->a_rpublish) {
        // check for repeat publishing
        if ((usec = _tvdiff(d->now, d->rpublish)) > 0) {
            d->sleep.tv_usec = usec;
            TO_SEC;
        } else {
            d->sleep.tv_sec = 0;
            d->sleep.tv_usec = 0;
            return &d->sleep;
        }
    }

    if (d->checkqlist) {
        // also check for queries with known answer expiration/retry
        if ((sec = d->checkqlist - d->now.tv_sec) > 0) {
            if (d->sleep.tv_sec == 0 || sec < d->sleep.tv_sec) {
                d->sleep.tv_sec = sec;
            }
        } else {
            d->sleep.tv_sec = 0;
            d->sleep.tv_usec = 0;
            return &d->sleep;
        }
    }

    // check PTR expired timeout
    if ((sec = d->expireptr - d->now.tv_sec) > 0) {
        if (d->sleep.tv_sec == 0 || sec < d->sleep.tv_sec) {
            d->sleep.tv_sec = sec;
        }
    } else {
        d->sleep.tv_sec = 0;
        d->sleep.tv_usec = 0;
        return &d->sleep;
    }

    // last resort, next gc expiration
    if ((sec = d->expireall - d->now.tv_sec) > 0) {
        if (d->sleep.tv_sec == 0 || sec < d->sleep.tv_sec) {
            d->sleep.tv_sec = sec;
        }
    } else {
        d->sleep.tv_sec = 0;
        d->sleep.tv_usec = 0;
        return &d->sleep;
    }

    return &d->sleep;
}

void mdnsd_query(mdnsd d, const char *host, int type, MDDIface_t iface, unsigned int repeat_interval, int (*answer)(mdnsda a, void *arg), int (*resolve)(mdnsda a, MDDIface_t iface, void *arg), void *arg)
{
    struct query *q;
    struct cached *cur = 0;
    int i = _namehash(host) % SPRIME;
    if (!(q = _q_next(d, 0, host, type, &iface))) {
        if (!answer)
            return;
        q = (struct query *)malloc(sizeof(struct query));
        memset(q, 0, sizeof(struct query));
        q->name = MDDUtils_Strdup(host);
        q->type = type;
        q->iface = iface;
        q->next = d->queries[i];
        q->list = d->qlist;
        d->qlist = d->queries[i] = q;
        while ((cur = _c_next(d, cur, q->name, q->type, &q->iface)))
            cur->q = q; // any cached entries should be associated
        _q_reset(d, q);
        q->nexttry = d->checkqlist = d->now.tv_sec; // new questin, immediately send out
        if (repeat_interval > 0) {
            q->force_rinterval = repeat_interval;
            q->nextforce = d->now.tv_sec + q->force_rinterval;
        }
    } else {
        // the query already exist, update it and send out query immediately
        q->invalid = 0;
        while ((cur = _c_next(d, cur, q->name, q->type, &q->iface)))
            cur->q = q; // any cached entries should be associated
        _q_reset(d, q);
        q->nexttry = d->checkqlist = d->now.tv_sec; // new questin, immediately send out
        if (repeat_interval > 0) {
            q->force_rinterval = repeat_interval;
            q->nextforce = d->now.tv_sec + q->force_rinterval;
        } else {
            q->force_rinterval = 0;
            q->nextforce = 0;
        }

    }
    if (!answer) {
        // no answer means we don't care anymore
        _q_done(d, q);
        return;
    }
    q->answer = answer;
    if (resolve)
        q->resolve = resolve;
    q->arg = arg;
}

void mdnsd_set_conn_type_resolver(mdnsd d, int (*connection_type_resolver)(xht txt_xht, void *arg), void *arg)
{
    d->conn_type_resolver = connection_type_resolver;
    d->arg = arg;
}

void mdnsd_restart_query_interval(mdnsd d, const char *host, int type, MDDIface_t iface)
{
    struct query *q;

    // set the nextforce time to now for the matched queries
    q = d->qlist;
    while (q) {
        if (q->type == type &&
                !strcmp(q->name, host) &&
                MDDIface_Equal(&q->iface, &iface)) {
            q->nextforce = d->now.tv_sec;
        }
        q = q->list;
    }
}

void mdnsd_expire_cache_by_interface(mdnsd d, const char *host, int type, MDDIface_t iface)
{
    struct cached *c;

    if (host == NULL) {
        return;
    }

    c = _c_next(d, 0, host, type, &iface);
    while (c) {
        c->rr.ttl = 0;
        c = _c_next(d, c, host, type, &iface);
    }
    // expire the caches for the interface
    _c_expire(d, &d->cache[_namehash(host) % LPRIME]);
    // reset next expire item
    _cal_next_expire_ptr(d);
}

void mdnsd_remove_query_by_interface(mdnsd d, MDDIface_t removed_iface)
{
    struct query *q;

    // set invalid to the items  which match the interface
    q = d->qlist;
    while (q) {
        if (MDDIface_Equal(&removed_iface, &q->iface)) {
            q->invalid = 1;
        }
        q = q->list;
    }
}

mdnsda mdnsd_list(mdnsd d, char *host, int type, mdnsda last, MDDIface_t iface)
{
    return (mdnsda)_c_next(d, (struct cached *)last, host, type, &iface);
}

void mdnsd_repeat_publish(mdnsd d, unsigned int max_interval)
{
    d->current_publish_rinterval = PUBLISH_RETRY_INTERVAL; // start from retry interval
    d->max_publish_rinterval = max_interval;
}

void mdnsd_reset_all_publish_ttl(mdnsd d, long int ttl)
{
    int i = 0;
    // find all publish and change its ttl
    for (i = 0; i < SPRIME; i++) {
        mdnsdr cur;
        cur = d->published[i];
        while (cur) {
            cur->rr.ttl = ttl;
            cur = cur->next;
        }
    }
}

void mdnsd_restart_repeat_publish(mdnsd d)
{
    // reset the rpublish to now
    d->rpublish.tv_sec = d->now.tv_sec;
    d->rpublish.tv_usec = d->now.tv_usec;
    d->current_publish_rinterval = PUBLISH_RETRY_INTERVAL; // start from retry interval
}

void mdnsd_remove_publish_by_interface(mdnsd d, MDDIface_t removed_iface)
{
    mdnsdr cur;
    struct unicast *u;

    // set invalid to the items which match the interface
    cur = d->a_publish;
    while (cur) {
        if (MDDIface_Equal(&removed_iface, &cur->iface)) {
            cur->invalid = 1;
        }
        cur = cur->list;
    }

    cur = d->a_rpublish;
    while (cur) {
        if (MDDIface_Equal(&removed_iface, &cur->iface)) {
            cur->invalid = 1;
        }
        cur = cur->rp_list;
    }

    cur = d->probing;
    while (cur) {
        if (MDDIface_Equal(&removed_iface, &cur->iface)) {
            cur->invalid = 1;
        }
        cur = cur->list;
    }

    u = d->ruanswers;
    while (u) {
        if (MDDIface_Equal(&removed_iface, &u->iface)) {
            u->invalid = 1;
        }
        u = u->next;
    }
}

void mdnsd_temp_shutdown_service(mdnsd d, int shutdown)
{
    d->temp_shutdown_service = shutdown;
}

int mdnsd_in_temp_shutdown_state(mdnsd d)
{
    return d->temp_shutdown_service;
}

mdnsdr mdnsd_shared(mdnsd d, char *host, int type, long int ttl, MDDIface_t iface, int (*append_txt)(xht txt_xht, MDDIface_t *iface, void *arg), void *arg)
{
    int i = _namehash(host) % SPRIME;
    mdnsdr r;
    r = (mdnsdr)malloc(sizeof(struct mdnsdr_struct));
    memset(r, 0, sizeof(struct mdnsdr_struct));
    r->rr.name = MDDUtils_Strdup(host);
    r->rr.type = type;
    r->rr.ttl = ttl;
    r->iface = iface;
    r->next = d->published[i];
    if (type == QTYPE_PTR) {
        r->append_txt = append_txt;
        r->arg = arg;
    }
    d->published[i] = r;
    return r;
}

mdnsdr mdnsd_unique(mdnsd d, char *host, int type, long int ttl, MDDIface_t iface, void (*conflict)(char *host, int type, void *arg), void *arg)
{
    mdnsdr r;
    r = mdnsd_shared(d, host, type, ttl, iface, NULL, NULL);
    r->conflict = conflict;
    r->arg = arg;
    r->unique = 1;
    _r_push(&d->probing, r);
    d->probe.tv_sec = d->now.tv_sec;
    d->probe.tv_usec = d->now.tv_usec;
    return r;
}

void mdnsd_done(mdnsd d, mdnsdr r)
{
    mdnsdr cur;
    if (r->unique && r->unique < 5) {
        // probing yet, zap from that list first!
        if (d->probing == r) {
            d->probing = r->list;
        } else {
            for (cur = d->probing; cur->list != r; cur = cur->list);
            cur->list = r->list;
        }
        _r_done(d, r);
        return;
    }
    r->rr.ttl = 0;
    _r_send(d, r);
}

void mdnsd_set_raw(mdnsd d, mdnsdr r, char *data, int len)
{
    free(r->rr.rdata);
    r->rr.rdata = (unsigned char *)malloc(len);
    memcpy(r->rr.rdata, data, len);
    r->rr.rdlen = len;
    _r_publish(d, r);
}

void mdnsd_set_host(mdnsd d, mdnsdr r, char *name)
{
    free(r->rr.rdname);
    r->rr.rdname = MDDUtils_Strdup(name);
    _r_publish(d, r);
    d->current_publish_rinterval = PUBLISH_RETRY_INTERVAL; // start from retry interval
}

void mdnsd_set_ip(mdnsd d, mdnsdr r, unsigned long int ip)
{
    r->rr.ip = ip;
    _r_publish(d, r);
}

void mdnsd_set_srv(mdnsd d, mdnsdr r, int priority, int weight, int port, char *name)
{
    r->rr.srv.priority = priority;
    r->rr.srv.weight = weight;
    r->rr.srv.port = port;
    mdnsd_set_host(d, r, name);
}

