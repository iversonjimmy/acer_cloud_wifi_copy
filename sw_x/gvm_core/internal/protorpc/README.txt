ProtoRPC

ProtoRPC is Acer Cloud Technology's proprietary mechanism for performing RPC with Protocol Buffers.

Note that the open source Protocol Buffers project doesn't provide an actual RPC implementation,
(although it does provide the "service" and "rpc" keywords to assist with generating interfaces).
That is why we needed to implement our own.

See rpc.proto for the format of the request and response messages.
