#include "mdd.h"

#include "vpl_thread.h"
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
//#include <pthread.h>
#include "log.h"

static int append_txt_callback(MDDInstance inc, MDDXht txt_xht, void *arg)
{
    int changed = 0;
    if (!MDD_XhtGet(txt_xht, "version")) {
        MDD_XhtSet(txt_xht, "version", "0.0.1");
        changed = 1;
    }
    if (MDD_XhtGet(txt_xht, "dummy")) {
        MDD_XhtDelete(txt_xht, "dummy");
        changed = 1;
    }
    return changed;
}

static void* publish_test_run(void *ptr)
{
    MDDInstance inc = ptr;
    int repeat_interval = 1;
    while (1) {
        int changed = 0;
        VPLThread_Sleep(VPLTIME_FROM_SEC(20));
        LOG_INFO("Start temporary shuwdown service\n");
        MDD_TempShutdownService(inc, 1);
        VPLThread_Sleep(VPLTIME_FROM_SEC(20));
        LOG_INFO("Stop temporary shuwdown service\n");
        MDD_TempShutdownService(inc, 0);
        VPLThread_Sleep(VPLTIME_FROM_SEC(60));
        if (repeat_interval != 60) {
            repeat_interval = repeat_interval * 2;
            if (repeat_interval > 60) {
                repeat_interval = 60;
            }
            changed = 1;
        }
        if (changed) {
            LOG_INFO("Change max publishing repeat interval to %d seconds\n",
                     repeat_interval);
            MDD_SetPublishRepeatInterval(inc, repeat_interval);
        }
    }
    return NULL;
}

int main(int argc, char* argv[])
{
    int rc;
    MDDInstance mddInstance;
    MDDXht txt_xht;
    VPLThread_t publish_test_thread;
    VPLThread_return_t publishTestThreadReturnVal;

    mddInstance = MDD_Init();
    MDD_PublishConnectionType(mddInstance, "ConnType");
    txt_xht = MDD_XhtCreate();
    MDD_XhtSet(txt_xht, "name", "CloudNode");
    MDD_XhtSet(txt_xht, "dummy", "dummy");
    rc = VPLThread_Create(&publish_test_thread,
                          publish_test_run,
                          mddInstance,
                          NULL,
                          "publishTestThread");
    if(rc != 0) {
        LOG_ERROR("VPLThread_Create:%d", rc);
        return -2;
    }

    if (mddInstance != NULL) {
        //MDD_Resolve(i, "_rcs._tcp.local.", resolve_cb, NULL);
        //MDD_Resolve(i, "_http._tcp.local.", resolve_cb, NULL);
        
        //MDD_Publish(i, "CloudNode", "vss", 9999, txt_xht, 2, append_txt_callback, NULL); // repeat publishing every 60 seconds
        MDD_Publish(mddInstance, "CloudNode", "simpletest", 9999, txt_xht, 2, append_txt_callback, NULL); // repeat publishing every 60 seconds
        MDD_Start(mddInstance);
    } else {
        LOG_ERROR("Fail to init mdd\n");
    }
    LOG_INFO("Joining publish_test_thread");
    rc = VPLThread_Join(&publish_test_thread, &publishTestThreadReturnVal);
    if(rc != 0) {
        LOG_ERROR("VPLThread_Join:%d", rc);
        return -3;
    }
    LOG_INFO("publishTestThreadEnded:%p", publishTestThreadReturnVal);
    return 0;
}
