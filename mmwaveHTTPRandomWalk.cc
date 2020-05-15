/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2018 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *   Author: Marco Miozzo <marco.miozzo@cttc.es>
 *           Nicola Baldo  <nbaldo@cttc.es>
 *
 *   Modified by: Marco Mezzavilla < mezzavilla@nyu.edu>
 *                         Sourjya Dutta <sdutta@nyu.edu>
 *                         Russell Ford <russell.ford@nyu.edu>
 *                         Menglei Zhang <menglei@nyu.edu>
 */

//Import libraries
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store.h"
#include <ns3/buildings-helper.h>
#include "ns3/internet-stack-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/three-gpp-http-helper.h"
#include "ns3/applications-module.h"
#include "ns3/energy-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/mmwave-helper.h"

//Define namespace
using namespace ns3;
using namespace mmwave;

//Print remaining energy function
//static void PrintCellInfo (liIonSourceHelper)
//{
//   std::cout << "At " << Simulator::Now ().GetSeconds () << " Cell voltage: " << liIonSourceHelper->GetSupplyVoltage () << " V Remaining Capacity: " <<
//   liIonSourceHelper->GetRemainingEnergy () / (3.7 * 3600) << " Ah" << std::endl; 
// 
//  if (!Simulator::IsFinished ())
//        {
//          Simulator::Schedule (Seconds (10),
//                               &PrintCellInfo,
//                               liIonSourceHelper);
//        }
//}

 NS_LOG_COMPONENT_DEFINE ("mmwaveHTTPRandomWalk");
 
//Print sent and received bytes to console as the simulation runs

 void
 ServerConnectionEstablished (Ptr<const ThreeGppHttpServer>, Ptr<Socket>)
 {
   NS_LOG_INFO ("Client has established a connection to the server.");
 }
 
 void
 MainObjectGenerated (uint32_t size)
 {
   NS_LOG_INFO ("Server generated a main object of " << size << " bytes.");
 }
 
 void
 EmbeddedObjectGenerated (uint32_t size)
 {
   NS_LOG_INFO ("Server generated an embedded object of " << size << " bytes.");
 }
 
 void
 ServerTx (Ptr<const Packet> packet)
 {
   NS_LOG_INFO ("Server sent a packet of " << packet->GetSize () << " bytes.");
 }

 void
 ClientRx (Ptr<const Packet> packet, const Address &address)
 {
   NS_LOG_INFO ("Client received a packet of " << packet->GetSize () << " bytes from " << address);
 }

