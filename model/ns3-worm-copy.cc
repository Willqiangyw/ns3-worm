
#include "ns3-worm.h"

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/udp-socket.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "onoff-application.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/trace-source-accessor.h"

bool     Worm::initialized = false;
// this is for setting vulnerabilities
ns3::RandomVariable*  Worm::rngV = NULL;
Time_t   Worm::defaultPatchTime = 1.0;
Size_t   Worm::payloadlength = 376;
PortId_t Worm::infectionport = 1434;
//100% machines are vulnerable, please change the folowing for
// for making host's vulnerability be selected randomly
double   Worm::vulnerability = 1.0;

IPAddr_t Worm::baseIP = ns3::Ipv4Address("192.168.0.0");

Count_t  Worm::totalinfected = 0;
Count_t  Worm::totalvulnerable = 0;
Count_t  Worm::totalinstances = 0;

std::string   Worm::signature = "WORM";
char *   Worm::wormdata = NULL;

//// By default we are setting the target vector to be NULL
//// The code then selects the uniform target vector object by default

//WormTargetVector* Worm::defaultTV = NULL;


namespace ns3 {

// Constructors

Worm::Worm(bool isVulnerable)
  : infected(false), vulnerable(isVulnerable)
{
  // Whether or not the application object should
  // be copied when connection requests are processed
//  copyOnConnect=false; member variable of GTNets::Application

  if (!initialized) {
    Initialize();
    initialized = true;
  }

  if (!wormdata) {
    PrepareWormData(wormdata);
  }

  if (vulnerability!=1.0)   {
    if (!rngV) rngV = new ns3::UniformVariable(0, 1);
  }

//  // if there is no target vector object defined then we are setting the
//  // it to be the Uniform random target vector class.
//  if (!defaultTV) {
//    defaultTV = new WTVUniform(DEFAULTSCANRANGE);
//  }
}

// Application Methods
void Worm::StartApplication()    // Called at time specified by Start
{
  if (infected)
  {
    Activate();
  }
  started = true;
}

void Worm::StopApplication()     // Called at time specified by Stop
{
  started = false;
}

//-----------------------------
//       Packet Sink
//-----------------------------

uint32_t Worm::GetTotalRx () const
{
  NS_LOG_FUNCTION (this);
  return m_totalRx;
}

Ptr<Socket>
Worm::GetListeningSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket_rx;
}

std::list<Ptr<Socket> >
Worm::GetAcceptedSockets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socketList;
}

void Worm::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket_rx = 0;
  m_socketList.clear ();

  // chain up
  Application::DoDispose ();
}


// Application Methods
void Worm::StartApplication ()    // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  // Create the socket if not already
  if (!m_socket_rx)
    {
      m_socket_rx = Socket::CreateSocket (GetNode (), m_tid);
      m_socket_rx->Bind (m_local);
      m_socket_rx->Listen ();
      m_socket_rx->ShutdownSend ();
      if (addressUtils::IsMulticast (m_local))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket_rx);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, m_local);
            }
          else
            {
              NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
            }
        }
    }

  m_socket_rx->SetRecvCallback (MakeCallback (&Worm::HandleRead, this));
  m_socket_rx->SetAcceptCallback (
    MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    MakeCallback (&Worm::HandleAccept, this));
  m_socket_rx->SetCloseCallbacks (
    MakeCallback (&Worm::HandlePeerClose, this),
    MakeCallback (&Worm::HandlePeerError, this));
}

void Worm::StopApplication ()     // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
  while(!m_socketList.empty ()) //these are accepted sockets, close them
    {
      Ptr<Socket> acceptedSocket = m_socketList.front ();
      m_socketList.pop_front ();
      acceptedSocket->Close ();
    }
  if (m_socket_rx)
    {
      m_socket_rx->Close ();
      m_socket_rx->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void Worm::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }
      m_totalRx += packet->GetSize ();
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
                       << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
      m_rxTrace (packet, from);
    }
}


void Worm::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

void Worm::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}


void Worm::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  s->SetRecvCallback (MakeCallback (&Worm::HandleRead, this));
  m_socketList.push_back (s);
}

//-----------------------
//      OnOFF
//-----------------------

void
Worm::SetMaxBytes (uint32_t maxBytes)
{
  NS_LOG_FUNCTION (this << maxBytes);
  m_maxBytes = maxBytes;
}

Ptr<Socket>
Worm::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket_tx;
}

int64_t
Worm::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_onTime->SetStream (stream);
  m_offTime->SetStream (stream + 1);
  return 2;
}

void
Worm::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket_tx = 0;
  // chain up
  Application::DoDispose ();
}

