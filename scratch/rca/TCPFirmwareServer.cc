/*
 * TCPFirmwareServer.cc
 *
 *  Created on: Aug 9, 2016
 *      Author: dwight
 */

#include "TCPFirmwareServer.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TCPFirmwareServer");
NS_OBJECT_ENSURE_REGISTERED(TCPFirmwareServer);

TCPFirmwareServer::TCPFirmwareServer() {

	m_rv = CreateObject<UniformRandomVariable>();

}

TCPFirmwareServer::~TCPFirmwareServer() {
	m_rv = 0;
}

ns3::TypeId TCPFirmwareServer::GetTypeId(void) {
	static ns3::TypeId tid = ns3::TypeId("TCPFirmwareServer")
			.SetParent<TcpServer>()
			.AddConstructor<TCPFirmwareServer>()

			.AddAttribute ("FirmwareSize",
			                   "The size of the firmware to send",
			                   UintegerValue(1024 * 500),
			                   MakeUintegerAccessor(&TCPFirmwareServer::firmwareSize),
			                   MakeUintegerChecker<uint32_t>())

		   .AddAttribute ("BlockSize",
						   "The size of 1 chunk when sending the firmware",
						   UintegerValue(1024),
						   MakeUintegerAccessor(&TCPFirmwareServer::firmwareBlockSize),
						   MakeUintegerChecker<uint16_t>())

		   .AddAttribute("NewUpdateProbability",
									   "The probability that the received version is outdated",
									   DoubleValue(0.01),
									   MakeDoubleAccessor(&TCPFirmwareServer::newVersionProbability),
									   MakeDoubleChecker<double>(0.0,1.0))
	;
	return tid;
}

void TCPFirmwareServer::OnConnected(ns3::Address from) {
	stringBuffer[from] = "";
}

void TCPFirmwareServer::OnDataReceived(ns3::Address from) {

	std::string data = ReadString(from, 4096);
	stringBuffer[from] += data;

	int splitIdx = stringBuffer[from].find("~~~");
	while(splitIdx != -1) {
		std::string msg = stringBuffer[from].substr(0, splitIdx);
		 stringBuffer[from].erase(0, splitIdx + 3);

		if(msg.find("VERSION") == 0) {

			bool newerVersion = m_rv->GetValue(0,1) < newVersionProbability;
			if(newerVersion) {
				NS_LOG_INFO("New version for " << from << ", sending details");
				WriteString(from, std::string("NEWUPDATE,") + std::to_string(firmwareSize) + "," + std::to_string(firmwareBlockSize), false);
				WriteString(from, "~~~", true);
			}
			else {
				NS_LOG_INFO("No new version for " << from);
				WriteString(from, "UPTODATE", false);
				WriteString(from, "~~~", true);
			}
		}
		else if(msg.find("READYTOUPDATE") == 0) {
			NS_LOG_INFO(from << " has notified it is ready for the firmware update, sending firmware block");
			curFirmwarePos[from] = 0;
			SendFirmwareBlock(from);
		}
		else if(msg.find("NEXTBLOCK") == 0) {
			NS_LOG_INFO(from << " requests next firmware block, sending block " << (int)(curFirmwarePos[from] / firmwareBlockSize) << " of " << (int)(firmwareSize/firmwareBlockSize));
			curFirmwarePos[from] += firmwareBlockSize;
			SendFirmwareBlock(from);
		}
		else if(msg.find("BLOCKFAILED") == 0) {
			NS_LOG_INFO(from << " notifies it didn't receive the block correctly, resending firmware block");
			SendFirmwareBlock(from);
		}
		else if(msg.find("UPDATED") == 0) {
			NS_LOG_INFO(from << " is completely updated to the latest version");
			curFirmwarePos.erase(from);
		}



		splitIdx = stringBuffer[from].find("~~~");
	}
}

void TCPFirmwareServer::SendFirmwareBlock(Address from) {
	if(curFirmwarePos[from] > firmwareSize) {
		WriteString(from, "ENDOFUPDATE",false);
		WriteString(from, "~~~", true);
	}
	else {
		WriteString(from, "BLOCK", false);
		char* buf = new char[firmwareBlockSize];
		for(int i = 0; i < firmwareBlockSize; i++)
			buf[i] = i % 256;
		Write(from, buf, firmwareBlockSize);
		WriteString(from, "~~~", true);
		delete buf;
	}
}


