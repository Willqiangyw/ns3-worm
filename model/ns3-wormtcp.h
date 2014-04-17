
#ifndef WORMTCP_APPLICATION_H //#ifndef __wormtcp__h__
#define WORMTCP_APPLICATION_H //#define __wormtcp__h__

#include "ns3/core-module.h" //#include "common-defs.h"
#include "ns3/application.h" //#include "application.h"
#include "ns3/timer.h" //#include "timer.h"
#include "ns3/tcp-header.h"  //#include "tcp.h"
#include "ns3/rng-stream.h"  //#include "rng.h"
#include "ns3-worm.h"

//#include <map>

// This version only supports TCP protocol

//Doc:ClassXRef
class WormTCP : public Worm {
 //Doc:Class Define a application that works as the Sapphire worm
  //Doc:Class 
public:
  // Define a number of constructors for different needs
  //Doc:Method
  WormTCP(bool debug);   // Just specify endpoint
    //Doc:Desc Specifies a node to attach the worm application to. 

  virtual void AttachNode(ns3::Node*); // Note which node attached to
  virtual ~WormTCP() {};
  virtual Application* Copy() const;// Make a copy of the application
  
  static void SetConnections(Count_t);

  virtual void Initialize();

public:
  // newly defined in this class
  virtual void Activate();

private:
  bool _debug;
  ns3::TcpSocketBase* tcp; //TCP*                  tcp;           // this one is used for the server side
  std::vector <ns3::TcpSocketBase*> tcp_c;//std::vector <TCP*>    tcp_c;         // the client side protocols
  std::map <ns3::TcpSocketBase*, int> tcp_c_map; //std::map <TCP*,int>   tcp_c_map;     // client side protocol mapping to know how much data ack'd
  std::map <ns3::TcpSocketBase*,Count_t>   tcp_r_count; //std::map <TCP*,Count_t>   tcp_r_count;     // server side protocol mapping to know how much data has been received
  std::map <ns3::TcpSocketBase*,bool>  tcp_r_isworm;//std::map <TCP*,bool>  tcp_r_isworm;    // server side protocol mapping to know store if packets that are coming is a worm

  std::vector <Count_t> sentAck;

  static ns3::RandomVariable* rngD; //static Random* rngD;   // for adding some transmission delay

  static Count_t connections;

  void SendWorm(int);

  // the following method is called when packets are received from the port
  virtual void Receive(ns3::Packet*, ns3::IpL4Protocol*, Seq_t);//virtual void Receive(Packet*, L4Protocol*, Seq_t); // Data received
  virtual void Sent(Count_t , ns3::IpL4Protocol*); //virtual void Sent(Count_t , L4Protocol*);
  virtual void ConnectionComplete(ns3::IpL4Protocol*);//virtual void ConnectionComplete(L4Protocol*);
  virtual void ConnectionFailed(ns3::IpL4Protocol*, bool);//virtual void ConnectionFailed(L4Protocol*, bool);
  virtual void CloseRequest(ns3::IpL4Protocol*);//virtual void CloseRequest(L4Protocol*);
  virtual void Closed(ns3::IpL4Protocol*);//virtual void Closed(L4Protocol*);

  virtual void StartApp();    // Called at time specified by Start
  virtual void StopApp();     // Called at time specified by Stop

};

#endif

