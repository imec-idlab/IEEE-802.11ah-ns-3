/*
 * TCPFirmwareServer.h
 *
 *  Created on: Aug 9, 2016
 *      Author: dwight
 */

#ifndef SCRATCH_AHSIMULATION_TCPFirmwareServer_H_
#define SCRATCH_AHSIMULATION_TCPFirmwareServer_H_

#include "ns3/application.h"
#include "ns3/core-module.h"
#include "ns3/tcp-server.h"
#include <map>

using namespace ns3;

class TCPFirmwareServer: public ns3::TcpServer {
public:
	TCPFirmwareServer();
	virtual ~TCPFirmwareServer();

	static ns3::TypeId GetTypeId (void);

protected:
	virtual void OnDataReceived(ns3::Address from);

	virtual void OnConnected(ns3::Address from);

private:

	void SendFirmwareBlock(Address from);

	ns3::Ptr<ns3::UniformRandomVariable> m_rv;

	uint32_t firmwareSize;
	uint16_t firmwareBlockSize;
	double newVersionProbability;

	std::map<ns3::Address, uint32_t> curFirmwarePos;

	std::map<ns3::Address, std::string> stringBuffer;

};

#endif /* SCRATCH_AHSIMULATION_TCPFirmwareServer_H_ */
