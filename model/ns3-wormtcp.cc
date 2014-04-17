


//#include "debug.h"
//#include "ns3/tcp-header.h" // added by Derin
#include "ns3-wormtcp.h"
#include "ns3/tcp-tahoe.h"//#include "tcp-tahoe.h"

#include "ns3/node.h"//#include "node.h"
#include "ns3/simulator.h" //#include "simulator.h"
#include <map>

#ifdef HAVE_QT
#include <qnamespace.h>
#endif

using namespace std;


ns3::RandomVariable* WormTCP::rngD = NULL;//Random*  WormTCP::rngD = nil;
Count_t  WormTCP::connections = 2;

// In this version of the worm application I am using a constant
//    packets per second method to sent UDP packets.
// I will later change this into a more robust method.

// Constructors


WormTCP::WormTCP(bool debug)
    : tcp(NULL), _debug(debug)//: tcp(nil)
{
    if(_debug) cout << "Attaching application " << this << endl;

  tcp_c.clear();
  tcp_c_map.clear();
  tcp_r_count.clear();
  sentAck.clear();

  if (!rngD) rngD = new ns3::UniformVariable(0, 1);//if (!rngD) rngD = new Uniform(0, 1);
}


// Application Methods
void WormTCP::StartApp()     // Called at time specified by Start
{
  Worm:: StartApp();
}

void WormTCP::StopApp()    // Called at time specified by Stop
{
  Worm :: StopApp();
}

void WormTCP::AttachNode(ns3::Node* n)//void WormTCP::AttachNode(Node* n)
{
  Worm::AttachNode(n);

  tcp = new ns3::TcpTahoe();
  tcp->SetNode(n); //tcp = new TCPTahoe(n);
  
  //tcp->Bind(infectionport);
  tcp->Listen();
  //tcp->AttachApplication(this);

  if(_debug) cout <<"tcp   : " << tcp <<", attached application "/*<< tcp->GetApplication()*/<<endl; //DEBUG(1,(if(_debug) cout <<"tcp   : " << tcp <<", attached application "<< tcp->GetApplication()<<endl;

 for (int i =0; i<(int)connections; i++) {
    ns3::TcpSocketBase* temp_tcp = new ns3::TcpTahoe(); //TCP * temp_tcp = new TCPTahoe(n);
    //temp_tcp->AttachApplication(this);
    tcp_c.push_back(temp_tcp);
    sentAck.push_back(0);
    tcp_c_map[temp_tcp]=i;
    if(_debug) cout <<"tcp_c : " << tcp_c[i] <<", attached application "/*<< tcp_c[i]->GetApplication()*/<<endl;//DEBUG(1,(if(_debug) cout <<"tcp_c : " << tcp_c[i] <<", attached application "<< tcp_c[i]->GetApplication()<<endl;
  }
}

ns3::Application* WormTCP::Copy() const// Application* WormTCP::Copy() const
{
  return new WormTCP(*this);
}

void WormTCP::Activate()
{
    ns3::Ptr<ns3::Ipv4> ipv4 = node->GetObject<ns3::Ipv4>();
    ns3::Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
    ns3::Ipv4Address addri = iaddr.GetLocal ();
  if(_debug) cout<<"StartInfection["<<addri/*(node->GetIPAddr())*/<<
       "]: Infected machine."<<endl;
  // Set the node to red on animation
#ifdef HAVE_QT
  node->Color(Qt::red);
#endif

  for (int i =0; i<(int) connections; i++)
    SendWorm(i);
}

void WormTCP::Initialize()
{
  if(_debug) cout<<"TCP initialize"<<endl;
  // override this in child classes
}


void WormTCP::SendWorm(int threadno)
{
  //  char buffer[payloadlength+1];   // later on I'll add the real contents here
  
  IPAddr_t target = GenerateNextIPAddress();

  ns3::Ptr<ns3::Ipv4> ipv4 = node->GetObject<ns3::Ipv4>();
  ns3::Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
  ns3::Ipv4Address addri = iaddr.GetLocal ();
  if(_debug) cout<<"SendWorm["<<addri<<
      "]: <Thread "<<threadno<<"> first connecting to "<<target<<endl;
  ns3::InetSocketAddress ipPort(target, infectionport);
  tcp_c[threadno]->Connect(ipPort);//tcp_c[threadno]->Connect(target, infectionport);

  //  udp->SendTo(payloadlength, target, defaultinfectionport);
    if(_debug) cout<<"sent worm packet"<<endl;
}

void WormTCP::ConnectionComplete( ns3::IpL4Protocol *proto)//void WormTCP::ConnectionComplete(L4Protocol *proto)
{
  int threadno = tcp_c_map[(ns3::TcpSocketBase*)proto];//int threadno = tcp_c_map[(TCP*)proto];//derin question?
  
  ns3::Ptr<ns3::Ipv4> ipv4 = node->GetObject<ns3::Ipv4>();
  ns3::Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
  ns3::Ipv4Address addri = iaddr.GetLocal ();
  if(_debug) cout<<"ConnectionComplete["<<addri<<
      "]: <Thread "<<threadno<<"> connection successful to "
                << "not sure what to use" /*(string)IPAddr(proto->peerIP)*/
      <<" now sending packet of length "<<payloadlength<<endl;

 
  sentAck[threadno]=0;

  // a small hack to add some transmission delay sometimes
  /*
  double rvalue = rngD->Value();
  if (rvalue>0.5)
    rvalue = 0;
  tcp_c[threadno]->AddExtraTxDelay(rvalue / 1000);
  */

  ns3::Ptr<ns3::Packet> packet = new ns3::Packet(payloadlength);
  tcp_c[threadno]->Send(packet, (uint32_t)0);//tcp_c[threadno]->Send(wormdata, payloadlength);

  if(_debug) cout<<"Contents:"<<wormdata<<endl; //DEBUG(1,(if(_debug) cout<<"Contents:"<<wormdata<<endl;
}

