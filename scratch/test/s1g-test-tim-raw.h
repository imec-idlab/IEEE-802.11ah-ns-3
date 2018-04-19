#pragma once

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <fstream>
#include <sys/stat.h>
#include "ns3/rps.h"
#include <utility>
#include <map>

#include "Configuration.h"
#include "NodeEntry.h"
#include "SimpleTCPClient.h"
#include "Statistics.h"
#include "SimulationEventManager.h"

#include "TCPPingPongClient.h"
#include "TCPPingPongServer.h"
#include "TCPIPCameraClient.h"
#include "TCPIPCameraServer.h"
#include "TCPFirmwareClient.h"
#include "TCPFirmwareServer.h"
#include "TCPSensorClient.h"
#include "TCPSensorServer.h"

using namespace std;
using namespace ns3;

NodeContainer wifiStaNode;
NodeContainer wifiApNode;

Ipv4InterfaceContainer staNodeInterface;
Ipv6InterfaceContainer staNodeInterface6;

Ipv4InterfaceContainer apNodeInterface;
Ipv6InterfaceContainer apNodeInterface6;

NetDeviceContainer apDevice;

uint16_t currentRps;
uint16_t currentRawGroup;
uint16_t currentRawSlot;

vector<NodeEntry*> nodes;

vector<long> transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval;
vector<long> transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval;

ApplicationContainer serverApp;
uint32_t AppStartTime = 0;
uint32_t ApStopTime = 0;
std::map<uint16_t, float> traffic_sta;

void configureNodes();

void configureUDPServer();
void configureUDPClients();

void configureUDPEchoClients();
void configureUDPEchoServer();
void configureUdpEchoClientHelper(UdpEchoClientHelper& clientHelper);

void configureTCPEchoClients();
void configureTCPEchoServer();
void configureTCPEchoClientHelper(TcpEchoClientHelper& clientHelper);

void configureTCPPingPongServer();
void configureTCPPingPongClients();

void configureTCPIPCameraServer();
void configureTCPIPCameraClients();

void configureTCPFirmwareServer();
void configureTCPFirmwareClients();

void configureTCPSensorServer();
void configureTCPSensorClients();

void wireTCPServer(ApplicationContainer serverApp);
void wireTCPClient(ApplicationContainer clientApp, int i);

void onSTAAssociated(int i);
void onSTADeassociated(int i);

void onChannelTransmission(Ptr<NetDevice> senderDevice, Ptr<Packet> packet);

void updateNodesQueueLength();


int getSTAIdFromAddress(Ipv4Address from);

int main(int argc, char** argv);

void sendStatistics(bool schedule);

void configurePageSlice (void);
void configureTIM (void);
void checkRawAndTimConfiguration (void);
bool check (uint16_t aid, uint32_t index);
