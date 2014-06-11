#include "vplu.h"

#ifdef IOS

#include <sys/sysctl.h>
#include <net/if_dl.h>
#include <resolv.h>
#include <unistd.h> // getlogin_r, getuid
#include <sys/types.h> // uid_t
#include <errno.h>
#include <sys/socket.h> // for getMacAddress
#include <net/if.h> // for getMacAddress
#include <sys/ioctl.h> // for getMacAddress

#else

#include <unistd.h> // getlogin_r, getuid
#include <sys/types.h> // uid_t
#include <errno.h>
#include <net/if.h> // for getMacAddress
#include <sys/ioctl.h> // for getMacAddress

#endif

#if defined(LINUX) || defined(__CLOUDNODE__)
#include <openssl/crypto.h>
#include "vpl_th.h"
#include "vpl_thread.h"
#endif

// Cannot be static; we need to access this from sw_x/gvm_core/vpl/src_use_cocoa/vpl__plat.m
VPL_BOOL gInitialized = VPL_FALSE;

static u8 randHardwareInfo[33] = "RAND";

#if defined(LINUX) || defined(__CLOUDNODE__)
static VPL_BOOL gSSLInitialized = VPL_FALSE;
static VPLMutex_t* sslLocks = NULL;
static int sslNumLocks = 0;

static unsigned long sslIdFunction(void)
{
    return VPLDetachableThread_Self();
}

static void sslLockingFunction(int mode, int n, const char* file, int line)
{
    if (mode & CRYPTO_LOCK) {
        VPLMutex_Lock(&sslLocks[n]);
    } else {
        VPLMutex_Unlock(&sslLocks[n]);
    }
}

static int SSL_MultiThread_Init()
{
    int rv = 0;
    int i;

    sslNumLocks = CRYPTO_num_locks();
    sslLocks = (VPLMutex_t*) malloc(sslNumLocks * sizeof(VPLMutex_t));
    if (sslLocks == NULL) {
        VPL_REPORT_FATAL("Failed to allocate %d mutexes", sslNumLocks);
        rv = VPL_ERR_NOMEM;
        goto fail;
    }

    for (i = 0; i < sslNumLocks; i++) {
        rv = VPLMutex_Init(&sslLocks[i]);
        if (rv != VPL_OK) {
            VPL_REPORT_FATAL("Failed to initialize mutex %d", i);
            goto fail;
        }
    }

    CRYPTO_set_id_callback(sslIdFunction);
    CRYPTO_set_locking_callback(sslLockingFunction);

    return rv;

fail:
    if (sslLocks != NULL) {
        free(sslLocks);
        sslLocks = NULL;
    }
    sslNumLocks = 0;

    return rv;
}

static int SSL_MultiThread_Quit()
{
    int i;
    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);

    if (sslLocks != NULL) {
        for (i = 0; i < sslNumLocks; i++) {
            VPLMutex_Destroy(&sslLocks[i]);
        }
        free(sslLocks);
        sslLocks = NULL;
    }
    sslNumLocks = 0;

    return 0;
}
#endif

int VPL_Init(void)
{
    int rv = VPL_OK;
    if (gInitialized) {
        rv = VPL_ERR_IS_INIT;
    } else {
        // Random value, to use as a fallback.
        srand((unsigned int)VPLTime_GetTime());
        size_t i;
        for (i = 4; i < ARRAY_SIZE_IN_BYTES(randHardwareInfo); i++) {
            randHardwareInfo[i] = (char)(50 + (rand() % 64)); // ensure a string of printable ASCII characters
        }
        randHardwareInfo[ARRAY_SIZE_IN_BYTES(randHardwareInfo) - 1] = '\0';
        gInitialized = VPL_TRUE;
    }
#if defined(LINUX) || defined(__CLOUDNODE__)
    if(!gSSLInitialized) {
        rv = SSL_MultiThread_Init();
        if(rv == VPL_OK) {
            gSSLInitialized = VPL_TRUE;
        } else {
            VPL_REPORT_FATAL("SSL_MultiThread_Init failed with error code %d", rv);
        }
    }
#endif
    return rv;
}

VPL_BOOL VPL_IsInit()
{
    if (gInitialized) {
        return VPL_TRUE;
    } else {
        return VPL_FALSE;
    }
}

int VPL_Quit(void)
{
    if (!gInitialized) {
        return VPL_ERR_NOT_INIT;
    }
    gInitialized = VPL_FALSE;
#if defined(LINUX) || defined(__CLOUDNODE__)
    SSL_MultiThread_Quit();
    gSSLInitialized = VPL_FALSE;
#endif
    return VPL_OK;
}

