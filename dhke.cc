#include "dhke.h"

#include "ns3/core-module.h"         // For core NS-3 functionalities
#include "ns3/network-module.h"      // For network-related functionalities
#include "ns3/internet-module.h"     // For the Internet stack and socket support
#include "ns3/applications-module.h" // For socket-based applications (like TcpSocket)
#include "ns3/socket.h" 

using std::cout;

namespace ns3 
{
    NS_LOG_COMPONENT_DEFINE("DhkeApplication");
    NS_OBJECT_ENSURE_REGISTERED(DhkeApplication);

    TypeId DhkeApplication::GetTypeId()
    {
        static TypeId tid = TypeId("ns3::DhkeApplication")
                                    .AddConstructor<DhkeApplication>()
                                    .SetParent<Application>();
        return tid;
    }

    
    DhkeApplication::DhkeApplication()
    {
    }
    DhkeApplication::~DhkeApplication()
    {
    }

    void DhkeApplication::StartApplication()
    {

        // Send the public key to the other node
        Ptr<Packet> pkt = Create<Packet>(reinterpret_cast<const uint8_t*>("hello"), 5);
        Ptr<Socket> socket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::TcpSocketFactory"));

        socket->SetRecvCallback(MakeCallback(&DhkeApplication::ReceivePacket, this));


    }


    void DhkeApplication::ReceivePacket(Ptr<Socket> socket)
    {
        LogComponentEnable("DhkeApplication", LOG_LEVEL_INFO);
        NS_LOG_INFO ("Recieved Packet. ");

        Ptr<Packet> packet = socket->Recv();
        uint8_t buffer[1024]; // Pointer in buffer. Allocated buffer data is same size as the packet payload size
        int payloadSize = packet->CopyData(buffer, packet->GetSize()); // Size of payload data in buffer


        if (payloadSize > 0)
        {
            std::string extractedData(reinterpret_cast<const char*>(buffer), payloadSize);
            NS_LOG_INFO ("Extracted Data: ");
            cout << "Extracted Data: ";

        }
        else
        {
            NS_LOG_INFO ("Failure");
            cout << "Failure";
        }
        }


}
