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
#include "ns3/applications-module.h"
#include "ns3/energy-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/mmwave-helper.h"
//#include "ns3/gtk-config-store.h"

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

NS_LOG_COMPONENT_DEFINE ("mmwaveTCPRandomWalk");

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
  double simTime = 30.0;
  bool useCa = false;
  bool useV6 = false;
 
  Address serverAddress;

  //Command line arguments, overrides defaults if given
  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
  cmd.AddValue ("useCa", "Whether to use carrier aggregation.", useCa);
  cmd.AddValue ("useV6", "Whether to use IPv6 or not.", useV6);
  cmd.Parse (argc, argv);
 
  LogComponentEnable ("mmwaveTCPRandomWalk", LOG_INFO);

  //Other default inputs can be gathered from a pre-existing text file and loaded into a future simulation.
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

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
    "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"), //random number generator for speed of walk
    "Bounds", RectangleValue (Rectangle (0.0, 20.0, 0.0, 20.0)));
  mobility.Install (clientServerNodes.Get(0));
  BuildingsHelper::Install (clientServerNodes.Get(0));

 // Create Devices and install them in the Nodes (eNB and UE)
  NetDeviceContainer enbDevs = ptr_mmWave->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueDevs= ptr_mmWave->InstallUeDevice (clientServerNodes.Get (0));

  //Attach ue to enb
  ptr_mmWave->AttachToClosestEnb (ueDevs, enbDevs.Get (0));

  //Whenever a user equipment is being provided with any service,
  //the service has to be associated with a Radio Bearer specifying
  //the configuration for Layer-2 and Physical Layer in order to have
  //its QoS clearly defined.

  // Activate a data radio bearer
  enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  EpsBearer bearer (q);
  ptr_mmWave->ActivateDataRadioBearer (ueDevs, bearer);

  //Create P2P link
  PointToPointHelper pointToPoint;
  //Set P2P attributes
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  //install on client/server nodes
  NetDeviceContainer clientServerDevs;
  clientServerDevs = pointToPoint.Install (clientServerNodes);
  
  //Use the internet stack helper and install on the ue nodes
  InternetStackHelper internet;
  internet.Install (clientServerNodes);
  
  //Assigning IP addresses
//  if (useV6 == false)
  //  {
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (clientServerDevs);
//  serverAddress = Address (i.GetAddress (1));
   // }
 /* else
    {
      Ipv6AddressHelper ipv6;
      ipv6.SetBase ("2001:0000:f00d:cafe::", Ipv6Prefix (64));
      Ipv6InterfaceContainer i6 = ipv6.Assign (clientServerDevs);
      serverAddress = Address(i6.GetAddress (1,1));
    }
*/
  
  // Create a TCP Server on the receiver
  uint16_t port = 50000;
  // Create a packet sink to receive packets from OnOff application
  Address sinkAddress (InetSocketAddress (i.GetAddress (1), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApp = sinkHelper.Install (clientServerNodes.Get(1));
  sinkApp.Start (Seconds (1.0));

  // Create the OnOff applications to send TCP packets to the server
   uint32_t MaxPacketSize = 1024;
//   Time interPacketInterval = Seconds (0.05);
//   uint32_t maxPacketCount = 320;
   OnOffHelper client ("ns3::TcpSocketFactory", sinkAddress);
//   client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
//   client.SetAttribute ("Interval", TimeValue (interPacketInterval));
   client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
   client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
   client.SetAttribute ("DataRate", DataRateValue (DataRate ("10Mbps")));
   client.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
   client.SetAttribute ("MaxBytes", UintegerValue (10000));
   ApplicationContainer clientApp = client.Install (clientServerNodes.Get (0));
   clientApp.Start (Seconds (2.0));


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

   //configuring energy source helper
//   liIonSourceHelper.Set("LiIonEnergySourceInitialEnergyJ", DoubleValue (35000.00)); //Joules
//   liIonSourceHelper.Set("InitialCellVoltage", DoubleValue (3.7)); //ax voltage when fully charged
//   liIonSourceHelper.Set("LiIonEnergyLowBatteryThreshold", DoubleValue (0.10)); //as a fraction of the initial energy
//   liIonSourceHelper.Set("PeriodicEnergyUpdateInterval", TimeValue (Seconds (1.0))); //time between two consectutive periodic energy updates
//   liIonSourceHelper->SetLiIonEnergySourceInitialEnergyJ (35000.00);

//   liIonSourceHelper->SetInitialEnergy (35000.00);
//   liIonSourceHelper->SetInitialSupplyVoltage(3.7);
//   liIonSourceHelper->SetEnergyUpdateInterval (Time (1.0));


   //3000 mAh and 3.7V is average mobile phone

//  PrintCellInfo (liIonSourceHelper);

//mmwave tracing ALL LAYERS
ptr_mmWave->EnableTraces (); //creates Dl* and Ul* files

//mmwave LAYER tracing
//ptr_Helper->EnablePhyTraces ();
//ptr_Helper->EnableMacTraces ();
//ptr_Helper->EnableRlcTraces ();
//ptr_Helper->EnablePdcpTraces ();

//P2P tracing
AsciiTraceHelper ascii;
pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("ASCIImmwaveTCPRandomWalk.tr")); //ascii
pointToPoint.EnablePcapAll ("PCAPmmwaveTCPRandomWalk"); //pcap


// Flow monitor
Ptr<FlowMonitor> flowMonitor;
FlowMonitorHelper flowHelper;
flowMonitor = flowHelper.InstallAll();
//Specifying histogram bin widths for delay, jitter and packet size
flowMonitor->SetAttribute("DelayBinWidth", DoubleValue(0.001));
flowMonitor->SetAttribute("JitterBinWidth", DoubleValue(0.001));
flowMonitor->SetAttribute("PacketSizeBinWidth", DoubleValue(20));


//Running and Stopping simulation
  //Simulator::Stop (Seconds (simTime));
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();

//Callback to class, checks for packets that appear to be lost
flowMonitor->CheckForLostPackets();

//Only show flow, transferred bytes, received bytes and throughput
Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());

FlowMonitor::FlowStatsContainer stats = flowMonitor->GetFlowStats ();

//std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats ();
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
flowMonitor->SerializeToXmlFile("FlowMonitormmwaveTCPRandomWalk.xml", true, true); //histograms and probes enabled


  // GtkConfigStore config;
  // config.ConfigureAttributes ();

  Simulator::Destroy ();
  return 0;
}

