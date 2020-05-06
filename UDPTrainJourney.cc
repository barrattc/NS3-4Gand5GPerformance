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
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

//Import libraries
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/config-store.h"
#include <ns3/buildings-helper.h>
#include "ns3/a2-a4-rsrq-handover-algorithm.h"
#include "ns3/random-walk-2d-mobility-model.h"
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"

//Define namespace
using namespace ns3;

//Arguments function
int main (int argc, char *argv[])
{
	
  /*
  
  
		  Default Attribute Values 
  
  
  */
	
  //Defaults if none given at runtime
  Time simTime = MilliSeconds (1050); //7,200,000 for 2 hours
  bool useCa = false;
  uint16_t numberOfUEs = 1;
  uint16_t numberOfeNbs = 4;
  double distance = 6000; //in meters
  uint16_t outdoorUeMinSpeed = 25; //in m/s
  uint16_t outdoorUeMaxSpeed = 25; //in m/s
  bool useV6 = false; //IPv6 disabled by default

  /*
  
  
  	  	  Command Line Options
  
  
  */
  
  //Command line arguments, overrides defaults if provided at runtime 
  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation", simTime); // --simTime=*time*
  cmd.AddValue ("useCa", "Whether to use carrier aggregation.", useCa); // --useCa=true
  cmd.AddValue ("numberOfUEs", "Number of UE nodes", numberOfUEs);
  cmd.AddValue ("numberOfeNbs", "Number of eNb nodes", numberOfeNbs);
  cmd.AddValue ("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue ("outdoorUeMinSpeed", "Min speed for UE to travel at [m/s]", outdoorUeMinSpeed);
  cmd.AddValue ("outdoorUeMaxSpeed", "Max speed for UE to travel at [m/s]", outdoorUeMaxSpeed);
  cmd.AddValue ("useV6", "Use IPv6 addresses", useV6);
  cmd.Parse (argc, argv);

  //Other default inputs can be gathered from a pre-existing text file and loaded into a future simulation.
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  // Parse again so you can override default values from the command line
  cmd.Parse (argc, argv);
  
  
  /*
  
  
  	  	  Topology; nodes, channels, devices, mobility
  
  
  */
  

  //If user carrier aggregation is set to true via the command line...
    //enable carrier aggregation
    //set number of component carriers to 2
    //split traffic equally among carriers
  if (useCa)
   {
     Config::SetDefault ("ns3::LteHelper::UseCa", BooleanValue (useCa));
     Config::SetDefault ("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue (2));
     Config::SetDefault ("ns3::LteHelper::EnbComponentCarrierManager", StringValue ("ns3::RrComponentCarrierManager"));
   }

  //Initialising the ltehelper function
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  
  //EnB Handover
  //Setting handover type to RSRQ
  lteHelper->SetHandoverAlgorithmType ("ns3::A2A4RsrqHandoverAlgorithm");
  
  //If the RSRQ (power value) of the serving cell is worse than this threshold, neighbour cells are consider for handover
  lteHelper->SetHandoverAlgorithmAttribute ("ServingCellThreshold",
                                            UintegerValue (30));
  //Minimum offset between the serving and the best neighbour cell to trigger the handover. 
  lteHelper->SetHandoverAlgorithmAttribute ("NeighbourCellOffset",
                                            UintegerValue (1));

  // Uncomment to enable logging
//  lteHelper->EnableLogComponents ();

  // Create Nodes: eNodeB and UE
  NodeContainer enbNodes;
  NodeContainer ueNodes;
  NodeContainer serverNode;
  enbNodes.Create (numberOfeNbs);
  ueNodes.Create (numberOfUEs);
  serverNode.Create(1);

  // Install Mobility Model
  MobilityHelper mobility;
  //set non moving enb nodes  
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  	  //x, y, z coords
	  enbPositionAlloc->Add (Vector (distance, 0.0, 0.0));			// eNB1
	  enbPositionAlloc->Add (Vector (distance*2,  0.0, 0.0));		// eNB2
	  enbPositionAlloc->Add (Vector (distance*3, 0.0, 0.0));		// eNB3
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (enbPositionAlloc);
  mobility.Install (enbNodes);
  BuildingsHelper::Install (enbNodes);
  
  //Set UE nodes movement - FAST MOBILITY CAUSES FAILURES 
  Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
  	  //x, y, z coords
	  uePositionAlloc->Add (Vector (100, 0.0, 0.0));		// ue1
  mobility.SetMobilityModel ("ns3::SteadyStateRandomWaypointMobilityModel");
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxSpeed", DoubleValue (outdoorUeMaxSpeed));
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinSpeed", DoubleValue (outdoorUeMinSpeed));
  mobility.SetPositionAllocator (uePositionAlloc);
  mobility.Install (ueNodes);
  BuildingsHelper::Install (ueNodes);
  
  // Create Devices and install them in the Nodes (eNB and UE)
  NetDeviceContainer enbDevs;
  NetDeviceContainer ueDevs;
  // Default scheduler is PF (proportionally fair), uncomment to use RR (round robin)
  //lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");

  enbDevs = lteHelper->InstallEnbDevice (enbNodes);
  ueDevs = lteHelper->InstallUeDevice (ueNodes);

  // Attach a UE to an eNB
  lteHelper->AttachToClosestEnb (ueDevs, enbDevs);

  //Whenever a user equipment is being provided with any service,
  //the service has to be associated with a Radio Bearer specifying
  //the configuration for Layer-2 and Physical Layer in order to have
  //its QoS clearly defined.

  // Activate a data radio bearer
  enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  EpsBearer bearer (q);
  lteHelper->ActivateDataRadioBearer (ueDevs, bearer);
  lteHelper->EnableTraces ();
  
  /*
  
  
		  Internet and IP addresses
  
  
  */
  
  //Technology for server and ue to communicate
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10000Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));
  //Install technology on nodes
  NetDeviceContainer devices;
  devices = pointToPoint.Install (ueNodes);
  
  //Use the internet stack helper and install on the ue nodes
  InternetStackHelper internet;
  internet.Install (ueNodes);
  
  //Assigning IP addresses
  NS_LOG_INFO ("Assign IP Addresses.");
  if (useV6 == false)
    {
      Ipv4AddressHelper ipv4;
      ipv4.SetBase ("10.1.1.0", "255.255.255.0");
      Ipv4InterfaceContainer i = ipv4.Assign (enbDevs, ueDevs);
      serverAddress = Address (i.GetAddress (1));
    }
  else
    {
      Ipv6AddressHelper ipv6;
      ipv6.SetBase ("2001:0000:f00d:cafe::", Ipv6Prefix (64));
      Ipv6InterfaceContainer i6 = ipv6.Assign (enbDevs, ueDevs);
      serverAddress = Address(i6.GetAddress (1,1));
    }
  
  /*
  
  
		  Applications
  
  
  */
  
  // Create a UDP Server on the receiver
  uint16_t port = 50000;
  UdpServerHelper server (port)
  Address sinkLocalAddress(InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApp = sinkHelper.Install (serverNode);
  sinkApp.Start (Seconds (1.0));
  sinkApp.Stop (Seconds (simTime));
  
  // Create the OnOff applications to send data to the UDP receiver
  OnOffHelper clientHelper ("ns3::UdpSocketFactory", Address ());
  clientHelper.SetAttribute ("Remote", remoteAddress);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  //Install the client apps on the ue nodes and start/stop themn
  ApplicationContainer clientApps = (clientHelper.Install (ueNodes);
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (simTime));
  
  /*
  
  
  	  	  Tracing and XML file creation
  
  
  */
  
  //Create xml file with given name
  AnimationInterface anim ("streamingOnTrain.xml");
  
  /*
  
  
  	  	  Run/Stop simulation
  
  
  */

  Simulator::Stop (simTime);
  Simulator::Run ();

  // GtkConfigStore config;
  // config.ConfigureAttributes ();

  Simulator::Destroy ();
  return 0;
}
