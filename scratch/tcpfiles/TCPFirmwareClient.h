

#ifndef SCRATCH_APPLICATIONS_PINGPONG_TCPFirmwareClient_H_
#define SCRATCH_APPLICATIONS_PINGPONG_TCPFirmwareClient_H_


#include "ns3/tcp-client.h"
#include "ns3/application.h"
#include "ns3/core-module.h"
#include "ns3/traced-callback.h"


class TCPFirmwareClient : public ns3::TcpClient {
public:
	TCPFirmwareClient();
	virtual ~TCPFirmwareClient();

	static ns3::TypeId GetTypeId (void);


	typedef void (* FirmwareUpdatedCallback)
	                      (ns3::Time totalFirmwareTransferTime);




protected:
	virtual void StartApplication(void);
	virtual void StopApplication(void);
	virtual void OnDataReceived();

private:
	void Action();

	ns3::Time m_interval;

	std::string stringBuffer;

	double corruptionProbability;
	ns3::Ptr<ns3::UniformRandomVariable> m_rv;

	ns3::Time firmwareReceiveStarted;

	ns3::EventId actionEventId;

	ns3::TracedCallback<ns3::Time> firmwareUpdated;
};

#endif /* SCRATCH_APPLICATIONS_PINGPONG_TCPFirmwareClient_H_ */
