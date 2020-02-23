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
#include "ns3/internet-stack-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/three-gpp-http-helper.h"
#include "ns3/applications-module.h"
//#include "ns3/gtk-config-store.h"

//Define namespace
using namespace ns3;

//Arguments function
int main (int argc, char *argv[])
{
  //Defaults if none given at runtime
  double simTime = 300.0;
  bool useCa = false;

  //Command line arguments, overrides defaults if given
  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
  cmd.AddValue ("useCa", "Whether to use carrier aggregation.", useCa);
  cmd.Parse (argc, argv);

  //Other default inputs can be gathered from a pre-existing text file and loaded into a future simulation.
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  // Parse again so you can override default values from the command line
  cmd.Parse (argc, argv);

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

  // Uncomment to enable logging
//  lteHelper->EnableLogComponents ();

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
    "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"), //speed of walk
    "Bounds", RectangleValue (Rectangle (0.0, 20.0, 0.0, 20.0)));
  mobility.Install (clientServerNodes.Get(0));
  BuildingsHelper::Install (clientServerNodes.Get(0));

  // Create Devices and install them in the Nodes (eNB and UE)
  NetDeviceContainer enbDevs;
  NetDeviceContainer ueDevs;
  // Default scheduler is PF (proportionally fair), uncomment to use RR (round robin)
  //lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");

  enbDevs = lteHelper->InstallEnbDevice (enbNodes);
  ueDevs = lteHelper->InstallUeDevice (clientServerNodes.Get (0));

  // Attach a UE to a eNB
  lteHelper->Attach (ueDevs, enbDevs.Get (0));

  //Whenever a user equipment is being provided with any service,
  //the service has to be associated with a Radio Bearer specifying
  //the configuration for Layer-2 and Physical Layer in order to have
  //its QoS clearly defined.

  // Activate a data radio bearer
  enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  EpsBearer bearer (q);
  lteHelper->ActivateDataRadioBearer (ueDevs, bearer);
  lteHelper->EnableTraces ();

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

  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();

  // GtkConfigStore config;
  // config.ConfigureAttributes ();

  Simulator::Destroy ();
  return 0;
}
