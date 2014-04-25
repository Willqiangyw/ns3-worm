/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 *
 * Author: Aderinola Gbade-Alabi <aagbade@gmail.com>
 *         Jared S. Ivey         <jivey@gatech.edu>
 *         Drew Petry            <drew.petry@gatech.edu>
 *         Peter Vieira          <pete.vieira@gmail.com>
 *
 */

#include <iostream>
#include <stdio.h>
#include <string>
#include <sys/time.h>
#include <vector>
#include <time.h>
#include <iomanip>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/stats-module.h"

#include "ns3/ns3-worm.h"


// ------------ Define worm types    ---------------
#define TCPWORMTYPE  1
#define UDPWORMTYPE  2
#define WORMTYPE     UDPWORMTYPE

// ------------ Define the topology  ---------------
#define TREES        4
#define FANOUT1      8
#define FANOUT2      16
#define TREELEGPROB  0.85

#define LINKBW       "1Mbps"
#define HLINKBW      "2Mbps"
#define BLINKBW      "5Mbps"

// ------------ Worm parameters -----------------------
#define VULNERABILITY  1.0
#define SCANRATE       100
#define SCANRANGE      0
#define PAYLOAD        1000

// ----------- Simulation settings -------------------
#define SIMTIME        10.0
#define SEEDVALUE      1

using namespace ns3;
using namespace std;

