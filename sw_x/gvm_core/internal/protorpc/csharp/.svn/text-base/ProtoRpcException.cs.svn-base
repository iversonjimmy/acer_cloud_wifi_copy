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

namespace protorpc
{
    [Serializable]
    public class ProtoRpcException : Exception
    {
        public ProtoRpcException()
            : base()
        {
        }

        public ProtoRpcException(string message)
            : base(message)
        {
        }

        public ProtoRpcException(string message, Exception innerException)
            : base(message, innerException)
		{
		}

        protected ProtoRpcException(SerializationInfo info, StreamingContext context)
            : base(info, context)
		{
		}

        public override void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            base.GetObjectData(info, context);
        }
    }
}
