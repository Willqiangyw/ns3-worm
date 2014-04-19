
#include "ns3-worm.h"
#include "ns3/log.h"
#include "ns3/on-off-helper.h"

//// By default we are setting the target vector to be NULL
//// The code then selects the uniform target vector object by default

//WormTargetVector* Worm::defaultTV = NULL;

// Constructors

Worm::Worm(bool isInfected, bool isVulnerable)
  : infected(isInfected), vulnerable(isVulnerable)
{

}

void Worm::SetUp(std::string protocol, ns3::Ipv4Address ipToInfect,
                 uint32_t infectionPort, ns3::Ptr<ns3::Node> nodeToInfect)
{
    node = nodeToInfect;

    // OnOff Helper
    onoffHelper = new ns3::OnOffHelper(protocol,
                                        ns3::InetSocketAddress(ipToInfect,
                                        infectionPort));

    TypeId tid = TypeId::LookupByName(protocol);
    sinkSocket = ns3::Socket::CreateSocket(node, tid);
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), infectionPort);
    sinkSocket->Bind(local);
}

void Worm::Listen()
{
    // Wait until socket receives packet on infection port,
    // and then start infecting other nodes
    while(!infected) {
        if(sinkSocket->Listen() == 0) {
            infected = true;
        }
    }
    NS_LOG_INFO ("Received one packet!");
    if (!vulnerable) {
        infected = true;
        StartInfectingNodes();
    }
}

void Worm::StartInfectingNodes()
{
    ns3::ApplicationContainer onOffContainer;
//    onOffContainer.Add(onoffHelper->Install(node));
    onOffContainer.Start(ns3::Simulator::Now());
}

// Application Methods
void Worm::StartApplication()    // Called at time specified by Start
{
    // If we're vulnerable, if already infected, start infected other nodes
    // else listen for packets on infection port
    if (vulnerable) {
        if (infected) {
            StartInfectingNodes();
        } else {
            Listen();
        }
    }
}

void Worm::StopApplication()     // Called at time specified by Stop
{
    ns3::Simulator::Schedule(ns3::Seconds(2.0), &Worm::CloseAndPrint, this);
}

void Worm::CloseAndPrint()
{

}

void Worm::DoDispose()
{
    Application::DoDispose();
}
