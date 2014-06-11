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
    public class AppLayerException : ProtoRpcException
    {
        private int appStatus;

        public AppLayerException()
            : base()
        {
        }

        public AppLayerException(string message)
            : base(message)
        {
        }

        public AppLayerException(string message, Exception innerException)
            : base(message, innerException)
        {
        }

        public AppLayerException(int status)
            : base("AppStatus = " + status.ToString())
        {
            this.appStatus = status;
        }

        public AppLayerException(int status, string message)
            : base("AppStatus = " + status.ToString() + ": " + message)
        {
            this.appStatus = status;
        }

        public AppLayerException(int status, string message, Exception innerException)
            : base("AppStatus = " + status.ToString() + ": " + message, innerException)
        {
            this.appStatus = status;
        }

        protected AppLayerException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
            if (info != null)
            {
                this.appStatus = info.GetInt32("appStatus");
            }
        }

        public int Status
        {
            get { return this.appStatus; }
            set { this.appStatus = value; }
        }

        public override void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            base.GetObjectData(info, context);

            if (info != null)
            {
                info.AddValue("appStatus", this.appStatus);
            }
        }
    }
}