/*
 * TCPSensorServer.h
 *
 *  Created on: Aug 9, 2016
 *      Author: dwight
 */

#ifndef SCRATCH_AHSIMULATION_TCPSensorServer_H_
#define SCRATCH_AHSIMULATION_TCPSensorServer_H_

#include "ns3/application.h"
#include "ns3/core-module.h"
#include "ns3/tcp-server.h"
#include <map>

class TCPSensorServer: public ns3::TcpServer {
public:
	TCPSensorServer();
	virtual ~TCPSensorServer();

	static ns3::TypeId GetTypeId (void);


protected:
	virtual void OnDataReceived(ns3::Address from);

private:
	std::map<ns3::Address, std::string> partialBytesReceived;
};

#endif /* SCRATCH_AHSIMULATION_TCPSensorServer_H_ */
