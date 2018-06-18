#pragma once

#ifndef NODEENTRY_H
#define NODEENTRY_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/extension-headers.h"
#include <functional>
#include "Statistics.h"
#include "ns3/drop-reason.h"
#include "ns3/tcp-socket.h"
#include "ns3/log.h"


using namespace ns3;

class NodeEntry {
public:
    int id;

    uint32_t aId = 8192; // unassociated is 8192
    uint16_t rpsIndex = 0;
    uint8_t rawGroupNumber = 0;
    uint8_t rawSlotIndex = 0;
    
    double x = 0;
    double y = 0;
    bool isAssociated = false;
    uint32_t queueLength = 0;

    static Time maxLatency;
    static Time minLatency;
    static Time minJitter;
    static Time maxJitter;
    NodeEntry(int id, Statistics* stats,Ptr<Node> node, Ptr<NetDevice> device);

    virtual ~NodeEntry();

    enum NodeType {
    	CLIENT,
		SERVER,
		DUMMY
    } m_nodeType;
    
    void SetAssociation(std::string context, Mac48Address address);
    void UnsetAssociation(std::string context, Mac48Address address);
    void OnS1gBeaconMissed(std::string context,bool nextBeaconIsDTIM);
    void OnNrOfTransmissionsDuringRAWSlotChanged(std::string context, uint16_t oldValue, uint16_t newValue);


    void OnPhyTxBegin(std::string context, Ptr<const Packet> packet);
    void OnPhyTxEnd(std::string context,Ptr<const Packet> packet);
    void OnPhyTxDrop(std::string context,Ptr<const Packet> packet, DropReason reason);
    
    void OnPhyRxBegin(std::string context,Ptr<const Packet> packet);
    void OnPhyRxEnd(std::string context,Ptr<const Packet> packet);
    void OnPhyRxDrop(std::string context,Ptr<const Packet> packet, DropReason reason);


    void OnMacPacketDropped(std::string context, Ptr<const Packet> packet, DropReason reason);
    void OnMacTxRtsFailed(std::string context,Mac48Address address);
    void OnMacTxDataFailed(std::string context,Mac48Address address);
    void OnMacTxFinalRtsFailed(std::string context,Mac48Address address);
    void OnMacTxFinalDataFailed(std::string context,Mac48Address address);


    void OnCollision(std::string context, uint32_t nrOfBackoffSlots);
    void OnTransmissionWillCrossRAWBoundary(std::string context, Time txDuration, Time remainingTimeInRawSlot);

    void OnPhyStateChange(std::string context, const Time start, const Time duration, const WifiPhy::State state);

    void OnTcpPacketSent(Ptr<const Packet> packet);
    void OnTcpPacketDropped(Ptr<Packet> packet, DropReason reason);

    void OnTcpFirmwareUpdated(Time totalFirmwareTransferTime);

    void OnTcpIPCameraStreamStateChanged(bool newStateIsStreaming);
    void OnTcpIPCameraDataSent(uint16_t nrOfBytes);
    void OnTcpIPCameraDataReceivedAtAP(uint16_t nrOfBytes);

    void OnTcpEchoPacketReceived(Ptr<const Packet> packet, Address from);
    void OnTcpPacketReceivedAtAP(Ptr<const Packet> packet);
    void OnTcpCongestionWindowChanged(uint32_t oldval, uint32_t newval);
    void OnTcpRTOChanged(Time oldval, Time newval);
    void OnTcpRTTChanged(Time oldval, Time newval);

    void OnTcpStateChanged(TcpSocket::TcpStates_t oldval, TcpSocket::TcpStates_t newval);
    void OnTcpStateChangedAtAP(TcpSocket::TcpStates_t oldval, TcpSocket::TcpStates_t newval);

    void OnTcpRetransmission(Address to);
    void OnTcpRetransmissionAtAP();

    void OnTcpSlowStartThresholdChanged(uint32_t oldVal,uint32_t newVal);
    void OnTcpEstimatedBWChanged(double oldVal, double newVal);



    void OnUdpPacketSent(Ptr<const Packet> packet);
    void OnUdpEchoPacketReceived(Ptr<const Packet> packet, Address from);
    void OnUdpPacketReceivedAtAP(Ptr<const Packet> packet);
    void OnCoapPacketReceivedAtServer(Ptr<const Packet> packet);
    void OnCoapPacketSent(Ptr<const Packet> packet); ///ami
    void OnCoapPacketReceived(Ptr<const Packet> packet, Address from);

    void UpdateQueueLength();
    
    void SetAssociatedCallback(std::function<void()> assocCallback);
    void SetDeassociatedCallback(std::function<void()> assocCallback);

private:
    Statistics* stats;
	Ptr<Node> node;
	Ptr<NetDevice> device;

    std::function<void()> associatedCallback;
    std::function<void()> deAssociatedCallback;
    std::map<uint64_t, Time> txMap;
    std::map<uint64_t, Time> rxMap;

    uint16_t lastBeaconAIDStart = 0;
    uint16_t lastBeaconAIDEnd = 0;

    bool rawTIMGroupFlaggedAsDataAvailableInDTIM = false;

    Time lastBeaconReceivedOn = Time();


    void OnEndOfReceive(Ptr<const Packet> packet);
    void UpdateJitter (Time timeDiff);

    bool tcpConnectedAtSTA = true;
    bool tcpConnectedAtAP = true;

    Time timeStreamStarted;

    Time delayFirst = Time();
    Time delaySecond = Time();
    Time timeSent = Time();
};

#endif /* NODEENTRY_H */