// Application Methods
void Worm::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);

  // Create the socket if not already
  if (!m_socket_tx)
    {
      m_socket_tx = Socket::CreateSocket (GetNode (), m_tid);
      if (Inet6SocketAddress::IsMatchingType (m_peer))
        {
          m_socket_tx->Bind6 ();
        }
      else if (InetSocketAddress::IsMatchingType (m_peer) ||
               PacketSocketAddress::IsMatchingType (m_peer))
        {
          m_socket_tx->Bind ();
        }
      m_socket_tx->Connect (m_peer);
      m_socket_tx->SetAllowBroadcast (true);
      m_socket_tx->ShutdownRecv ();

      m_socket_tx->SetConnectCallback (
        MakeCallback (&Worm::ConnectionSucceeded, this),
        MakeCallback (&Worm::ConnectionFailed, this));
    }
  m_cbrRateFailSafe = m_cbrRate;

  // Insure no pending event
  CancelEvents ();
  // If we are not yet connected, there is nothing to do here
  // The ConnectionComplete upcall will start timers at that time
  //if (!m_connected) return;
  ScheduleStartEvent ();
}

void Worm::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  CancelEvents ();
  if(m_socket_tx != 0)
    {
      m_socket_tx->Close ();
    }
  else
    {
      NS_LOG_WARN ("OnOffApplication found null socket to close in StopApplication");
    }
}

void Worm::CancelEvents ()
{
  NS_LOG_FUNCTION (this);

  if (m_sendEvent.IsRunning () && m_cbrRateFailSafe == m_cbrRate )
    { // Cancel the pending send packet event
      // Calculate residual bits since last packet sent
      Time delta (Simulator::Now () - m_lastStartTime);
      int64x64_t bits = delta.To (Time::S) * m_cbrRate.GetBitRate ();
      m_residualBits += bits.GetHigh ();
    }
  m_cbrRateFailSafe = m_cbrRate;
  Simulator::Cancel (m_sendEvent);
  Simulator::Cancel (m_startStopEvent);
}

// Event handlers
void Worm::StartSending ()
{
  NS_LOG_FUNCTION (this);
  m_lastStartTime = Simulator::Now ();
  ScheduleNextTx ();  // Schedule the send packet event
  ScheduleStopEvent ();
}

void Worm::StopSending ()
{
  NS_LOG_FUNCTION (this);
  CancelEvents ();

  ScheduleStartEvent ();
}

// Private helpers
void Worm::ScheduleNextTx ()
{
  NS_LOG_FUNCTION (this);

  if (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    {
      uint32_t bits = m_pktSize * 8 - m_residualBits;
      NS_LOG_LOGIC ("bits = " << bits);
      Time nextTime (Seconds (bits /
                              static_cast<double>(m_cbrRate.GetBitRate ()))); // Time till next packet
      NS_LOG_LOGIC ("nextTime = " << nextTime);
      m_sendEvent = Simulator::Schedule (nextTime,
                                         &Worm::SendPacket, this);
    }
  else
    { // All done, cancel any pending events
      StopApplication ();
    }
}

void Worm::ScheduleStartEvent ()
{  // Schedules the event to start sending data (switch to the "On" state)
  NS_LOG_FUNCTION (this);

  Time offInterval = Seconds (m_offTime->GetValue ());
  NS_LOG_LOGIC ("start at " << offInterval);
  m_startStopEvent = Simulator::Schedule (offInterval, &Worm::StartSending, this);
}

void Worm::ScheduleStopEvent ()
{  // Schedules the event to stop sending data (switch to "Off" state)
  NS_LOG_FUNCTION (this);

  Time onInterval = Seconds (m_onTime->GetValue ());
  NS_LOG_LOGIC ("stop at " << onInterval);
  m_startStopEvent = Simulator::Schedule (onInterval, &Worm::StopSending, this);
}


void Worm::SendPacket ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());
  Ptr<Packet> packet = Create<Packet> (m_pktSize);
  m_txTrace (packet);
  m_socket_tx->Send (packet);
  m_totBytes += m_pktSize;
  if (InetSocketAddress::IsMatchingType (m_peer))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                   << "s on-off application sent "
                   <<  packet->GetSize () << " bytes to "
                   << InetSocketAddress::ConvertFrom(m_peer).GetIpv4 ()
                   << " port " << InetSocketAddress::ConvertFrom (m_peer).GetPort ()
                   << " total Tx " << m_totBytes << " bytes");
    }
  else if (Inet6SocketAddress::IsMatchingType (m_peer))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                   << "s on-off application sent "
                   <<  packet->GetSize () << " bytes to "
                   << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6 ()
                   << " port " << Inet6SocketAddress::ConvertFrom (m_peer).GetPort ()
                   << " total Tx " << m_totBytes << " bytes");
    }
  m_lastStartTime = Simulator::Now ();
  m_residualBits = 0;
  ScheduleNextTx ();
}


void Worm::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void Worm::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

}