//Calculate total sent and received bytes and print to console as the simulation runs

 void
 ClientMainObjectReceived (Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
 {
   Ptr<Packet> p = packet->Copy ();
   ThreeGppHttpHeader header;
   p->RemoveHeader (header);
   if (header.GetContentLength () == p->GetSize ()
       && header.GetContentType () == ThreeGppHttpHeader::MAIN_OBJECT)
     {
       NS_LOG_INFO ("Client has successfully received a main object of "
                    << p->GetSize () << " bytes.");
     }
   else
     {
       NS_LOG_INFO ("Client failed to parse a main object. ");
     }
 }


 void
 ClientEmbeddedObjectReceived (Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
 {
   Ptr<Packet> p = packet->Copy ();
   ThreeGppHttpHeader header;
   p->RemoveHeader (header);
   if (header.GetContentLength () == p->GetSize ()
       && header.GetContentType () == ThreeGppHttpHeader::EMBEDDED_OBJECT)
     {
       NS_LOG_INFO ("Client has successfully received an embedded object of "
                    << p->GetSize () << " bytes.");
     }
   else
     {
       NS_LOG_INFO ("Client failed to parse an embedded object. ");
     }
 }

//Checking for lost packets as part of the Flow Monitor
 void
 FlowMonitor::CheckForLostPackets (Time maxDelay)
 {
   NS_LOG_FUNCTION (this << maxDelay.GetSeconds ());
   Time now = Simulator::Now ();
 
   for (TrackedPacketMap::iterator iter = m_trackedPackets.begin ();
        iter != m_trackedPackets.end (); )
     {
       if (now - iter->second.lastSeenTime >= maxDelay)
         {
           // packet is considered lost, add it to the loss statistics
           FlowStatsContainerI flow = m_flowStats.find (iter->first.first);
           NS_ASSERT (flow != m_flowStats.end ());
           flow->second.lostPackets++;
 
           // we won't track it anymore
           m_trackedPackets.erase (iter++);
         }
       else
         {
           iter++;
         }
     }
 }

//Main function
int main (int argc, char *argv[])
{
  //Defaults if none given at runtime
  double simTime = 10.0;
  bool useCa = false;

  Config::SetDefault ("ns3::MmWavePhyMacCommon::ResourceBlockNum", UintegerValue (1));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::ChunkPerRB", UintegerValue (72));

  //Command line arguments, overrides defaults if given
  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
  cmd.AddValue ("useCa", "Whether to use carrier aggregation.", useCa);
  cmd.Parse (argc, argv);

   // Required for tx/rx bytes printing functions
   Time::SetResolution (Time::NS);
   LogComponentEnableAll (LOG_PREFIX_TIME);
   LogComponentEnable ("mmwaveHTTPRandomWalk", LOG_INFO);

  // Parse again so you can override default values from the command line
  cmd.Parse (argc, argv);

  //If user carrier aggregation is set to true via the command line...
  if (useCa)
    {
      Config::SetDefault ("ns3::MmWaveHelper::UseCa",BooleanValue (useCa));
      Config::SetDefault ("ns3::MmWaveHelper::NumberOfComponentCarriers",UintegerValue (2));
      Config::SetDefault ("ns3::MmWaveHelper::EnbComponentCarrierManager",StringValue ("ns3::MmWaveRrComponentCarrierManager"));
    }

  //Creating the mmwavehelper object
  Ptr<MmWaveHelper> ptr_mmWave = CreateObject<MmWaveHelper> ();

  //and then initialising it
  ptr_mmWave->Initialize ();

  // Create Nodes: 2eNodeB and 2 UE
  NodeContainer enbNodes;
  NodeContainer clientServerNodes;
  enbNodes.Create (1);
  clientServerNodes.Create (2);

  // Install Mobility Model
  MobilityHelper mobility;
    //set non moving enb nodes
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (enbNodes);
  BuildingsHelper::Install (enbNodes);
    //set randomly walking ue nodes
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
    "MinX", DoubleValue (1.0),
    "MinY", DoubleValue (1.0),
    "DeltaX", DoubleValue (5.0),
    "DeltaY", DoubleValue (5.0),
    "GridWidth", UintegerValue (3),
    "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
    "Mode", StringValue ("Time"), //time or distance mode
    "Time", StringValue ("2s"), //change current direction and speed after this delay
    "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"), //set constant speed of walk
    "Bounds", RectangleValue (Rectangle (-50.0, 50.0, -50.0, 50.0)));
  mobility.Install (clientServerNodes.Get(0));
  BuildingsHelper::Install (clientServerNodes.Get(0));

 // Create Devices and install them in the Nodes (eNB and UE)
  NetDeviceContainer enbDevs = ptr_mmWave->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueDevs= ptr_mmWave->InstallUeDevice (clientServerNodes.Get (0));

  //Attach ue to enb
  ptr_mmWave->AttachToClosestEnb (ueDevs, enbDevs.Get (0));

  // Activate a data radio bearer
  enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  EpsBearer bearer (q);
  ptr_mmWave->ActivateDataRadioBearer (ueDevs, bearer);

   //Create P2P link
   PointToPointHelper pointToPoint;
   //Set P2P attributes
   pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
   pointToPoint.SetChannelAttribute ("Delay", StringValue ("5ms"));
   //install on client/server nodes
   NetDeviceContainer clientServerDevs;
   clientServerDevs = pointToPoint.Install (clientServerNodes);
  
   //Use the internet stack helper and install on the ue nodes
   InternetStackHelper internet;
   internet.Install (clientServerNodes);
  
   //Set base IPv4 addresses
   Ipv4AddressHelper address;
   address.SetBase ("10.1.1.0", "255.255.255.0");
   //assign these addresses to the client/server devices
   Ipv4InterfaceContainer interfaces = address.Assign (clientServerDevs);
   Ipv4Address serverAddress = interfaces.GetAddress (1);
  
   // Create HTTP server helper
   ThreeGppHttpServerHelper serverHelper (serverAddress);
  
   // Install HTTP server
   ApplicationContainer serverApps = serverHelper.Install (clientServerNodes.Get (1));
   Ptr<ThreeGppHttpServer> httpServer = serverApps.Get (0)->GetObject<ThreeGppHttpServer> ();

   // Example of connecting to the trace sources for server
   httpServer->TraceConnectWithoutContext ("ConnectionEstablished", MakeCallback (&ServerConnectionEstablished));
   httpServer->TraceConnectWithoutContext ("MainObject", MakeCallback (&MainObjectGenerated));
   httpServer->TraceConnectWithoutContext ("EmbeddedObject", MakeCallback (&EmbeddedObjectGenerated));
   httpServer->TraceConnectWithoutContext ("Tx", MakeCallback (&ServerTx));

   // Setup HTTP variables for the server
   PointerValue varPtr;
   httpServer->GetAttribute ("Variables", varPtr);
   Ptr<ThreeGppHttpVariables> httpVariables = varPtr.Get<ThreeGppHttpVariables> ();
   httpVariables->SetMainObjectSizeMean (102400); // 100kB - mean of the main object sizes in bytes
   httpVariables->SetMainObjectSizeStdDev (40960); // 40kB - standard deviation of main object sizes in bytes
  
   // Create HTTP client helper
   ThreeGppHttpClientHelper clientHelper (serverAddress);
  
   // Install HTTP client
   ApplicationContainer clientApps = clientHelper.Install (clientServerNodes.Get (0));
   Ptr<ThreeGppHttpClient> httpClient = clientApps.Get (0)->GetObject<ThreeGppHttpClient> ();

  // Example of connecting to the trace sources for client
  httpClient->TraceConnectWithoutContext ("RxMainObject", MakeCallback (&ClientMainObjectReceived));
  httpClient->TraceConnectWithoutContext ("RxEmbeddedObject", MakeCallback (&ClientEmbeddedObjectReceived));
  httpClient->TraceConnectWithoutContext ("Rx", MakeCallback (&ClientRx));

//V2
/*
   // energy source //
   BasicEnergySourceHelper basicSourceHelper;
   // configure energy source
   basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (1.0));
   // install source
   EnergySourceContainer sources;
   sources = basicSourceHelper.Install (clientServerNodes.Get(0));
   // device energy model //
   LiIonEnergySourceHelper liIonSourceHelper;
   liIonSourceHelper.Set("LiIonEnergySourceInitialEnergyJ",
                          DoubleValue(35000.00));
   liIonSourceHelper.Set("InitialCellVoltage",
                          DoubleValue(3.7));
   liIonSourceHelper.Set("PeriodicEnergyUpdateInterval",
                          TimeValue (Seconds(1.0)));
*/

//   DeviceEnergyModelContainer deviceModels = liIonSourceHelper.Install (ueDevs, sources);

   //3000 mAh and 3.7V is average mobile phone

//  PrintCellInfo (liIonSourceHelper);

//mmwave tracing ALL LAYERS
ptr_mmWave->EnableTraces (); //creates Dl* and Ul* files

//Uncomment for specific mmwave LAYER tracing
//ptr_Helper->EnablePhyTraces ();
//ptr_Helper->EnableMacTraces ();
//ptr_Helper->EnableRlcTraces ();
//ptr_Helper->EnablePdcpTraces ();

//P2P tracing
AsciiTraceHelper ascii;
pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("ASCIImmwaveHTTPRandomWalk.tr")); //ascii
pointToPoint.EnablePcapAll ("PCAPmmwaveHTTPRandomWalk"); //pcap

