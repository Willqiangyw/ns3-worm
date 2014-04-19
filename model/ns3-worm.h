
#ifndef WORM_APPLICATION_H
#define WORM_APPLICATION_H

#include "ns3/core-module.h"
#include "ns3/application.h"
#include "ns3/internet-module.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/socket.h"
#include "ns3/event-id.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"

//NS_LOG_COMPONENT_DEFINE ("ns3-worm");

/**
 * \class Worm Worm.h
 * \brief Worm application
 */
class Worm : public ns3::Application
{
public:
    /**
     * \brief Constructs worm application object
     */
    static ns3::TypeId GetTypeId(void);
    Worm();
    ~Worm();

    void SetUp(std::string protocol, ns3::Ipv4Address ipToInfect,
                     uint32_t infectionPort, ns3::Ptr<ns3::Node> nodeToInfect);
    void SetMaxBytes(uint32_t maxBytes);
    void PrintState();
    void SetInfected(bool alreadyInfected);
    void SetVulnerable(bool vulnerable);
    void SetName(std::string name);

protected:
    void DoDispose (void);


    bool m_infected;
    bool m_vulnerable;
    bool m_connected;

    uint16_t m_infectionPort;
    uint32_t m_pktSize;
    uint32_t m_maxBytes;
    uint32_t m_residualBits; //!< Number of generated, but not sent, bits
    uint32_t m_totalBytes;
    ns3::Time m_lastStartTime;


    std::string m_protocol;
    std::string m_name;

    ns3::EventId m_startStopEvent;
    ns3::EventId m_sendEvent;
    ns3::TypeId m_typeId;
    ns3::Address m_ipToInfect;
    ns3::Ptr<ns3::RandomVariableStream> m_onTime;       //!< rng for On Time
    ns3::Ptr<ns3::RandomVariableStream> m_offTime;      //!< rng for Off Time
    ns3::Ptr<ns3::Node> m_node;
    ns3::Ptr<ns3::Socket> m_sinkSocket;
    ns3::Ptr<ns3::Socket> m_onoffSocket;
    ns3::DataRate m_cbrRate;    //!< Rate that data is generated
    ns3::DataRate m_cbrRateFailSafe;

    ns3::TracedCallback<ns3::Ptr<const ns3::Packet> > m_txTrace;

private:
//     inherited from Application base class.
    virtual void StartApplication (void);    // Called at time specified by Start
    virtual void StopApplication (void);     // Called at time specified by Stop
    void StartInfectingNodes();
    void Listen();
    void CloseAndPrint();
    void ConnectionSucceeded(ns3::Ptr<ns3::Socket> socket);
    void ConnectionFailed(ns3::Ptr<ns3::Socket> socket);
    void ScheduleStartEvent();
    void ScheduleStopEvent();
    void SendPacket();
    void StartSending();
    void StopSending();
    void ScheduleNextTx();
    void CancelEvents();
};

#endif // WORM_APPLICATION_H