#ifdef IOS
static int
getMacAddress(const char* interfaceName, char* macAddrStr_out, size_t macAddrStrLen)
{
    int rv = -1;
    int                 mgmtInfoBase[6];
    char                *msgBuffer = NULL;
    size_t              length;
    unsigned char       macAddress[6];
    struct if_msghdr    *interfaceMsgStruct;
    struct sockaddr_dl  *socketStruct;
    
    // Setup the management Information Base (mib)
    mgmtInfoBase[0] = CTL_NET;        // Request network subsystem
    mgmtInfoBase[1] = AF_ROUTE;       // Routing table info
    mgmtInfoBase[2] = 0;              
    mgmtInfoBase[3] = AF_LINK;        // Request link layer information
    mgmtInfoBase[4] = NET_RT_IFLIST;  // Request all configured interfaces
    
    // With all configured interfaces requested, get handle index
    if ((mgmtInfoBase[5] = if_nametoindex("en0")) == 0)
    {
        return -1;
    }
    else
    {
        // Get the size of the data available (store in len)
        if (sysctl(mgmtInfoBase, 6, NULL, &length, NULL, 0) < 0) {
            //errorFlag = @"sysctl mgmtInfoBase failure";
            return -1;
        }
        else
        {
            // Alloc memory based on above call
            if ((msgBuffer = malloc(length)) == NULL)
            {
                //errorFlag = @"buffer allocation failure";
                return -1;
            }
            else
            {
                // Get system information, store in buffer
                if (sysctl(mgmtInfoBase, 6, msgBuffer, &length, NULL, 0) < 0)
                {
                    //errorFlag = @"sysctl msgBuffer failure";
                    return -1;
                }
            }
        }
    }
    
    // Map msgbuffer to interface message structure
    interfaceMsgStruct = (struct if_msghdr *) msgBuffer;
    
    // Map to link-level socket structure
    socketStruct = (struct sockaddr_dl *) (interfaceMsgStruct + 1);
    
    // Copy link layer address data in socket structure to an array
    memcpy(&macAddress, socketStruct->sdl_data + socketStruct->sdl_nlen, 6);
    
    snprintf(macAddrStr_out, macAddrStrLen, "%02X%02X%02X%02X%02X%02X",
             macAddress[0], macAddress[1], macAddress[2],
             macAddress[3], macAddress[4], macAddress[5] );
    
    rv = VPL_OK;
    
    return rv;
}
#else
static int
getMacAddress(const char* interfaceName, char* macAddrStr_out, size_t macAddrStrLen)
{
    int rv = -1;

    // Create a socket that we can use for all of our ioctls
    int ioctlSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (ioctlSocket < 0) {
        return -1;
    }

    // The if_nameindex() function shall return an array of if_nameindex structures, one structure per interface.
    // The end of the array is indicated by a structure with an if_index field of zero and an if_name field of NULL.
    // Applications should call if_freenameindex() to release the memory that may be dynamically allocated by this function, after they have finished using it.
    struct if_nameindex* interfaceList = if_nameindex();
    if (interfaceList == NULL) {
        // TODO: can log errno
        rv = -1;
        goto fail_if_nameindex;
    }

    struct if_nameindex* currInterface ; // Ptr to interface name index
    for (currInterface = interfaceList; currInterface->if_index != 0; currInterface++) {

        // Is this the requested interface?
        if (strcmp(currInterface->if_name, interfaceName) != 0) {
            // Nope.
            continue;
        }

        // Get the MAC address for this interface
        struct ifreq sIfReq;
        memset(&sIfReq, 0, sizeof(sIfReq));
        strncpy(sIfReq.ifr_name, currInterface->if_name, IF_NAMESIZE);
        if (ioctl(ioctlSocket, SIOCGIFHWADDR, &sIfReq) != 0) {
            rv = -1;
            goto end;
        }
        // Print it to the string.
        u8* cMacAddr = (u8*)sIfReq.ifr_hwaddr.sa_data;
        snprintf(macAddrStr_out, macAddrStrLen, "%02X%02X%02X%02X%02X%02X",
                cMacAddr[0], cMacAddr[1], cMacAddr[2],
                cMacAddr[3], cMacAddr[4], cMacAddr[5] );
        rv = VPL_OK;
        goto end;
    }

end:
    if_freenameindex(interfaceList);
fail_if_nameindex:
    close(ioctlSocket);
    return rv;
}
#endif

