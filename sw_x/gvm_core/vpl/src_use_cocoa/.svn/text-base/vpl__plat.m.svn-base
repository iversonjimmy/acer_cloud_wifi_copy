#include "vpl_plat.h"

#import <UIKit/UIKit.h>

extern VPL_BOOL gInitialized;
#define MAX_BUF_SIZE 256

int VPL_GetDeviceInfo(char** manufacturer_out, char **model_out)
{
    NSString *manufacturer = nil;
    NSString *model = nil;

    char *buf1 = NULL;
    char *buf2 = NULL;

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

    buf1 = malloc(MAX_BUF_SIZE);
    if(buf1 == NULL){
        rv = VPL_ERR_NOMEM;
        goto cleanup;
    }

    //manufacturer = [[UIDevice currentDevice] name];
    manufacturer = @"Apple";
    if(manufacturer){
        BOOL ret = [manufacturer getCString:buf1 maxLength:MAX_BUF_SIZE encoding:NSUTF8StringEncoding];
        if(ret){
            *manufacturer_out = buf1;
        }else{
            rv = VPL_ERR_FAIL;
            goto cleanup;
        }
    }else{
        rv = VPL_ERR_FAIL;
        goto cleanup;
    }

    buf2 = malloc(MAX_BUF_SIZE);
    if(buf2 == NULL){
        rv = VPL_ERR_NOMEM;
        goto cleanup;
    }

    model = [[UIDevice currentDevice] model];
    if(model){
        BOOL ret = [model getCString:buf2 maxLength:MAX_BUF_SIZE encoding:NSUTF8StringEncoding];
        if(ret){
            *model_out = buf2;
        }else{
            rv = VPL_ERR_FAIL;
            goto cleanup;
        }
    }else{
        rv = VPL_ERR_FAIL;
        goto cleanup;
    }


cleanup:

    if(buf1 != NULL && rv != VPL_OK){
        free(buf1);
        buf1 = NULL;
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
    NSString *osversion = nil;

    char *buf1 = NULL;

    int rv = VPL_OK;

    if (osVersion_out == NULL) {
        return VPL_ERR_INVALID;
    }

    *osVersion_out = NULL;

    if (!gInitialized) {
        return VPL_ERR_NOT_INIT;
    }

    buf1 = malloc(MAX_BUF_SIZE);
    if(buf1 == NULL){
        rv = VPL_ERR_NOMEM;
        goto cleanup;
    }

    osversion = [NSString stringWithFormat:@"iOS %@", [UIDevice currentDevice].systemVersion];
    if(osversion){
        BOOL ret = [osversion getCString:buf1 maxLength:MAX_BUF_SIZE encoding:NSUTF8StringEncoding];
        if(ret){
            *osVersion_out = buf1;
        }else{
            rv = VPL_ERR_FAIL;
            goto cleanup;
        }
    }else{
        rv = VPL_ERR_FAIL;
        goto cleanup;
    }


cleanup:

    if(buf1 != NULL && rv != VPL_OK){
        free(buf1);
        buf1 = NULL;
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
