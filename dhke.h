#ifndef DHKE_APPLICATION_H
#define DHKE_APPLICATION_H

#include "ns3/application.h"
#include "ns3/socket.h" 

namespace ns3 
{
    class DhkeApplication : public Application 
    {
        public:
            DhkeApplication();
            ~DhkeApplication();
            static TypeId GetTypeId ();
        private:
            void StartApplication() override;
            void ReceivePacket(Ptr<Socket> socket);
    };
}
#endif