// Flow monitor
Ptr<FlowMonitor> flowMonitor;
FlowMonitorHelper flowHelper;
flowMonitor = flowHelper.InstallAll();
//Specifying histogram bin widths for delay, jitter and packet size
flowMonitor->SetAttribute("DelayBinWidth", DoubleValue(0.001));
flowMonitor->SetAttribute("JitterBinWidth", DoubleValue(0.001));
flowMonitor->SetAttribute("PacketSizeBinWidth", DoubleValue(20));

//Running and Stopping simulation
Simulator::Stop (Seconds (simTime));
Simulator::Run ();

//Callback to class, checks for packets that appear to be lost
flowMonitor->CheckForLostPackets();

//Only show flow, transferred bytes, received bytes and throughput
Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());
std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats ();
for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
{
Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
	std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
	std::cout << " Tx Bytes: " << i->second.txBytes << "\n";
	std::cout << " Rx Bytes: " << i->second.rxBytes << "\n";
	std::cout << " Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024 << " Mbps\n";
	std::cout << " Delay: " << i->second.delaySum << "\n";
	std::cout << " Lost Packets: " << i->second.lostPackets << "\n";
}

//Flow monitor file generation
flowMonitor->SerializeToXmlFile("FlowMonitormmwaveHTTPRandomWalk.xml", true, true); //histograms and probes enabled

  Simulator::Destroy ();
  return 0;
}
