
#ifndef WORM_APPLICATION_H
#define WORM_APPLICATION_H

#include "ns3/core-module.h"
#include "ns3/application.h"
#include "ns3/internet-module.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"

// Typedefs from GTNets
typedef unsigned long Size_t;
typedef unsigned long Count_t;
typedef uint16_t PortId_t;
typedef unsigned long Seq_t;
typedef ns3::Ipv4Address IPAddr_t; //typedef unsigned long IPAddr_t;
typedef double Time_t;

using namespace ns3;

class Address;
class RandomVariableStream;
class Socket;
class Packet;

/**
 * \class Worm Worm.h
 * \brief Worm application
 */
class Worm : public Application
{
public:
    /**
     * \brief Constructs worm application object
     */
    Worm(bool isVulnerable=true);
    ~Worm();



protected:
    virtual void DoDispose (void);

    bool infected;
    bool vulnerable;

private:
    // inherited from Application base class.
    virtual void StartApplication (void);    // Called at time specified by Start
    virtual void StopApplication (void);     // Called at time specified by Stop




//----------------------------------
//          Packet Sink
//----------------------------------

//    /**
//     * \brief Get the type ID.
//     * \return the object TypeId
//     */
//    static TypeId GetTypeId (void);
//    PacketSink ();

//    virtual ~PacketSink ();

    /**
     * \return the total bytes received in this sink app
     */
    uint32_t GetTotalRx () const;

    /**
     * \return pointer to listening socket
     */
    Ptr<Socket> GetListeningSocket (void) const;

    /**
     * \return list of pointers to accepted sockets
     */
    std::list<Ptr<Socket> > GetAcceptedSockets (void) const;

    /**
    * \brief Handle a packet received by the application
    * \param socket the receiving socket
    */
    void HandleRead (Ptr<Socket> socket);
    /**
    * \brief Handle an incoming connection
    * \param socket the incoming connection socket
    * \param from the address the connection is from
    */
    void HandleAccept (Ptr<Socket> socket, const Address& from);
    /**
    * \brief Handle an connection close
    * \param socket the connected socket
    */
    void HandlePeerClose (Ptr<Socket> socket);
    /**
    * \brief Handle an connection error
    * \param socket the connected socket
    */
    void HandlePeerError (Ptr<Socket> socket);

    // In the case of TCP, each socket accept returns a new socket, so the
    // listening socket is stored separately from the accepted sockets
    Ptr<Socket>     m_socket_rx;       //!< Listening socket
    std::list<Ptr<Socket> > m_socketList; //!< the accepted sockets

    Address         m_local;        //!< Local address to bind to
    uint32_t        m_totalRx;      //!< Total bytes received
    TypeId          m_tid;          //!< Protocol TypeId

    /// Traced Callback: received packets, source address.
    TracedCallback<Ptr<const Packet>, const Address &> m_rxTrace;

//----------------------------------
//           OnOff
//----------------------------------

    /**
     * \brief Set the total number of bytes to send.
     *
     * Once these bytes are sent, no packet is sent again, even in on state.
     * The value zero means that there is no limit.
     *
     * \param maxBytes the total number of bytes to send
     */
    void SetMaxBytes (uint32_t maxBytes);

    /**
     * \brief Return a pointer to associated socket.
     * \return pointer to associated socket
     */
    Ptr<Socket> GetSocket (void) const;

   /**
    * \brief Assign a fixed random variable stream number to the random variables
    * used by this model.
    *
    * \param stream first stream index to use
    * \return the number of stream indices assigned by this model
    */
    int64_t AssignStreams (int64_t stream);

    //helpers
    /**
     * \brief Cancel all pending events.
     */
    void CancelEvents ();

    // Event handlers
    /**
     * \brief Start an On period
     */
    void StartSending ();
    /**
     * \brief Start an Off period
     */
    void StopSending ();
    /**
     * \brief Send a packet
     */
    void SendPacket ();

    Ptr<Socket>     m_socket_tx;       //!< Associated socket
    Address         m_peer;         //!< Peer address
    bool            m_connected;    //!< True if connected
    Ptr<RandomVariableStream>  m_onTime;       //!< rng for On Time
    Ptr<RandomVariableStream>  m_offTime;      //!< rng for Off Time
    DataRate        m_cbrRate;      //!< Rate that data is generated
    DataRate        m_cbrRateFailSafe;      //!< Rate that data is generated (check copy)
    uint32_t        m_pktSize;      //!< Size of packets
    uint32_t        m_residualBits; //!< Number of generated, but not sent, bits
    Time            m_lastStartTime; //!< Time last packet sent
    uint32_t        m_maxBytes;     //!< Limit total number of bytes sent
    uint32_t        m_totBytes;     //!< Total bytes sent so far
    EventId         m_startStopEvent;     //!< Event id for next start or stop event
    EventId         m_sendEvent;    //!< Event id of pending "send packet" event
    TypeId          m_tid;          //!< Type of the socket used

    /// Traced Callback: transmitted packets.
    TracedCallback<Ptr<const Packet> > m_txTrace;

    /**
     * \brief Schedule the next packet transmission
     */
    void ScheduleNextTx ();
    /**
     * \brief Schedule the next On period start
     */
    void ScheduleStartEvent ();
    /**
     * \brief Schedule the next Off period start
     */
    void ScheduleStopEvent ();
    /**
     * \brief Handle a Connection Succeed event
     * \param socket the connected socket
     */
    void ConnectionSucceeded (Ptr<Socket> socket);
    /**
     * \brief Handle a Connection Failed event
     * \param socket the not connected socket
     */
    void ConnectionFailed (Ptr<Socket> socket);

};

#endif // WORM_APPLICATION_H