int VPL_GetHwUuid(char** hwUuid_out)
{
    char* buf;
    if (hwUuid_out == NULL) {
        return VPL_ERR_INVALID;
    }
    *hwUuid_out = NULL;
    if (!gInitialized) {
        return VPL_ERR_NOT_INIT;
    }
    {
        int temp_rv;

        char macAddr[32];
        temp_rv = getMacAddress("eth0", macAddr, ARRAY_SIZE_IN_BYTES(macAddr));
        if (temp_rv != 0) {
            VPL_REPORT_WARN("%s failed: %d", "getMacAddress", temp_rv);
            goto plan_b;
        }
#ifdef IOS
        // In iOS 7 and after, the retrieved MAC address will always be 020000000000
        // https://developer.apple.com/library/prerelease/ios/releasenotes/General/WhatsNewIniOS/Articles/iOS7.html#//apple_ref/doc/uid/TP40013162-SW34
        if (strcmp(macAddr, "020000000000") == 0) {
            VPL_REPORT_INFO("%s returns invalid MAC address: %s","getMacAddress", macAddr);
            goto plan_b;
        }
#endif
        
        char hostname[256];
        temp_rv = gethostname(hostname, ARRAY_SIZE_IN_BYTES(hostname));
        if (temp_rv != 0) {
            VPL_REPORT_WARN("%s failed: %d", "gethostname", temp_rv);
            goto plan_b;
        }
        hostname[ARRAY_SIZE_IN_BYTES(hostname) - 1] = '\0';

        // TODO: this always seems to return "(none)" on our Ubuntu VMs
        char domainname[256];
        temp_rv = getdomainname(domainname, ARRAY_SIZE_IN_BYTES(domainname));
        if (temp_rv != 0) {
            VPL_REPORT_WARN("%s failed: %d", "getdomainname", temp_rv);
            goto plan_b;
        }
        domainname[ARRAY_SIZE_IN_BYTES(domainname) - 1] = '\0';

        
        size_t len = strlen(macAddr) + strlen(hostname) + strlen(domainname) + 3;
        buf = malloc(len);
        if (buf == NULL) {
            return VPL_ERR_NOMEM;
        }
        snprintf(buf, len, "%s.%s.%s", macAddr, hostname, domainname);
        goto done;
    }
plan_b:
    {
        buf = malloc(ARRAY_SIZE_IN_BYTES(randHardwareInfo));
        if (buf == NULL) {
            return VPL_ERR_NOMEM;
        }
        memcpy(buf, randHardwareInfo, ARRAY_SIZE_IN_BYTES(randHardwareInfo));
        goto done;
    }
done:
    *hwUuid_out = buf;
    return VPL_OK;
}

void VPL_ReleaseHwUuid(char* hwUuid)
{
    if (hwUuid != NULL) {
        free(hwUuid);
    }
}

#define MAX_OS_USERNAME_LEN  256

int VPL_GetOSUserName(char** osUserName_out)
{
    if (osUserName_out == NULL) {
        return VPL_ERR_INVALID;
    }
    *osUserName_out = NULL;
    if (!gInitialized) {
        return VPL_ERR_NOT_INIT;
    }
    char* buf = malloc(MAX_OS_USERNAME_LEN);
    if (buf == NULL) {
        return VPL_ERR_NOMEM;
    }
    int rc = getlogin_r(buf, MAX_OS_USERNAME_LEN);
    if (rc != 0) {
        if (rc == EINVAL) {
            // TODO: This seems to happen when running from buildbot, but not when running from a TTY.
            VPL_REPORT_WARN("%s returned EINVAL", "getlogin_r");
        } else {
            VPL_REPORT_WARN("%s failed: %d", "getlogin_r", rc);
        }
        free(buf);
        return VPLError_XlatErrno(rc);
    }
    *osUserName_out = buf;
    return VPL_OK;
}

void VPL_ReleaseOSUserName(char* osUserName)
{
    if (osUserName != NULL) {
        free(osUserName);
    }
}

int VPL_GetOSUserId(char** osUserId_out)
{
    if (osUserId_out == NULL) {
        return VPL_ERR_INVALID;
    }
    *osUserId_out = NULL;
    if (!gInitialized) {
        return VPL_ERR_NOT_INIT;
    }
    uid_t uid = getuid();
    int numChars = VPL_snprintf(NULL, 0, FMTu32, uid);
    char* buf = malloc(numChars + 1);
    if (buf == NULL) {
        return VPL_ERR_NOMEM;
    }
    VPL_snprintf(buf, numChars + 1, FMTu32, uid);
    *osUserId_out = buf;
    return VPL_OK;
}

void VPL_ReleaseOSUserId(char* osUserId)
{
    if (osUserId != NULL) {
        free(osUserId);
    }
}

#if defined(LINUX) || defined(__CLOUDNODE__)
#define MAX_BUF_SIZE 256