int main(int argc, char* argv[])
{
  uint32_t wormtype = TCPWORMTYPE;
  uint32_t nt = TREES;
  uint32_t nf1 = FANOUT1;
  uint32_t nf2 = FANOUT2;
  string linkbw  = LINKBW;
  string hlinkbw = HLINKBW;
  string blinkbw = BLINKBW;
  uint32_t scanrate = SCANRATE;
  uint32_t payload = PAYLOAD;
  uint32_t seedValue = SEEDVALUE;
  double vulnerability = VULNERABILITY;
  double treelegprob = TREELEGPROB;
  double simtime = SIMTIME;
  bool logTop = 0;

  CommandLine cmd;
  cmd.AddValue ("wormtype",      "Type of worm: UDP or TCP",     wormtype);
  cmd.AddValue ("trees",         "Number of trees",              nt);
  cmd.AddValue ("fanout1",       "First fanout of trees",        nf1);
  cmd.AddValue ("fanout1",       "Second fanout of trees",       nf2);
  cmd.AddValue ("linkbw",        "Link bandwidth",               linkbw);
  cmd.AddValue ("hlinkbw",       "HLink bandwidth",              hlinkbw);
  cmd.AddValue ("blinkbw",       "BLink bandwidth",              blinkbw);
  cmd.AddValue ("scanrate",      "Scan rate",                    scanrate);
  cmd.AddValue ("payload",       "Payload",                      payload);
  cmd.AddValue ("seedvalue",     "Seed value for RNG",           seedValue);
  cmd.AddValue ("vulnerability", "Vulnerability to infection",   vulnerability);
  cmd.AddValue ("treelegprob",   "Probability of tree legs",     treelegprob);
  cmd.AddValue ("simtime",       "Simulator time in seconds",    simtime);
  cmd.AddValue ("logTop",        "Display the topology stats",   logTop);

  cmd.Parse (argc,argv);

  // Set the random number generator
  SeedManager::SetSeed (seedValue);
  //SeedManager::SetSeed ((int) clock());
  UniformVariable uv;

  Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpTahoe::GetTypeId()));
  
  // Create nodes.
  vector<NodeContainer> fanout1Nodes(nt);
  vector<NodeContainer> fanout2Nodes;
  if (logTop) cout << "Topology: " << endl;
  for (uint32_t i = 0; i < nt; ++i)
    {
       if (logTop) cout << "Tree " << i << endl;
       for (uint32_t j = 0; j < nf1; ++j)
         {
           double uv1 = uv.GetValue (0.0, 1.0);
           if (uv1 <= treelegprob)
             {
               // Create a fanout node for this tree
               if (logTop) cout << "\tFanout " << fanout1Nodes[i].GetN() << ": ";
               fanout1Nodes[i].Create(1);
               fanout2Nodes.push_back(NodeContainer ());
               for (uint32_t k = 0; k < nf2; ++k)
                 {
                   double uv2 = uv.GetValue (0.0, 1.0);
                   if (uv2 <= treelegprob)
                     {
                       // Create a fanout node for this fanout node
                       fanout2Nodes[fanout2Nodes.size()-1].Create(1);
                     }
                 }
               if (logTop) cout << fanout2Nodes[fanout2Nodes.size()-1].GetN() << " ";
               if (logTop) cout << endl;
             }
         }
    }

  NodeContainer treeNodes;
  treeNodes.Create(nt);

  // Consolidate all nodes to install on the stack.
  NodeContainer nodes;
  nodes.Create(1); // Root node
  nodes.Add(treeNodes);
  for (uint32_t i = 0; i < nt; ++i)
    {
      nodes.Add(fanout1Nodes[i]);
    }

  for (uint32_t i = 0; i < fanout2Nodes.size(); ++i)
    {
      nodes.Add(fanout2Nodes[i]);
    }

  // Install nodes on internet stack.
  InternetStackHelper stack;
  stack.Install (nodes);

  // Create channels and devices.
  PointToPointHelper tree;
  tree.SetDeviceAttribute ("DataRate", StringValue (blinkbw));
  tree.SetChannelAttribute ("Delay", StringValue ("20ms"));
  vector<NetDeviceContainer> treeDevices(nt);

  PointToPointHelper fanout1;
  fanout1.SetDeviceAttribute ("DataRate", StringValue (hlinkbw));
  fanout1.SetChannelAttribute ("Delay", StringValue ("20ms"));
  vector<NetDeviceContainer> fanout1Devices;

  PointToPointHelper fanout2;
  fanout2.SetDeviceAttribute ("DataRate", StringValue (linkbw));
  fanout2.SetChannelAttribute ("Delay", StringValue ("20ms"));
  vector<NetDeviceContainer> fanout2Devices;

  uint32_t next = 0;
  uint32_t nNext = 0;
  for (uint32_t i = 0; i < nt; ++i)
    {
      // Connect the root node to each tree node
      treeDevices[i] = tree.Install (nodes.Get(0),
                                     treeNodes.Get(i));

      for (uint32_t j = 0; j < fanout1Nodes[i].GetN(); ++j)
        {
          // Connect each tree node to its fanout nodes
          fanout1Devices.push_back (fanout1.Install (treeNodes.Get(i),
                                                     fanout1Nodes[i].Get(j)));

          for (uint32_t k = 0; k < fanout2Nodes[next+j].GetN(); ++k)
            {
              fanout2Devices.push_back (fanout2.Install (fanout1Nodes[i].Get(j),
                                                         fanout2Nodes[next+j].Get(k)));
            }
        }
      next += fanout1Nodes[i].GetN();
    }

  // Assign IP addresses
  ostringstream oss;
  Ipv4AddressHelper ipv4;
  vector<Ipv4InterfaceContainer> iTree(nt);
  vector<Ipv4InterfaceContainer> iFanout1;
  vector<Ipv4InterfaceContainer> iFanout2;
  next = 0;
  nNext = 0;
  for (uint32_t i = 0; i < nt; ++i)
    {
      oss.str ("");
      oss << i + 1 << ".1.0.0";
      ipv4.SetBase (oss.str ().c_str (), "255.255.255.0");

      iTree[i] = ipv4.Assign(treeDevices[i]);

      for (uint32_t j = 0; j < fanout1Nodes[i].GetN(); ++j)
        {
          oss.str ("");
          oss << "192." << 1 + next + j << ".1.0";
          ipv4.SetBase (oss.str ().c_str (), "255.255.255.0");

          iFanout1.push_back(ipv4.Assign(fanout1Devices[next+j]));
          for (uint32_t k = nNext; k < nNext + fanout2Nodes[next+j].GetN(); ++k)
            {
              oss.str ("");
              oss << "192." << 1 + next + j << "." << k + 2 - nNext << ".0";
              ipv4.SetBase (oss.str ().c_str (), "255.255.255.0");
              iFanout2.push_back(ipv4.Assign(fanout2Devices[k]));
            }
          nNext += fanout2Nodes[next+j].GetN();
        }
      next += fanout1Nodes[i].GetN();
    }

  // Populate routing tables.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Install client and server applications.
  // Install packet sinks for client and server nodes.
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  ApplicationContainer wormApps;
  UniformVariable uv_time(0.0, 1.0);
  next = 0;
  uint32_t lastFanout = iFanout2.size();
  uint32_t numVulnerableNodes = 0;
  for (uint32_t i = 0; i < fanout2Nodes.size(); ++i)
    {
      for (uint32_t j = 0; j < fanout2Nodes[i].GetN(); j++)
        {
          PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory",
                                       InetSocketAddress (Ipv4Address::GetAny (),
                                                          9999));

          ApplicationContainer sinkApp = sinkHelper.Install (fanout2Nodes[i].Get(j));
          sinkApp.Start (Seconds (0.0));

          OnOffHelper client ("ns3::UdpSocketFactory", Address ());
          AddressValue remoteAddress (InetSocketAddress (iFanout2[lastFanout-next-j-1].GetAddress (1), 9999));
          client.SetAttribute ("Remote", remoteAddress);
          client.SetAttribute ("MaxBytes", UintegerValue(50000));

          ApplicationContainer clientApp;
          clientApp.Add (client.Install (fanout2Nodes[i].Get(j)));
          clientApp.Start (Seconds (uv_time.GetValue()));

          Ptr<Worm> wormApp = CreateObject<Worm> ();
          wormApp->SetTotalNodes (nt*nf1*nf2);
//          wormApp->SetExistNodes (lastFanout);

          if (uv.GetValue(0.0, 1.0) <= vulnerability) {
            wormApp->SetVulnerable (true);
            numVulnerableNodes++;
          }
          wormApp->SetExistNodes(numVulnerableNodes);

          if (next + j == 0) 
            wormApp->SetInfected (true);

          wormApp->SetStartTime (Seconds (0.0));
          wormApp->SetStopTime (Seconds (simtime));

          fanout2Nodes[i].Get(j)->AddApplication (wormApp);
          if (wormtype == TCPWORMTYPE) 
            wormApp->SetUp ("ns3::TcpSocketFactory", 5000);
          if (wormtype == UDPWORMTYPE) 
            wormApp->SetUp ("ns3::UdpSocketFactory", 5000);
        }
      next += fanout2Nodes[i].GetN();
    }

