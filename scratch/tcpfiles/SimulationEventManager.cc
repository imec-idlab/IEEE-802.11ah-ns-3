/*
 * SimulationEventManager.cc
 *
 *  Created on: Jun 30, 2016
 *      Author: dwight
 */

#include "SimulationEventManager.h"
#include "SimpleTCPClient.h"

SimulationEventManager::SimulationEventManager()
	: hostname("localhost"), port(7707), filename("") {
}

SimulationEventManager::SimulationEventManager(string hostname, int port, string filename)
	: hostname(hostname), port(port), filename(filename) {

	if(filename != "") {
		//delete old file
		std::remove(filename.c_str());
	}
}


void SimulationEventManager::onStart(Configuration& config) {
	m_config = config;
	send({"start",
		  std::to_string(config.NRawSta),
		  config.DataMode,
		  "",
		  "",

		  std::to_string(config.trafficInterval),
		  std::to_string(config.payloadSize),

		  std::to_string(config.BeaconInterval),

		  config.name,

		  std::to_string(config.propagationLossExponent),
		  std::to_string(config.propagationLossReferenceLoss),
		  std::to_string(config.APAlwaysSchedulesForNextSlot),
		  std::to_string(config.MinRTO),
		  std::to_string(config.simulationTime),
		  config.trafficType,
		  std::to_string(config.trafficIntervalDeviation),
		  std::to_string(config.TCPSegmentSize),
		  std::to_string(config.TCPInitialSlowStartThreshold),
		  std::to_string(config.TCPInitialCwnd),

		  std::to_string(config.MaxTimeOfPacketsInQueue),
		  std::to_string(config.ipcameraMotionPercentage),
		  std::to_string(config.ipcameraMotionDuration),
		  std::to_string(config.ipcameraDataRate),
		  std::to_string(config.Nsta),
		  std::to_string(config.CoolDownPeriod),
		  std::to_string(config.firmwareSize),
		  std::to_string(config.firmwareBlockSize),
		  std::to_string(config.firmwareCorruptionProbability),
		  std::to_string(config.firmwareNewUpdateProbability),
		  std::to_string(config.sensorMeasurementSize),

		  std::to_string(config.NGroup),
		  std::to_string(config.SlotFormat),
		  std::to_string(config.NRawSlotCount),
		  //std::to_string(config.NRawSlotCount * 120 + 500),
		  std::to_string(config.NRawSlotNum),
		  std::to_string(config.ContentionPerRAWSlot),
		  std::to_string(config.ContentionPerRAWSlotOnlyInFirstGroup),
		  config.TrafficPath,
		  std::to_string(config.rps.rpsset.size())
		  //config.RawConfigString
	});
}


void SimulationEventManager::onAPNodeCreated(double x, double y) {
	send({"apnodeadd", std::to_string(x), std::to_string(y)});
}

void SimulationEventManager::onSTANodeCreated(NodeEntry& node) {
	send({"stanodeadd", std::to_string(node.id), std::to_string(node.x), std::to_string(node.y), std::to_string(node.aId)});
}

void SimulationEventManager::onNodeAssociated(NodeEntry& node) {
	send({"stanodeassoc", std::to_string(node.id), std::to_string(node.aId), std::to_string(node.rpsIndex), std::to_string(node.rawGroupNumber), std::to_string(node.rawSlotIndex)}); //TODO rawGroupNumber not known in assoc
}

void SimulationEventManager::onNodeDeassociated(NodeEntry& node) {
	send({"stanodedeassoc", std::to_string(node.id)});
}

string SimulationEventManager::SerializeDropReason(map<DropReason, long>& map) {

	int lastItem = DropReason::TCPTxBufferExceeded;
	std::stringstream s;
	for(int i = 0; i <= lastItem;i++) {
		s << map[(DropReason)i] << ((i == lastItem) ? "": ",");
	}
	return s.str();
}

void SimulationEventManager::onUpdateSlotStatistics(vector<long>& transmissionsPerSlotFromAP, vector<long>& transmissionsPerSlotFromSTA) {

	vector<string> values;

	values.push_back("slotstatsAP");
	for(uint32_t i = 0; i < transmissionsPerSlotFromAP.size(); i++) {
		values.push_back(std::to_string(transmissionsPerSlotFromAP[i]));
	}
	send(values);

	values.clear();

	values.push_back("slotstatsSTA");
	for(uint32_t i = 0; i < transmissionsPerSlotFromSTA.size(); i++) {
		values.push_back(std::to_string(transmissionsPerSlotFromSTA[i]));
	}
	send(values);
}

