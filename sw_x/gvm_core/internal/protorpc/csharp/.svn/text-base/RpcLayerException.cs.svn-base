//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
using System;
using System.Runtime.Serialization;
using rpc;

namespace protorpc
{
    [Serializable]
    public class RpcLayerException : ProtoRpcException
    {
        private RpcStatus.Status status;

        public RpcLayerException()
            : base()
        {
        }

        public RpcLayerException(string message)
            : base(message)
        {
        }

        public RpcLayerException(string message, Exception innerException)
            : base(message, innerException)
        {
        }

        public RpcLayerException(RpcStatus.Status status)
            : base("Status = " + status.ToString())
        {
            this.status = status;
        }

        public RpcLayerException(RpcStatus.Status status, string message)
            : base("Status = " + status.ToString() + ": " + message)
        {
            this.status = status;
        }

        public RpcLayerException(RpcStatus.Status status, string message, Exception innerException)
            : base("Status = " + status.ToString() + ": " + message, innerException)
        {
            this.status = status;
        }

        protected RpcLayerException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
            if (info != null)
            {
                //this.status = info.GetString("rpcStatus");
            }
        }

        public RpcStatus.Status Status
        {
            get { return this.status; }
            set { this.status = value; }
        }

        public override void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            base.GetObjectData(info, context);

            if (info != null)
            {
                info.AddValue("rpcStatus", this.status);
            }
        }
    }
}