//  std::cerr << "Num Vulnerable Nodes: " << numVulnerableNodes
//            << "/" << nt*nf1*nf2*treelegprob*treelegprob
//            << " = " << (double)numVulnerableNodes/treelegprob/treelegprob/(double)(nt*nf1*nf2)*100.0 << "%"
//            << std::endl;

  // Flow Monitor
  //Ptr<FlowMonitor> flowmon;
  //FlowMonitorHelper flowmonHelper;
  //flowmon = flowmonHelper.InstallAll ();

//  Simulator::Stop(Seconds(2));
  Simulator::Run();

  double percInfected = 100.*(double)Worm::GetInfectedNodes() / Worm::GetTotalNodes()/treelegprob/treelegprob;
  double percVulnerable = (double)numVulnerableNodes/treelegprob/treelegprob/(double)(nt*nf1*nf2)*100.0;
  double percInfToVuln = percInfected / percVulnerable;
  cerr << "Time(s)\tInf(#)\tTot(#)\tPerc(%)\tVuln(%)\tInf/Vul(%)" << std::endl;
  cerr << setprecision(3) << Simulator::Now().GetSeconds() << "\t"
       << Worm::GetInfectedNodes() << "\t"
       << (uint32_t)(Worm::GetTotalNodes()*treelegprob*treelegprob) << "\t"
       << setprecision(4) << percInfected << "\t"
       << setprecision(4) << percVulnerable << "\t"
       << setprecision(4) << percInfToVuln*100. << "\t"
       << std::endl;



  //flowmon->SerializeToXmlFile ("p4.flowmon", false, false);

  Simulator::Destroy();
}