void SimulationEventManager::onUpdateStatistics(Statistics& stats) {
	for(int i = 0; i < stats.getNumberOfNodes(); i++) {
		send({"nodestats", std::to_string(i),
			std::to_string(stats.get(i).TotalTxTime.GetMilliSeconds()),
			std::to_string(stats.get(i).TotalRxTime.GetMilliSeconds()),
			std::to_string(stats.get(i).TotalSleepTime.GetMilliSeconds()),
			std::to_string(stats.get(i).TotalIdleTime.GetMilliSeconds()),
			std::to_string(stats.get(i).NumberOfTransmissions),
			std::to_string(stats.get(i).NumberOfTransmissionsDropped),
			std::to_string(stats.get(i).NumberOfReceives),
			std::to_string(stats.get(i).NumberOfReceivesDropped),
			std::to_string(stats.get(i).NumberOfSentPackets),
			std::to_string(stats.get(i).NumberOfSuccessfulPackets),
			std::to_string(stats.get(i).getNumberOfDroppedPackets()),
			std::to_string(stats.get(i).getAveragePacketSentReceiveTime()),
			std::to_string(stats.get(i).getGoodputKbit(stats.TimeWhenEverySTAIsAssociated)),
			std::to_string(stats.get(i).EDCAQueueLength),
			std::to_string(stats.get(i).NumberOfSuccessfulRoundtripPackets),
			std::to_string(stats.get(i).getAveragePacketRoundTripTime(m_config.trafficType)),
			std::to_string(stats.get(i).TCPCongestionWindow),
			std::to_string(stats.get(i).NumberOfTCPRetransmissions),
			std::to_string(stats.get(i).NumberOfTCPRetransmissionsFromAP),
			std::to_string(stats.get(i).NumberOfReceiveDroppedByDestination),
			std::to_string(stats.get(i).NumberOfMACTxRTSFailed),
			std::to_string(stats.get(i).NumberOfMACTxMissedACK),
			this->SerializeDropReason(stats.get(i).NumberOfDropsByReason),
			this->SerializeDropReason(stats.get(i).NumberOfDropsByReasonAtAP),
			std::to_string(stats.get(i).TCPRTOValue.GetMicroSeconds() == 0 ? -1 : stats.get(i).TCPRTOValue.GetMicroSeconds()),
			std::to_string(stats.get(i).NumberOfAPScheduledPacketForNodeInNextSlot),
			std::to_string(stats.get(i).NumberOfAPSentPacketForNodeImmediately),
			std::to_string(stats.get(i).getAverageRemainingWhenAPSendingPacketInSameSlot().GetMicroSeconds()),
			std::to_string(stats.get(i).NumberOfCollisions),
			std::to_string(stats.get(i).NumberOfMACTxMissedACKAndDroppedPacket),
			(stats.get(i).TCPConnected ? "1" : "0"),
			std::to_string(stats.get(i).TCPSlowStartThreshold),
			std::to_string(stats.get(i).TCPEstimatedBandwidth),
			std::to_string(stats.get(i).TCPRTTValue.GetMicroSeconds() == 0 ? -1 : stats.get(i).TCPRTTValue.GetMicroSeconds()),
			std::to_string(stats.get(i).NumberOfBeaconsMissed),
			std::to_string(stats.get(i).NumberOfTransmissionsDuringRAWSlot),
			std::to_string(stats.get(i).getTotalDrops()),
			std::to_string(stats.get(i).FirmwareTransferTime.GetMicroSeconds()),
			std::to_string(stats.get(i).getIPCameraSendingRate()),
			std::to_string(stats.get(i).getIPCameraAPReceivingRate()),
			std::to_string(stats.get(i).NumberOfTransmissionsCancelledDueToCrossingRAWBoundary),
			std::to_string(stats.get(i).GetAverageJitter()), // I have jitter in micros abs delay between subsequent packets
			std::to_string(stats.get(i).GetPacketLoss(m_config.trafficType)),
			std::to_string(stats.get(i).GetInterPacketDelayAtServer()),
			std::to_string(stats.get(i).GetInterPacketDelayAtClient()),
			std::to_string(stats.get(i).GetInterPacketDelayDeviationPercentage(stats.get(i).m_interPacketDelayServer)),
			std::to_string(stats.get(i).GetInterPacketDelayDeviationPercentage(stats.get(i).m_interPacketDelayClient)),
			std::to_string(stats.get(i).latency.GetMilliSeconds()),
			std::to_string(stats.get(i).EnergyRxIdle),
			std::to_string(stats.get(i).EnergyTx)
		});
	}
}

