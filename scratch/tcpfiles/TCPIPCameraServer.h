/*
 * TCPIPCameraServer.h
 *
 *  Created on: Aug 9, 2016
 *      Author: dwight
 */

#ifndef SCRATCH_AHSIMULATION_TCPIPCameraServer_H_
#define SCRATCH_AHSIMULATION_TCPIPCameraServer_H_

#include "ns3/application.h"
#include "ns3/core-module.h"
#include "ns3/tcp-server.h"
#include "ns3/traced-callback.h"

class TCPIPCameraServer: public ns3::TcpServer {
public:
	TCPIPCameraServer();
	virtual ~TCPIPCameraServer();

	static ns3::TypeId GetTypeId (void);

	typedef void (* DataReceivedCallback)
	                      (ns3::Address from, uint16_t nrOfBytes);

protected:
	virtual void OnDataReceived(ns3::Address from);

private:

	ns3::TracedCallback<ns3::Address, uint16_t> dataReceived;
};

#endif /* SCRATCH_AHSIMULATION_TCPIPCameraServer_H_ */
