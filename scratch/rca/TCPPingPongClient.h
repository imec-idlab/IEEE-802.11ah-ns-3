

#ifndef SCRATCH_APPLICATIONS_PINGPONG_TCPPINGPONGCLIENT_H_
#define SCRATCH_APPLICATIONS_PINGPONG_TCPPINGPONGCLIENT_H_


#include "ns3/tcp-client.h"
#include "ns3/application.h"
#include "ns3/core-module.h"


class TCPPingPongClient : public ns3::TcpClient {
public:
	TCPPingPongClient();
	virtual ~TCPPingPongClient();

	static ns3::TypeId GetTypeId (void);

protected:
	virtual void StartApplication(void);
	virtual void OnDataReceived();

private:
	void Action();

	ns3::Time m_interval;

};

#endif /* SCRATCH_APPLICATIONS_PINGPONG_TCPPINGPONGCLIENT_H_ */