int VPL_GetDeviceInfo(char** manufacturer_out, char **model_out)
{
    int rv = VPL_OK;

    if (manufacturer_out == NULL) {
        return VPL_ERR_INVALID;
    }
    if (model_out == NULL) {
        return VPL_ERR_INVALID;
    }

    *manufacturer_out = NULL;
    *model_out = NULL;

    if (!gInitialized) {
        return VPL_ERR_NOT_INIT;
    }

    FILE *fd = NULL;
    FILE *fd2 = NULL;
    char *buf = NULL;
    char *buf2 = NULL;
#if defined(__CLOUDNODE__)
    buf = strdup("Acer");
    if(buf == NULL){
        rv = VPL_ERR_NOMEM;
        goto cleanup;
    }
    *manufacturer_out = buf;

    buf2 = strdup("Orbe");
    if(buf2 == NULL){
        rv = VPL_ERR_NOMEM;
        goto cleanup;
    }
    *model_out = buf2;

#else
    // Read /sys/class/dmi/id/sys_vendor
    {
        if(!(fd = fopen("/sys/class/dmi/id/sys_vendor", "r"))) {
            rv = VPL_ERR_FAIL;
            goto cleanup;
        }

        buf = malloc(MAX_BUF_SIZE);
        if(buf == NULL){
            rv = VPL_ERR_NOMEM;
            goto cleanup;
        }

        if(fgets(buf, MAX_BUF_SIZE, fd)){
            char *pos = NULL;
            if((pos=strchr(buf, '\n')) != NULL)
                *pos = '\0';
            *manufacturer_out = buf;
        }else{
            rv = VPL_ERR_FAIL;
            goto cleanup;
        }
    }

    // Read /sys/class/dmi/id/product_name
    {
        if(!(fd2 = fopen("/sys/class/dmi/id/product_name", "r"))) {
            rv = VPL_ERR_FAIL;
            goto cleanup;
        }

        buf2 = malloc(MAX_BUF_SIZE);
        if(buf2 == NULL){
            rv = VPL_ERR_NOMEM;
            goto cleanup;
        }

        if(fgets(buf2, MAX_BUF_SIZE, fd2)){
            char *pos = NULL;
            if((pos=strchr(buf2, '\n')) != NULL)
                *pos = '\0';
            *model_out = buf2;
        }else{
            rv = VPL_ERR_FAIL;
            goto cleanup;
        }
    }

#endif

cleanup:
    if(fd != NULL){
        fclose(fd);
        fd = NULL;
    }
    if(fd2 != NULL){
        fclose(fd2);
        fd2 = NULL;
    }

    if(buf != NULL && rv != VPL_OK){
        free(buf);
        buf = NULL;
        *manufacturer_out = NULL;
    }
    if(buf2 != NULL && rv != VPL_OK){
        free(buf2);
        buf2 = NULL;
        *model_out = NULL;
    }

    return rv;
}

void VPL_ReleaseDeviceInfo(char* manufacturer, char* model)
{
    if (manufacturer != NULL) {
        // should have been allocated via malloc
        free(manufacturer);
        manufacturer = NULL;
    }

    if (model != NULL) {
        // should have been allocated via malloc
        free(model);
        model = NULL;
    }
}

int VPL_GetOSVersion(char** osVersion_out)
{
    int rv = VPL_OK;

    if (osVersion_out == NULL) {
        return VPL_ERR_INVALID;
    }

    *osVersion_out = NULL;

    if (!gInitialized) {
        return VPL_ERR_NOT_INIT;
    }

    FILE *fd = NULL;
    char *buf = NULL;
    // Read /etc/issue
    {
#if defined(__CLOUDNODE__)
        buf = strdup("Orbe");
        if(buf == NULL){
            rv = VPL_ERR_NOMEM;
            goto cleanup; 
        }
        *osVersion_out = buf;
#else
        if(!(fd = fopen("/etc/issue", "r"))) {
            rv = VPL_ERR_FAIL;
            goto cleanup;
        }

        buf = malloc(MAX_BUF_SIZE);
        if(buf == NULL){
            rv = VPL_ERR_NOMEM;
            goto cleanup;
        }

        if(fgets(buf, MAX_BUF_SIZE, fd)){
            char *pos = NULL;
            if((pos=strchr(buf, '\n')) != NULL)
                *pos = '\0';
            *osVersion_out = buf;
        }else{
            rv = VPL_ERR_FAIL;
            goto cleanup;
        }
#endif
    }


cleanup:
    if(fd != NULL){
        fclose(fd);
        fd = NULL;
    }

    if(buf != NULL && rv != VPL_OK){
        free(buf);
        buf = NULL;
        *osVersion_out = NULL;
    }

    return rv;
}

void VPL_ReleaseOSVersion(char* osVersion)
{
    if (osVersion != NULL) {
        // should have been allocated via malloc
        free(osVersion);
        osVersion = NULL;
    }
}

#endif
