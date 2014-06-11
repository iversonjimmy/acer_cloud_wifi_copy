//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

package com.acer.dx.serviceclient;

import igware.gvm.pb.CcdiRpcClient.CCDIServiceClient;
import igware.protobuf.ProtoRpcException;

public abstract class AbstractCcdiClient {

    abstract protected CCDIServiceClient getCcdiRpcClient() throws ProtoRpcException;

    abstract public boolean isReady() throws ProtoRpcException;
    
    /**
     * Service binding occurs asynchronously; we use this to wait for it.
     */
    public void waitUntilReady() throws ProtoRpcException {

        while (!isReady()) {
            try {
                Thread.sleep(25);
            } catch (InterruptedException e) {
            }
        }
    }
}
