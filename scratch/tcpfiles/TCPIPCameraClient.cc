
#include "TCPIPCameraClient.h"

using namespace ns3;

TCPIPCameraClient::TCPIPCameraClient() {

}

TCPIPCameraClient::~TCPIPCameraClient() {

}

ns3::TypeId TCPIPCameraClient::GetTypeId(void) {
	static ns3::TypeId tid = ns3::TypeId("TCPIPCameraClient")
			.SetParent<TcpClient>()
			.AddConstructor<TCPIPCameraClient>()

			.AddAttribute ("Interval",
			                   "The time to check for motion",
			                   TimeValue (Seconds (1.0)),
			                   MakeTimeAccessor (&TCPIPCameraClient::m_interval),
			                   MakeTimeChecker ())

		   .AddAttribute("MotionPercentage",
									   "The probability motion occurred when checking",
									   DoubleValue(0.01),
									   MakeDoubleAccessor(&TCPIPCameraClient::m_motionProbability),
									   MakeDoubleChecker<double>(0.0,1.0))

		   .AddAttribute("MotionDuration",
					   "The time to stream data when motion is detected",
					   TimeValue (Seconds (60.0)),
					   MakeTimeAccessor (&TCPIPCameraClient::m_motionDuration),
						   MakeTimeChecker ())

		   .AddAttribute("DataRate",
						   "The datarate in kbps",
						   IntegerValue(200),
						   MakeUintegerAccessor(&TCPIPCameraClient::m_datarate),
						   MakeUintegerChecker<uint16_t>())


		   .AddTraceSource("DataSent",
										"Occurs when data is sent to the server",
										MakeTraceSourceAccessor(&TCPIPCameraClient::dataSent),
										"TCPIPCameraClient::DataSentCallback")

			.AddTraceSource("StreamStateChanged",
													"Occurs when either started or stopped streaming",
													MakeTraceSourceAccessor(&TCPIPCameraClient::streamStateChanged),
													"TCPIPCameraClient::StreamStateChangedCallback")
	;
	return tid;
}

void TCPIPCameraClient::StartApplication(void) {
	ns3::TcpClient::StartApplication();

	m_rv = CreateObject<UniformRandomVariable> ();

	m_schedule = ns3::Simulator::Schedule(m_interval, &TCPIPCameraClient::Action, this);
}

void TCPIPCameraClient::StopApplication(void) {
	 ns3::Simulator::Cancel((const ns3::EventId)m_schedule);
}

void TCPIPCameraClient::Action() {

	if(m_rv->GetValue(0,1) < m_motionProbability) {


		// start or reset motion started
		motionStartedOn = Simulator::Now();
		if(!motionActive) {
			//std::cout << "Motion detected, starting camera stream" << std::endl;
			motionActive = true;
			streamStateChanged(true);
			Stream();
		}else {
			//std::cout << "Motion detected, resetting motion timer" << std::endl;
		}
	}

	m_schedule = ns3::Simulator::Schedule(m_interval, &TCPIPCameraClient::Action, this);
}

void TCPIPCameraClient::Stream() {
	// stop transmitting if duration of motion has expired
	if((Simulator::Now() - motionStartedOn) > m_motionDuration) {
		//std::cout << "No motion for " << m_motionDuration << ", stopping stream" << std::endl;
		motionActive = false;
		streamStateChanged(false);
		return;
	}

	int msPerSend = 100;
	// data to send every 100 ms is (DataRate * 1024 / 8) / 10 bytes

	int bytesToSend = (m_datarate * 1024 / 8) / (1000/msPerSend);

	char* buffer = new char[bytesToSend];
	for(int i = 0; i < bytesToSend; i++) {
		buffer[i] = i % 256;
	}
	//std::cout << "Writing " << bytesToSend << " to buffer " << std::endl;
	Write(buffer, bytesToSend);
	dataSent(bytesToSend);

	delete buffer;

	ns3::Simulator::Schedule(MilliSeconds(msPerSend), &TCPIPCameraClient::Stream, this);
}

void TCPIPCameraClient::OnDataReceived() {

	char* buf = new char[1024];
	while(int nrOfBytesRead = Read(buf, 1024)) {
	}
	delete buf;

	// not expecting any data from the server
}
