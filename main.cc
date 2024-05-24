/* Author: Adam Azam
Test
*/

#include "dhke.h"

#include "ns3/lte-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/config-store.h"


#include <cfloat>
#include <sstream>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <string.h>
#include <openssl/rsa.h>

using namespace ns3;
using std::cout;


NS_LOG_COMPONENT_DEFINE ("D2DTest");



int main (int argc, char *argv[])
{
  LogComponentEnable("D2DTest", LOG_LEVEL_INFO);



  Time simTime = Seconds (10);
  bool enableNsLogs = false;

  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
  cmd.AddValue ("enableNsLogs", "Enable ns-3 logging (debug builds)", enableNsLogs);
  cmd.Parse (argc, argv);

  Config::SetDefault ("ns3::RrSlFfMacScheduler::Itrp", UintegerValue (0));
  Config::SetDefault ("ns3::RrSlFfMacScheduler::SlGrantSize", UintegerValue (5));

  Config::SetDefault ("ns3::LteEnbNetDevice::DlEarfcn", UintegerValue (100));
  Config::SetDefault ("ns3::LteUeNetDevice::DlEarfcn", UintegerValue (100));
  Config::SetDefault ("ns3::LteEnbNetDevice::UlEarfcn", UintegerValue (18100));
  Config::SetDefault ("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue (50));
  Config::SetDefault ("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue (50));


  // For PSSCH
  Config::SetDefault ("ns3::LteSpectrumPhy::SlDataErrorModelEnabled",
  BooleanValue (true));
  // For PSCCH
  Config::SetDefault ("ns3::LteSpectrumPhy::SlCtrlErrorModelEnabled",
  BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::DropRbOnCollisionEnabled",
  BooleanValue (false));


  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (23.0));
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (30.0));

  //Sidelink bearers activation time
  Time slBearersActivationTime = Seconds (2.0);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();
  // parse again so we can override input file default values via command line
  cmd.Parse (argc, argv);


  if (enableNsLogs)
    {
      LogLevel logLevel = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL);

      LogComponentEnable ("LteUeRrc", logLevel);
      LogComponentEnable ("LteUeMac", logLevel);
      LogComponentEnable ("LteSpectrumPhy", logLevel);
      LogComponentEnable ("LteUePhy", logLevel);
      LogComponentEnable ("LteEnbPhy", logLevel);
    }


  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> (); // Creates and Configures LTE devies
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  Ptr<LteSidelinkHelper> proseHelper = CreateObject<LteSidelinkHelper> ();
  proseHelper->SetLteHelper (lteHelper);

  lteHelper->SetAttribute ("PathlossModel",
  StringValue ("ns3::Cost231PropagationLossModel"));


  lteHelper->SetAttribute ("UseSidelink", BooleanValue (true));

  lteHelper->SetSchedulerType ("ns3::RrSlFfMacScheduler");


  //Create nodes (eNb + UEs)
  NodeContainer enbNode;
  enbNode.Create (1);
  NS_LOG_INFO ("eNb node id = [" << enbNode.Get (0)->GetId () << "]");
  NodeContainer ueNodes;
  ueNodes.Create (2);
  NS_LOG_INFO ("UE 1 node id = [" << ueNodes.Get (0)->GetId () << "]");
  NS_LOG_INFO ("UE 2 node id = [" << ueNodes.Get (1)->GetId () << "]");



  Ptr<ListPositionAllocator> positionAllocEnb =
  CreateObject<ListPositionAllocator> ();
  positionAllocEnb->Add (Vector (0.0, 0.0, 30.0));
  Ptr<ListPositionAllocator> positionAllocUe1 =
  CreateObject<ListPositionAllocator> ();
  positionAllocUe1->Add (Vector (10.0, 0.0, 1.5));
  Ptr<ListPositionAllocator> positionAllocUe2 =
  CreateObject<ListPositionAllocator> ();
  positionAllocUe2->Add (Vector (-10.0, 0.0, 1.5));
  //Install mobility
  MobilityHelper mobilityeNodeB;
  mobilityeNodeB.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityeNodeB.SetPositionAllocator (positionAllocEnb);
  mobilityeNodeB.Install (enbNode);
  MobilityHelper mobilityUe1;
  mobilityUe1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityUe1.SetPositionAllocator (positionAllocUe1);
  mobilityUe1.Install (ueNodes.Get (0));
  MobilityHelper mobilityUe2;
  mobilityUe2.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityUe2.SetPositionAllocator (positionAllocUe2);
  mobilityUe2.Install (ueNodes.Get (1));


  NetDeviceContainer enbDevs = lteHelper->InstallEnbDevice (enbNode);
  NetDeviceContainer ueDevs = lteHelper->InstallUeDevice (ueNodes);


  Ptr<LteSlEnbRrc> enbSidelinkConfiguration = CreateObject<LteSlEnbRrc> ();
  enbSidelinkConfiguration->SetSlEnabled (true);


  LteRrcSap::SlCommTxResourcesSetup pool;
  pool.setup = LteRrcSap::SlCommTxResourcesSetup::SCHEDULED;
  //BSR timers
  pool.scheduled.macMainConfig.periodicBsrTimer.period =
  LteRrcSap::PeriodicBsrTimer::sf16;
  pool.scheduled.macMainConfig.retxBsrTimer.period = LteRrcSap::RetxBsrTimer::sf640;
  //MCS
  pool.scheduled.haveMcs = true;
  pool.scheduled.mcs = 16;
  //resource pool
  LteSlResourcePoolFactory pfactory;
  pfactory.SetHaveUeSelectedResourceConfig (false); //since we want eNB to schedule
  //Control
  pfactory.SetControlPeriod("sf40");
  pfactory.SetControlBitmap(0x00000000FF); //8 subframes for PSCCH
  pfactory.SetControlOffset(0);
  pfactory.SetControlPrbNum(22);
  pfactory.SetControlPrbStart(0);
  pfactory.SetControlPrbEnd(49);
  pool.scheduled.commTxConfig = pfactory.CreatePool ();
  uint32_t groupL2Address = 255;
  enbSidelinkConfiguration->AddPreconfiguredDedicatedPool (groupL2Address, pool);
  lteHelper->InstallSidelinkConfiguration (enbDevs, enbSidelinkConfiguration);


  //pre-configuration for the UEs
  Ptr<LteSlUeRrc> ueSidelinkConfiguration = CreateObject<LteSlUeRrc> ();

  ueSidelinkConfiguration->SetSlEnabled (true);
  LteRrcSap::SlPreconfiguration preconfiguration;
  ueSidelinkConfiguration->SetSlPreconfiguration (preconfiguration);
  lteHelper->InstallSidelinkConfiguration (ueDevs, ueSidelinkConfiguration);

  //Install the IP stack on the UEs and assign IP address
  InternetStackHelper internet;
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));
  // set the default gateway for the UE
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
  {
    Ptr<Node> ueNode = ueNodes.Get (u);
    // Set the default gateway for the UE
    Ptr<Ipv4StaticRouting> ueStaticRouting =
    ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
    ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  }

  lteHelper->Attach (ueDevs);


  Ptr<Node> ue1 = ueNodes.Get(0);
  Ptr<Node> ue2 = ueNodes.Get(1);


  Ipv4Address ipA = ueIpIface.GetAddress(0);
  Ipv4Address ipB = ueIpIface.GetAddress(1);


  NS_LOG_INFO ("UEa IP address: " << ipA);
  NS_LOG_INFO ("UEb IP address: " << ipB);


  
  Ptr<Packet> packet = Create<Packet>(reinterpret_cast<const uint8_t*>("hello"), 5);
  Ptr<Socket> aSocket = Socket::CreateSocket(ue1, TypeId::LookupByName("ns3::TcpSocketFactory"));
  Ptr<Socket> bSocket = Socket::CreateSocket(ue2, TypeId::LookupByName("ns3::TcpSocketFactory"));



  bSocket->Bind(InetSocketAddress (Ipv4Address::GetAny (), 4477));

  bSocket->SetRecvCallback(MakeCallback(&MyRecvCallback));

  aSocket->Connect(InetSocketAddress (ipA, 4477));
  
  Simulator::ScheduleWithContext (aSocket->GetNode ()->GetId (),
                                  Seconds (1.0), &SendPacket, 
                                  aSocket, packet);


  

  NS_LOG_INFO ("Create sockets.");
  //Receiver socket on n1
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> source = Socket::CreateSocket (ue1, tid);
  Ptr<Socket> recvSink = Socket::CreateSocket (ue2, tid);

  InetSocketAddress local = InetSocketAddress (ipA, 4477);
  InetSocketAddress remote = InetSocketAddress (ipB, 4477);

  source->Bind (local);
  recvSink->Bind (remote);

  source->Connect (remote);



  Ptr<Packet> packet = Create<Packet>(reinterpret_cast<const uint8_t*>("MHcCAQEEIBThkE2XlF6dzvXyPM3uDKrD4xXUWodz8zjNaLu4vFA1oAoGCCqGSM49AwEHoUQDQgAErwelUpEnsP+zNQb+Wh45WUAGzJYtyR5ATJv+rtMmeALpgbx5nDg3bRMzJ5+85TfHHbd+8nG+yIIaxH63ylrf/g=="), 19999);


  

  LogComponentEnable("SimpleSocketExample", LOG_LEVEL_INFO);

  // Create nodes
  NodeContainer nodes;
  nodes.Create(2);


  internet.Install(nodes);

  // Assign IPv4 addresses to nodes
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign(nodes);

  // Create a socket on the first node
  Ptr<Socket> socket = Socket::CreateSocket(nodes.Get(0), TcpSocketFactory::GetTypeId());

  // Define a port number
  uint16_t port = 12345;

  // Bind the socket to a specific port
  InetSocketAddress local = InetSocketAddress(interfaces.GetAddress(0), port);
  socket->Bind(local);

  // Connect the socket to the second node
  InetSocketAddress remote = InetSocketAddress(interfaces.GetAddress(1), port);
  socket->Connect(remote);

  // Create a custom packet
  uint32_t packetSize = 1024; // Adjust packet size as needed
  Ptr<Packet> packet = Create<Packet>(packetSize);

  // Send the packet
  socket->Send(packet);
  


  
  Ptr<DhkeApplication> dhkeApplication1 = CreateObject<DhkeApplication>();
  Ptr<DhkeApplication> dhkeApplication2 = CreateObject<DhkeApplication>();

  Ptr<Node> ue1 = ueNodes.Get(0);
  Ptr<Node> ue2 = ueNodes.Get(1);

  ue1->AddApplication(dhkeApplication1);
  ue2->AddApplication(dhkeApplication2);
  
  dhkeApplication1->SetStartTime(Seconds(1));
  dhkeApplication2->SetStartTime(Seconds(1));

  dhkeApplication1->SetStopTime(Seconds(4));
  dhkeApplication2->SetStopTime(Seconds(4));
  


  Simulator::Stop (simTime);

  Simulator::Run ();
  Simulator::Destroy ();
}