void SimulationEventManager::send(vector<string> str) {

	std::stringstream s;
	s << Simulator::Now().GetNanoSeconds() << ";";
	for(uint32_t i = 0; i < str.size(); i++) {
		s << str[i] << ((i != str.size()-1) ? ";" : "");
	}
	s << "\n";

	if(this->filename != "" && this->filename != "none") {
		ofstream fileStream(filename,fstream::out|fstream::app);
		if(fileStream.is_open())
			fileStream << s.str();
		// append to file
		fileStream.close();
	}

	if(this->hostname != "" && this->hostname != "none") {

		//int sockfd ;
		if(socketDescriptor == -1) {
			std::cout << "Connecting to visualizer" << std::endl;
			socketDescriptor = stat_connect(this->hostname.c_str(), std::to_string(this->port).c_str());
			if(socketDescriptor == -1)
				return;
		}

		bool success = stat_send(socketDescriptor, string(s.str()).c_str());

		if(!success) {
			std::cout << "Sending failed" << std::endl;
			stat_close(socketDescriptor);
			socketDescriptor = -1;
		}
	}
}

void SimulationEventManager::onRawConfig (uint32_t rpsIndex, uint32_t rawIndex, RPS::RawAssignment raw)
{

	send({"rawconfig", std::to_string(rpsIndex), std::to_string(rawIndex),
		std::to_string(raw.GetSlotFormat()),
		std::to_string(raw.GetSlotDurationCount()),
		std::to_string(raw.GetSlotNum()),
		std::to_string(raw.GetSlotCrossBoundary()),
		std::to_string(raw.GetRawGroupAIDStart()),
		std::to_string(raw.GetRawGroupAIDEnd())
	});
}

void SimulationEventManager::onStartHeader() {
	send({"startheader",
		   "NRawSta",
		   "DataMode",
		   "",
		   "",
		   "TrafficInterval",
		   "PayloadSize",
		   "BeaconInterval",
		   "Name",
		   "PropagationLossExponent",
		   "PropagationLossReferenceLoss",
		   "APAlwaysSchedulesForNextSlot",
		   "MinRTO",
		   "SimulationTime",
		   "TrafficType",
		   "TrafficIntervalDeviation",
		   "TCPSegmentSize",
		   "TCPInitialSlowStartThreshold",
		   "TCPInitialCwnd",
		   "MaxTimeOfPacketsInQueue",
		   "IPCameraMotionPercentage",
		   "IPCameraMotionDuration",
		   "IPCameraDataRate",
		   "NSta",
		   "CoolDownPeriod",
		   "FirmwareSize",
		   "FirmwareBlockSize",
		   "FirmwareCorruptionProbability",
		   "FirmwareNewUpdateProbability",
		   "SensorMeasurementSize",
		   "NGroup",
		   "SlotFormat",
		   "NRawSlotCount",
		   "NRawSlotNum",
		   "ContentionPerRAWSlot",
		   "ContentionPerRAWSlotOnlyInFirstGroup",
		   "TrafficString",
		   "NumOfRpsElements"
		});
}


void SimulationEventManager::onStatisticsHeader() {
	send({"nodestatsheader", "STAIndex",
		"TotalTransmitTime",
		"TotalReceiveTime",
		"TotalSleepTime", //doye
		"TotalIdleTime", // active
		"NumberOfTransmissions",
		"NumberOfTransmissionsDropped",
		"NumberOfReceives",
		"NumberOfReceivesDropped",
		"NumberOfSentPackets",
		"NumberOfSuccessfulPackets",
		"NumberOfDroppedPackets",
		"AveragePacketSentReceiveTime",
		"GoodputKbit",
		"EDCAQueueLength",
		"NumberOfSuccessfulRoundtripPackets",
		"AveragePacketRoundTripTime",
		"TCPCongestionWindow",
		"NumberOfTCPRetransmissions",
		"NumberOfTCPRetransmissionsFromAP",
		"NumberOfReceiveDroppedByDestination",
		"NumberOfMACTxRTSFailed",
		"NumberOfMACTxMissedACK",
		"NumberOfDropsByReason",
		"NumberOfDropsByReasonAtAP",
		"TCPRTOValue",
		"NumberOfAPScheduledPacketForNodeInNextSlot",
		"NumberOfAPSentPacketForNodeImmediately",
		"AverageTimeRemainingWhenAPSendingPacketInSameSlot",
		"NumberOfCollisions",
		"NumberOfMACTxMissedACKAndDroppedPacket",
		"TCPConnected",
		"TCPSlowStartThreshold",
		"TCPEstimatedBandwidth",
		"TCPRTTValue",
		"NumberOfBeaconsMissed",
		"NumberOfTransmissionsDuringRAWSlot",
		"TotalNumberOfDrops",
		"FirmwareTransferTime",
		"IPCameraSendingRate",
		"IPCameraReceivingRate",
		"NumberOfTransmissionsCancelledDueToCrossingRAWBoundary",
		"Jitter",
		"PacketLoss",
		"InterPacketDelayAtServer",
		"InterPacketDelayAtClient",
		"InterPacketDelayDeviationPercentageAtServer",
		"InterPacketDelayDeviationPercentageAtClient",
		"Latency",
		"EnergyRxIdle",
		"EnergyTx"
	});

}

SimulationEventManager::~SimulationEventManager() {

}

