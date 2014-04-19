/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ns3-worm.h"
#include <vector>
// #include "ns3/ns3-worm-helper.h"
//NS_LOG_COMPONENT_DEFINE ("ns3-worm");

using namespace ns3;


int 
main (int argc, char *argv[])
{
    bool verbose = true;

    CommandLine cmd;
    cmd.AddValue ("verbose", "Tell application to log if true", verbose);

    cmd.Parse (argc,argv);

  /* ... */

  // Setup topology
  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // Create Apps and install nodes
//  Ptr<Worm> worm0 = new Worm(true, true);
  Ptr<Worm> worm0 = ns3::CreateObject<Worm>();
  worm0->SetInfected(true);
  worm0->SetName("Worm0");
  worm0->PrintState();
  //worm0->SetAttribute("DataRate", DataRateValue(DataRate(std::string("500kb/s"))));
//  worm0->SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));
//  worm0->SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));
  worm0->SetUp(std::string("ns3::TcpSocketFactory"),
                 interfaces.GetAddress(1),
                 uint32_t(6110),
                 nodes.Get(0));


//  Ptr<Worm> worm1 = new Worm(false, true);
  Ptr<Worm> worm1 = ns3::CreateObject<Worm>();
  worm1->SetName("Worm1");
  worm1->PrintState();
  std::vector<Ptr<Worm> > worms;
  worms.push_back(worm1);

  for (size_t i = 0; i < worms.size(); ++i) {
//    worms[i]->SetAttribute("DataRate", StringValue("500kb/s"));
//    worms[i]->SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));
//    worms[i]->SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));
    worms[i]->SetUp(std::string("ns3::TcpSocketFactory"),
                interfaces.GetAddress(1),
                uint32_t(5),
                nodes.Get(1));
  }

  nodes.Get(0)->AddApplication(worm0);
  nodes.Get(1)->AddApplication(worm1);

  ApplicationContainer apps;
  apps.Add(worm0);
  apps.Add(worm1);

  apps.Start(Seconds(0.0));
  apps.Stop(Seconds(1.0));
  Simulator::Stop(Seconds(2));

  Simulator::Run ();

  worm0->PrintState();
  worm1->PrintState();

  Simulator::Destroy ();
  return 0;
}
