
#ifndef WORM_APPLICATION_H
#define WORM_APPLICATION_H

#include "ns3/core-module.h"
#include "ns3/application.h"
#include "ns3/internet-module.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/on-off-helper.h"

NS_LOG_COMPONENT_DEFINE ("worm");

using namespace ns3;


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
    Worm(bool isInfected=false, bool isVulnerable=true);
    ~Worm();

    void SetUp(std::string protocol, ns3::Ipv4Address ipToInfect,
                     uint32_t infectionPort, ns3::Ptr<ns3::Node> nodeToInfect);

protected:
    void DoDispose (void);

    bool infected;
    bool vulnerable;

    uint16_t infectionPort;
    ns3::Ipv4Address ipToInfect;

private:
    // inherited from Application base class.
    void StartApplication (void);    // Called at time specified by Start
    void StopApplication (void);     // Called at time specified by Stop
    void StartInfectingNodes();
    void Listen();
    void CloseAndPrint();

    std::string protocol;
    ns3::OnOffHelper* onoffHelper;
    ns3::Ptr<ns3::Node> node;
    ns3::Ptr<ns3::Socket> sinkSocket;
};

#endif // WORM_APPLICATION_H
