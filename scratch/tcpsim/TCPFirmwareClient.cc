/*
 * TCPFirmwareClient.cc
 *
 *  Created on: Aug 9, 2016
 *      Author: dwight
 */

#include "TCPFirmwareClient.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TCPFirmwareClient");
NS_OBJECT_ENSURE_REGISTERED(TCPFirmwareClient);


TCPFirmwareClient::TCPFirmwareClient() {

	m_rv = CreateObject<UniformRandomVariable>();
}

TCPFirmwareClient::~TCPFirmwareClient() {

	m_rv = 0;
}

ns3::TypeId TCPFirmwareClient::GetTypeId(void) {
	static ns3::TypeId tid = ns3::TypeId("TCPFirmwareClient")
			.SetParent<TcpClient>()
			.AddConstructor<TCPFirmwareClient>()

			.AddAttribute ("VersionCheckInterval",
			                   "The interval to check versions",
			                   TimeValue (Seconds (1.0)),
			                   MakeTimeAccessor (&TCPFirmwareClient::m_interval),
			                   MakeTimeChecker ())

		   .AddAttribute("CorruptionProbability",
									   "The probability that the received block was corrupted",
									   DoubleValue(0.01),
									   MakeDoubleAccessor(&TCPFirmwareClient::corruptionProbability),
									   MakeDoubleChecker<double>(0.0,1.0))


		   .AddTraceSource("FirmwareUpdated",
					"Occurs when a firmware is completely received",
					MakeTraceSourceAccessor(&TCPFirmwareClient::firmwareUpdated),
					"TCPFirmwareClient::FirmwareUpdatedCallback")

	;
	return tid;
}

void TCPFirmwareClient::StartApplication(void) {
	ns3::TcpClient::StartApplication();

	stringBuffer = "";
	actionEventId = ns3::Simulator::Schedule(m_interval, &TCPFirmwareClient::Action, this);
}

void TCPFirmwareClient::StopApplication(void) {
	ns3::TcpClient::StopApplication();
	stringBuffer = "";
	ns3::Simulator::Cancel(actionEventId);
}

void TCPFirmwareClient::Action() {

	std::string str = "VERSION";

	NS_LOG_INFO("Sending current version to server");
	WriteString("VERSION,1.01.2345", false);
	WriteString("~~~", true);

	actionEventId = ns3::Simulator::Schedule(m_interval, &TCPFirmwareClient::Action, this);
}


void TCPFirmwareClient::OnDataReceived() {

	std::string data = ReadString(1024);
	stringBuffer += data;

	int splitIdx = stringBuffer.find("~~~");
	while(splitIdx != -1) {

		std::string msg = stringBuffer.substr(0, splitIdx);
		stringBuffer.erase(0, splitIdx + 3);

		if(msg.find("NEWUPDATE") == 0) {
			NS_LOG_INFO("Server has a new update for the client, requesting update");
			// schedule ?
			WriteString("READYTOUPDATE", false);
			WriteString("~~~", true);

			firmwareReceiveStarted = Simulator::Now();

			// cancel version check until firmware is updated
			ns3::Simulator::Cancel(actionEventId);
		}
		else if(msg.find("UPTODATE") == 0) {
			NS_LOG_INFO("Server replies the current client has the latest version");
		}
		else if(msg.find("BLOCK") == 0) {
			NS_LOG_INFO("Server has sent a block of size " << (int)(msg.size() - 5) << " of the firmware, checking if it's corrupted");
			bool corruption = m_rv->GetValue(0,1) < corruptionProbability;

			if(corruption) {
				NS_LOG_INFO("Block is corrupted, requesting it again");
				WriteString("BLOCKFAILED", false);
				WriteString("~~~", true);
			}
			else {
				NS_LOG_INFO("Block is OK, requesting next block");
				WriteString("NEXTBLOCK", false);
				WriteString("~~~", true);
			}
		}
		else if(msg.find("ENDOFUPDATE") == 0) {
			NS_LOG_INFO("Server has completely sent the firmware, updating");
			// schedule

			firmwareUpdated(Simulator::Now() - firmwareReceiveStarted);

			WriteString("UPDATED", false);
			WriteString("~~~", true);
			// restart version check
			actionEventId = ns3::Simulator::Schedule(m_interval, &TCPFirmwareClient::Action, this);
		}



		splitIdx = stringBuffer.find("~~~");
	}

}
