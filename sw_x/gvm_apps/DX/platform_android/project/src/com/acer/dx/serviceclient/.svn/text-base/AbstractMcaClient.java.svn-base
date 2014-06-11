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

import igware.cloud.media_metadata.pb.MCAForControlPointClient.MCAForControlPointServiceClient;
import igware.protobuf.ProtoRpcException;

public abstract class AbstractMcaClient {

    abstract protected MCAForControlPointServiceClient getMcaRpcClient() throws ProtoRpcException;

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