void WormTCP::Sent(Count_t c, ns3::IpL4Protocol *proto)//void WormTCP::Sent(Count_t c, L4Protocol *proto)
{
  int threadno = tcp_c_map[(ns3::TcpSocketBase*)proto];//int threadno = tcp_c_map[(TCP*)proto];

  sentAck[threadno]+=c;

  if (sentAck[threadno]>=payloadlength) {

      ns3::Ptr<ns3::Ipv4> ipv4 = node->GetObject<ns3::Ipv4>();
      ns3::Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
      ns3::Ipv4Address addri = iaddr.GetLocal ();
    if(_debug) cout <<"Sent["<<addri
            <<"]: <Thread "<<threadno<<"> All worm data sent, now closing"<<endl;
    tcp_c[threadno]->Close();
  }
}

void WormTCP::ConnectionFailed(ns3::IpL4Protocol* proto, bool aborted)//void WormTCP::ConnectionFailed(L4Protocol* proto, bool aborted)
{
  int threadno = tcp_c_map[(ns3::TcpSocketBase*)proto];//int threadno = tcp_c_map[(TCP*)proto];

  ns3::Ptr<ns3::Ipv4> ipv4 = node->GetObject<ns3::Ipv4>();
  ns3::Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
  ns3::Ipv4Address addri = iaddr.GetLocal ();
  if(_debug) cout<<"ConnectionFailed["<<addri<<
      "]: <Thread "<<threadno<<"> Connection failed to "<<"not sure what to use"/*(string)IPAddr(proto->peerIP)*/<<endl;
  SendWorm(threadno);
}

void WormTCP::CloseRequest(ns3::IpL4Protocol *proto)//void WormTCP::CloseRequest(L4Protocol *proto)
{
    ns3::Ptr<ns3::Ipv4> ipv4 = node->GetObject<ns3::Ipv4>();
    ns3::Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
    ns3::Ipv4Address addri = iaddr.GetLocal ();
  if(_debug) cout<<"CloseRequest["<<addri<<"]: Close request received, now closing"<<endl;
  tcp_r_count.erase((ns3::TcpSocketBase*)proto);//tcp_r_count.erase((TCP*)proto);
  tcp_r_isworm.erase((ns3::TcpSocketBase*)proto);//tcp_r_isworm.erase((TCP*)proto);

  ((ns3::TcpSocketBase*)proto)->Close(); //proto->Close();//proto->Close();
}

void WormTCP::Closed(ns3::IpL4Protocol *proto)//void WormTCP::Closed(L4Protocol *proto)
{
  int threadno = tcp_c_map[(ns3::TcpSocketBase*)proto];//int threadno = tcp_c_map[(TCP*)proto];

  ns3::Ptr<ns3::Ipv4> ipv4 = node->GetObject<ns3::Ipv4>();
  ns3::Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
  ns3::Ipv4Address addri = iaddr.GetLocal ();
  if(_debug) cout<<"Closed["<<addri<<"]:thread "<<threadno<<" connection has closed."<<endl; //derin question?
  // I am doing a hack here...
  // I am creating a new TCP object and pointing it using tcp_c... I think I should reuse this one
  // but for now I can't
  
  tcp_c_map.erase((ns3::TcpSocketBase*)proto);//tcp_c_map.erase((TCP*)proto);

  ns3::TcpSocketBase *temp_tcp = new ns3::TcpTahoe(); //TCP *temp_tcp = new TCPTahoe(node);
  //temp_tcp->AttachApplication(this);// derin question?
  tcp_c_map[temp_tcp] = threadno;
  tcp_c[threadno]=temp_tcp;

//  if(_debug) cout <<"tcp_c : " << temp_tcp <<", attached application "<< temp_tcp->GetApplication()<<endl;// derin question?
  
  SendWorm(threadno);

}

void WormTCP::SetConnections(Count_t th)
{
  connections = th;
}

void WormTCP::Receive(ns3::Packet *p, ns3::IpL4Protocol *proto, Seq_t)//void WormTCP::Receive(Packet *p, L4Protocol *proto, Seq_t)
{
  Count_t received = tcp_r_count[(ns3::TcpSocketBase*)proto];//Count_t received = tcp_r_count[(TCP*)proto];

  ns3::Ptr<ns3::Ipv4> ipv4 = node->GetObject<ns3::Ipv4>();
  ns3::Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
  ns3::Ipv4Address addri = iaddr.GetLocal ();
  if(_debug) cout<<"Receive["<<addri<<"]: Received TCP packet of size "<<p->GetSize()<<
      "Prev received "<<received<<endl; // derin question?
  
  if (received==0) {
    tcp_r_isworm[(ns3::TcpSocketBase*)proto]= PacketIsWorm(p);//tcp_r_isworm[(TCP*)proto]= PacketIsWorm(p);
  }
  received+=p->GetSize();//received+=p->Size();
  tcp_r_count[(ns3::TcpSocketBase*)proto] = received;//tcp_r_count[(TCP*)proto] = received;

  if (tcp_r_isworm[(ns3::TcpSocketBase*)proto] && received>=payloadlength) {//if (tcp_r_isworm[(TCP*)proto] && received>=payloadlength) {
    if (vulnerable && !infected) {
      Infect();
    }
  }

  delete p;
}
