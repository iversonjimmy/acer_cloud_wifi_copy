// This is the main DLL file.

#include <ccdi_client_named_socket.hpp>
#include <ccdi_client.hpp>
#include <vplex_file.h>

using namespace System;

namespace csharp_ccd_named_pipe_interop {

    public ref class CCDNamedPipeInterop
    {
    private:
        VPLNamedSocketClient_t* namedsock;
    public:
        CCDNamedPipeInterop()
        {
            namedsock = new VPLNamedSocketClient_t();
            int rv = ccdi::client::CCDIClient_OpenNamedSocket(namedsock);
            if (rv != 0) {
                throw gcnew System::Exception("CCDIClient_OpenNamedSocket failed: " + rv);
            }
        }

        void writeBufferToNamedPipe(array<Byte>^ buf)
        {
            int cfd = VPLNamedSocketClient_GetFd(namedsock);
            int bytesSent = 0;
            int size = buf->Length;
            while(bytesSent < size) {
                pin_ptr<Byte> p = &buf[0];   // pin pointer to first element in buf
                u8* np = p;   // pointer to the first element in buf
                int rc = VPLFile_Write((VPLFile_handle_t)cfd, np + bytesSent, size - bytesSent);
                if(rc <= 0) {
                    throw gcnew System::Exception("Error sending CCDI request: " + rc);
                }
                bytesSent += rc;
            }
            if(bytesSent != size) {
                throw gcnew System::Exception("Size mismatch for CCDI request: " + bytesSent + "/" + size);
            }
        }

        void readBufferFromNamedPipe(System::IO::MemoryStream^ outStream)
        {
            array<Byte>^ buf = gcnew array<Byte>(16*1024);
            pin_ptr<Byte> p = &buf[0];
            u8* np = p;   // native pointer to the first element in buf
            int cfd = VPLNamedSocketClient_GetFd(namedsock);
            while(1) {
                ssize_t rc = VPLFile_Read((VPLFile_handle_t)cfd, np, 16*1024);
                if(rc < 0) {
                    throw gcnew System::Exception("Error receiving CCDI response: " + rc);
                }
                if(rc == 0) {
                    // EOF, all done
                    return;
                }
                outStream->Write(buf, 0, rc);
            }
        }
    };
}
