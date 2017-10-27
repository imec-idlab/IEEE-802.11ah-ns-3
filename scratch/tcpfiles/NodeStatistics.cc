#include "NodeStatistics.h"
#include "Configuration.h"
#include <numeric>
#include <cmath>

long double NodeStatistics::getAveragePacketSentReceiveTime() { //milliseconds
	if(NumberOfSuccessfulPackets > 0)
		return static_cast<long double>(TotalPacketSentReceiveTime.GetMilliSeconds()) / NumberOfSuccessfulPackets;
	else
		return -1;
}

// This is jitter in milliseconds
long NodeStatistics::GetAverageJitter(void)
{
	if (NumberOfSuccessfulRoundtripPackets > 1)
		return sqrt(jitterAcc/(NumberOfSuccessfulRoundtripPackets - 1));
	else
		return -1;
}

double NodeStatistics::GetTotalEnergyConsumption (void)
{
	return this->EnergyRxIdle + this->EnergyTx; //mW
}

Time NodeStatistics::GetAverageInterPacketDelay(std::vector<Time>& delayVector){
	if (delayVector.size() != 0)
	{
		//cout << "inter packet delay vector size " << delayVector.size() << endl;
		return std::accumulate(delayVector.begin(), delayVector.end(), Seconds(0.0))/delayVector.size();
	}
	else return Time();
}

long double NodeStatistics::GetInterPacketDelayDeviation(std::vector<Time>& delayVector) // in microseconds
{
	long double meanInterPacketDelay (this->GetAverageInterPacketDelay(delayVector).GetMicroSeconds());
	long double dev (0);
	for (Time& delay : delayVector)
	{
		dev += (static_cast<long double>(delay.GetMicroSeconds()) - meanInterPacketDelay)*(static_cast<long double>(delay.GetMicroSeconds() - meanInterPacketDelay));
	}
	if (delayVector.size() > 0)
		return sqrt(dev/delayVector.size());
	else return -1; //implement exception handling for dummy nodes TODO
}

//reliability for one node or for all nodes in whole network? impossible with dummy nodes.
//try whole net when testing max nr of control loops
float NodeStatistics::GetPacketLoss (std::string trafficType)
{
	if (NumberOfSuccessfulRoundtripPackets > 0)
		return 100 - 100 * (float)NumberOfSuccessfulRoundtripPackets / NumberOfSentPackets;
	else if (NumberOfSuccessfulPackets > 0)
		return 100 - 100 * (float)NumberOfSuccessfulPackets / NumberOfSentPackets;
	else return -1;
}

long double NodeStatistics::GetInterPacketDelayDeviationPercentage(std::vector<Time>& delayVector){
	int64_t avg = GetAverageInterPacketDelay(delayVector).GetMicroSeconds();
	if (avg != 0)
		return (100*GetInterPacketDelayDeviation(delayVector)/avg);
	else
		return -1;
}


long double NodeStatistics::getAveragePacketRoundTripTime (std::string trafficType) {
	if(NumberOfSuccessfulRoundtripPackets > 0)
	{
		if (trafficType != "coap")
			return static_cast<long double>(TotalPacketRoundtripTime.GetMilliSeconds()) / NumberOfSuccessfulRoundtripPacketsWithSeqHeader;
		else
			return static_cast<long double>((TotalPacketRoundtripTime.GetMilliSeconds() + TotalPacketSentReceiveTime.GetMilliSeconds())) / NumberOfSuccessfulRoundtripPacketsWithSeqHeader;
		// In coap case TotalPacketRoundtripTime is not RTT but total time between the moment server sends a reply and a moment client receives it
		// same like TotalPacketSentReceiveTime just in the opposite direction
		// optimize the code and add new container for this value because the name is not representative
		// basically, true Total RTT = TotalPacketRoundtripTime + TotalPacketSentReceiveTime
	}
	else
		return -1;
}

long NodeStatistics::getNumberOfDroppedPackets() {
	if(NumberOfSentPackets == 0)
		return -1;
	else
		return NumberOfSentPackets - NumberOfSuccessfulPackets;
}

long double NodeStatistics::GetInterPacketDelayAtClient ()
{
	if (interPacketDelayAtClient.GetMilliSeconds() > 0)
	{
		return interPacketDelayAtClient.GetMilliSeconds();
	}
	else return -1;
}

long double NodeStatistics::GetInterPacketDelayAtServer ()
{
	if (interPacketDelayAtServer.GetMilliSeconds() > 0)
	{
		return interPacketDelayAtServer.GetMilliSeconds();
	}
	else return -1;
}

double NodeStatistics::getGoodputKbit(Time timeAllStationsAssociated) {
	if (Simulator::Now () > 0)
		return (TotalPacketPayloadSize * 8.) / ((Simulator::Now ().GetSeconds () - timeAllStationsAssociated.GetSeconds ()) * 1000);
	else return -1;
}


Time NodeStatistics::getAverageRemainingWhenAPSendingPacketInSameSlot() {
	if(NumberOfAPSentPacketForNodeImmediately == 0)
		return Time();
	else
		return APTotalTimeRemainingWhenSendingPacketInSameSlot / NumberOfAPSentPacketForNodeImmediately;
}


int NodeStatistics::getTotalDrops() {
	int sum = 0;
	for(auto& pair : this-> NumberOfDropsByReason) {
		sum += pair.second;
	}

	for(auto& pair : this-> NumberOfDropsByReasonAtAP) {
		sum += pair.second;
	}
	return sum;
}

double NodeStatistics::getIPCameraSendingRate() {
	if(TimeStreamStarted == Time(0))
		return -1;
	else {

		double elapsedSeconds;
		if(IPCameraTotalTimeSent == Time(0))
			elapsedSeconds = (Simulator::Now() -TimeStreamStarted).GetSeconds();
		else
			elapsedSeconds = IPCameraTotalTimeSent.GetSeconds();
		return (IPCameraTotalDataSent / elapsedSeconds) / 1024 * 8;
	}
}

double NodeStatistics::getIPCameraAPReceivingRate() {
	if(TimeStreamStarted == Time(0))
			return -1;
	else {

		double elapsedSeconds;
		if(IPCameraTotalTimeSent == Time(0))
			elapsedSeconds = (Simulator::Now() -TimeStreamStarted).GetSeconds();
		else
			elapsedSeconds = IPCameraTotalTimeSent.GetSeconds();

		return (IPCameraTotalDataReceivedAtAP / elapsedSeconds) / 1024 * 8;
	}
}


