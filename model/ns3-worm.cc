
#include "ns3-worm.h"
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"

//// By default we are setting the target vector to be NULL
//// The code then selects the uniform target vector object by default

//WormTargetVector* Worm::defaultTV = NULL;
NS_LOG_COMPONENT_DEFINE ("ns3-worm");

ns3::UniformVariable Worm::x = ns3::UniformVariable(1.000,33.999);
ns3::UniformVariable Worm::y = ns3::UniformVariable(1.000,255.999);
uint32_t Worm::m_totalInfected = 1;

ns3::TypeId Worm::GetTypeId(void)
{
  static ns3::TypeId tid = ns3::TypeId ("ns3::Worm")
    .SetParent<ns3::Application> ()
    .AddConstructor<Worm> ()
    .AddAttribute ("DataRate", "The data rate in on state.",
                   ns3::DataRateValue (ns3::DataRate ("500kb/s")),
                   ns3::MakeDataRateAccessor (&Worm::m_cbrRate),
                   ns3::MakeDataRateChecker ())
    .AddAttribute ("PacketSize", "The size of packets sent in on state",
                   ns3::UintegerValue (512),
                   ns3::MakeUintegerAccessor (&Worm::m_pktSize),
                   ns3::MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("OnTime", "A RandomVariableStream used to pick the duration of the 'On' state.",
                   ns3::StringValue ("ns3::ConstantRandomVariable[Constant=0.5]"),
                   ns3::MakePointerAccessor (&Worm::m_onTime),
                   ns3::MakePointerChecker <ns3::RandomVariableStream>())
    .AddAttribute ("OffTime", "A RandomVariableStream used to pick the duration of the 'Off' state.",
                   ns3::StringValue ("ns3::ConstantRandomVariable[Constant=0.5]"),
                   ns3::MakePointerAccessor (&Worm::m_offTime),
                   ns3::MakePointerChecker <ns3::RandomVariableStream>())
    .AddAttribute ("MaxBytes",
                   "The total number of bytes to send. Once these bytes are sent, "
                   "no packet is sent again, even in on state. The value zero means "
                   "that there is no limit.",
                   ns3::UintegerValue (0),
                   ns3::MakeUintegerAccessor (&Worm::m_maxBytes),
                   ns3::MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Protocol", "The type of protocol to use.",
                   ns3::TypeIdValue (ns3::UdpSocketFactory::GetTypeId ()),
                   ns3::MakeTypeIdAccessor (&Worm::m_typeId),
                   ns3::MakeTypeIdChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     ns3::MakeTraceSourceAccessor (&Worm::m_txTrace));
  return tid;
}

// Constructors

Worm::Worm()
  : m_infected(false),
    m_vulnerable(true),
    m_connected(false),
    m_residualBits(0),
    m_totalBytes(0),
    m_lastStartTime(ns3::Seconds(0)),
    m_sinkSocket(0),
    m_onoffSocket(0)
{
    NS_LOG_FUNCTION (this);

    //std::cerr << "Constructing Worm" << std::endl;
//    std::cerr << "Starting State:\n"
//              << "\tInfected: " << (m_infected ? "True\n" : "False\n")
//              << "\tVulnerable: " << (m_vulnerable ? "True\n" : "False\n")
//              << std::endl;
}

Worm::~Worm()
{
    NS_LOG_FUNCTION (this);
}

void Worm::SetInfected(bool alreadyInfected)
{
    m_infected = alreadyInfected;
}

void Worm::SetVulnerable(bool vulnerable)
{
    m_vulnerable = vulnerable;
}

void Worm::SetMaxBytes(uint32_t maxBytes)
{
    m_maxBytes = maxBytes;
}

void Worm::SetName(std::string name)
{
    m_name = name;
}

void Worm::SetTotalNodes (uint32_t totalNodes)
{
    m_totalNodes = totalNodes;
}

void Worm::SetExistNodes (uint32_t existNodes)
{
    m_existNodes = existNodes;
}

void Worm::SetUp(std::string protocol, uint32_t infectionPort)
{
    m_protocol = protocol;
    m_infectionPort = infectionPort;
    m_typeId = ns3::TypeId::LookupByName(m_protocol);

    // Sink socket
    m_sinkSocket = ns3::Socket::CreateSocket (GetNode (), m_typeId); 
    NS_ASSERT (m_sinkSocket != 0);
    ns3::InetSocketAddress local = ns3::InetSocketAddress(ns3::Ipv4Address::GetAny(),
                                                          m_infectionPort);
    m_sinkSocket->Bind(local);
    m_sinkSocket->Listen();
    m_sinkSocket->SetRecvCallback (MakeCallback (&Worm::Listen, this));

}

ns3::Ipv4Address Worm::guessIP()
{
  uint32_t first, second;
  char buff[13];

  first = Worm::x.GetInteger(1,32);
  second = Worm::y.GetInteger(1,256);
  sprintf(buff, "192.%d.%d.2",first,second);

  ns3::Ipv4Address address = ns3::Ipv4Address(buff);
  //std::cerr << buff << std::endl;
  //std::cerr << address.Get() << std::endl;
  return address;
}

void
Worm::Write32 (uint8_t *buffer, const uint32_t data)
{
  NS_LOG_FUNCTION (this << buffer << data);
  buffer[0] = (data >> 0) & 0xff;
  buffer[1] = (data >> 8) & 0xff;
  buffer[2] = (data >> 16) & 0xff;
  buffer[3] = (data >> 24) & 0xff;
}

void
Worm::Read32 (const uint8_t *buffer, uint32_t &data)
{
  NS_LOG_FUNCTION (this << buffer << data);
  data = (buffer[3] << 24) + (buffer[2] << 16) + (buffer[1] << 8) + buffer[0];
}

void Worm::SendPacket()
{
    //std::cerr << "[" << m_name << "] Sending Packet" << std::endl;

    uint8_t* data = new uint8_t[m_pktSize];
    for (uint32_t i = 0; i < m_pktSize; ++i) data[i] = 0;

    uint32_t tmp = m_infectionPort;
    Write32 (&data[0 * sizeof(uint32_t)], tmp);

    ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet> ((uint8_t *) data, m_pktSize);

    m_onoffSocket->SendTo(packet, 0, ns3::InetSocketAddress(guessIP(), m_infectionPort));

    //std::cerr << "Test: " << testVal << std::endl;
    m_totalBytes += m_pktSize;
    m_lastStartTime = ns3::Simulator::Now();
    m_residualBits = 0;
    ScheduleNextTx ();
}

void Worm::ConnectionSucceeded(ns3::Ptr<ns3::Socket> socket)
{
  //std::cerr << "[" << m_name << "] Connection Succeeded" << std::endl;
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void Worm::ConnectionFailed(ns3::Ptr<ns3::Socket> socket)
{
  //std::cerr << "[" << m_name << "] Connection Failure" << std::endl;
  NS_LOG_FUNCTION (this << socket);
}

void Worm::ScheduleStartEvent ()
{  // Schedules the event to start sending data (switch to the "On" state)
  NS_LOG_FUNCTION (this);

  ns3::Time offInterval = ns3::Seconds (m_offTime->GetValue ());
  NS_LOG_LOGIC ("start at " << offInterval);
  ns3::Simulator::ScheduleNow(&Worm::StartSending, this);
}

void Worm::ScheduleStopEvent ()
{  // Schedules the event to stop sending data (switch to "Off" state)
  NS_LOG_FUNCTION (this);

  ns3::Time onInterval = ns3::Seconds (m_onTime->GetValue ());
  NS_LOG_LOGIC ("stop at " << onInterval);
  ns3::Simulator::Schedule (onInterval, &Worm::StopSending, this);
}

// Event handlers
void Worm::StartSending ()
{
    //std::cerr << "[" << m_name << "] Starting sending" << std::endl;
    NS_LOG_FUNCTION (this);
    m_lastStartTime = ns3::Simulator::Now ();
    ScheduleNextTx ();  // Schedule the send packet event
    //ScheduleStopEvent ();
}

// Private helpers
void Worm::ScheduleNextTx ()
{
    //std::cerr << "[" << m_name << "] Scheduling Next Tx" << std::endl;
  NS_LOG_FUNCTION (this);

  if (m_maxBytes == 0 || m_totalBytes < m_maxBytes)
    {
      uint32_t bits = m_pktSize * 8 - m_residualBits;
      NS_LOG_LOGIC ("bits = " << bits);
      ns3::Time nextTime (ns3::Seconds (bits /
                              static_cast<double>(m_cbrRate.GetBitRate ()))); // Time till next packet
      NS_LOG_LOGIC ("nextTime = " << nextTime);
      m_sendEvent = ns3::Simulator::Schedule (nextTime,
                                         &Worm::SendPacket, this);
    }
  else
    { // All done, cancel any pending events
      StopApplication ();
    }
}

void Worm::StopSending ()
{
    NS_LOG_FUNCTION (this);
    CancelEvents ();

    ScheduleStartEvent ();
}

// Application Methods
void Worm::StartApplication()    // Called at time specified by Start
{
    //std::cerr << "[" << m_name << "] Starting Application" << std::endl;

    // If we're vulnerable, if already infected, start infected other nodes
    // else listen for packets on infection port
    if (m_vulnerable) {
        if (m_infected) {
            StartInfectingNodes();
        }
    }
}

void Worm::Listen(ns3::Ptr<ns3::Socket> socket)
{
  //std::cerr << "Anything!" << std::endl;
  while (socket->GetRxAvailable () > 0)
    {
      ns3::Address from;
      ns3::Ptr<ns3::Packet> p = socket->RecvFrom (0xffffffff, 0, from);
      NS_ASSERT (ns3::InetSocketAddress::IsMatchingType (from));

      uint32_t recvSize = p->GetSize ();
      uint32_t *buffer = new uint32_t [recvSize];
      p->CopyData((uint8_t *)buffer, recvSize);
      uint32_t pktMsg;

      Read32 ((const uint8_t *) &buffer[0], pktMsg);
      //std::cerr << "[" << m_name << "] Received one packet!" << std::endl;
      if (((uint32_t)m_infectionPort == pktMsg) && (!m_infected))
        {
          m_infected = true;
          m_totalInfected++;
          std::cerr << m_totalInfected << " This worked! sort of" << std::endl;

          if (m_totalInfected >= m_existNodes) ns3::Simulator::Stop();

          StartInfectingNodes();
        }
    }
}


void Worm::StartInfectingNodes()
{
    // OnOff socket
    m_onoffSocket = ns3::Socket::CreateSocket (GetNode (), m_typeId);   
    NS_ASSERT (m_onoffSocket != 0);
    m_onoffSocket->Bind();

    m_onoffSocket->SetAllowBroadcast(false);

    m_onoffSocket->SetConnectCallback(ns3::MakeCallback(&Worm::ConnectionSucceeded, this),
                                        ns3::MakeCallback(&Worm::ConnectionFailed, this));
        //m_onoffSocket->Connect(ns3::InetSocketAddress(guessIP(),m_infectionPort));

        m_cbrRateFailSafe = m_cbrRate;

        //CancelEvents ();

        ScheduleStartEvent();

        ns3::Simulator::Schedule(ns3::Seconds(2.0),&Worm::StartInfectingNodes, this);
}

void Worm::CancelEvents ()
{
  NS_LOG_FUNCTION (this);

  if (m_sendEvent.IsRunning () && m_cbrRateFailSafe == m_cbrRate )
    { // Cancel the pending send packet event
      // Calculate residual bits since last packet sent
      ns3::Time delta (ns3::Simulator::Now () - m_lastStartTime);
      ns3::int64x64_t bits = delta.To (ns3::Time::S) * m_cbrRate.GetBitRate ();
      m_residualBits += bits.GetHigh ();
    }
  m_cbrRateFailSafe = m_cbrRate;
  ns3::Simulator::Cancel (m_sendEvent);
  ns3::Simulator::Cancel (m_startStopEvent);
}

void Worm::StopApplication()     // Called at time specified by Stop
{
    //std::cerr << "[" << m_name << "] Stopping Application" << std::endl;

    if (m_sinkSocket != 0) {
        m_sinkSocket->Close();
    }
    if (m_onoffSocket != 0) {
        m_onoffSocket->Close();
    }
    ns3::Simulator::Schedule(ns3::Seconds(2.0), &Worm::CloseAndPrint, this);
}

void Worm::CloseAndPrint()
{

}

void Worm::DoDispose()
{
    m_sinkSocket = 0;
    m_onoffSocket = 0;
    Application::DoDispose();
}

void Worm::PrintState()
{
    std::cerr << "[" << m_name << "] State at time " << ns3::Simulator::Now() << ":\n"
              << "\tInfected: " << (m_infected ? "True\n" : "False\n")
              << "\tVulnerable: " << (m_vulnerable ? "True\n" : "False\n")
              << std::endl;
}
