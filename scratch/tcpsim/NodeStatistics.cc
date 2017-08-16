
#include "NodeStatistics.h"

Time NodeStatistics::getAveragePacketSentReceiveTime() {
	if(NumberOfSuccessfulPacketsWithSeqHeader > 0)
		return TotalPacketSentReceiveTime / NumberOfSuccessfulPacketsWithSeqHeader;
	else
		return Time();
}

Time NodeStatistics::getAveragePacketRoundTripTime() {
	if(NumberOfSuccessfulRoundtripPacketsWithSeqHeader > 0)
		return TotalPacketRoundtripTime / NumberOfSuccessfulRoundtripPacketsWithSeqHeader;
	else
		return Time();
}

long NodeStatistics::getNumberOfDroppedPackets() {
	if(NumberOfSentPackets == 0)
		return -1;
	else
		return NumberOfSentPackets - NumberOfSuccessfulPackets;
}

double NodeStatistics::getGoodputKbit() {
	if(TotalPacketSentReceiveTime.GetSeconds() > 0)
		return (TotalPacketPayloadSize*8 / (1024)) / TotalPacketSentReceiveTime.GetSeconds();
	else
		return -1;
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

