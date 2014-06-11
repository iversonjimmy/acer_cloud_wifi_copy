using System;
using System.Collections.Generic;
using System.Text;

namespace csharp_ccdi_test
{
    class Program
    {
        static void Main(string[] args)
        {
            try
            {
                ccd.CCDIServiceClient client = new ccd.CCDIServiceClient(new ccd.CCDProtoChannel(), true);
                ulong userId = 0;
                {
                    var request = new ccd.GetSystemStateInput();
                    request.get_users = true;
                    var response = new ccd.GetSystemStateOutput();
                    client.GetSystemState(request, ref response);
                    Console.WriteLine("Num users: {0}", response.users.Count);
                    for (int i = 0; i < response.users.Count; i++)
                    {
                        var user = response.users[i];
                        Console.WriteLine("  Users[{0}]: {1}, {2}", i, user.user_id, user.username);
                        userId = user.user_id;
                    }
                }
                {
                    ccd.ListLinkedDevicesInput request = new ccd.ListLinkedDevicesInput();
                    request.user_id = userId;
                    request.only_use_cache = true;
                    ccd.ListLinkedDevicesOutput response = new ccd.ListLinkedDevicesOutput();
                    client.ListLinkedDevices(request, ref response);
                    Console.WriteLine("Num devices: {0}", response.devices.Count);
                    for (int i = 0; i < response.devices.Count; i++)
                    {
                        ccd.LinkedDeviceInfo device = response.devices[i];
                        Console.WriteLine("  Devices[{0}]: {1}, {2}, {3}", i, device.device_id, device.device_name, device.connection_status.state);
                    }
                }
            }
            catch (protorpc.ProtoRpcException e)
            {
                Console.WriteLine("Failed when calling CCD: " + e.ToString());
            }

            Console.WriteLine("Bye"); // When debugging, probably want to set a breakpoint here.
        }
    }
}
