/*
 * SimulationEventManager.h
 *
 *  Created on: Jun 30, 2016
 *      Author: dwight
 */

#ifndef SCRATCH_AHSIMULATION_SIMULATIONEVENTMANAGER_H_
#define SCRATCH_AHSIMULATION_SIMULATIONEVENTMANAGER_H_

#include "NodeEntry.h"
#include "Statistics.h"
#include "Configuration.h"
#include "ns3/drop-reason.h"
#include <fstream>
#include <memory>
#include "ns3/rps.h"

class SimulationEventManager {

private:

	string hostname;
	int port;
	string filename;

	Configuration m_config; ///ami
	int socketDescriptor = -1;

	void send(vector<string> str);

public:
	SimulationEventManager();
	SimulationEventManager(string hostname, int port, string filename);

    void onStartHeader();
	void onStart(Configuration& config);

	void onAPNodeCreated(double x, double y);

	void onSTANodeCreated(NodeEntry& node);

	void onNodeAssociated(NodeEntry& node);
	void onNodeDeassociated(NodeEntry& node);

	string SerializeDropReason(map<DropReason, long>& map);

	void onUpdateSlotStatistics(vector<long>& transmissionsPerSlotFromAP, vector<long>& transmissionsPerSlotFromSTA);

	void onStatisticsHeader();

	void onUpdateStatistics(Statistics& stats);

	void onRawConfig (uint32_t rpsIndex, uint32_t rawIndex, RPS::RawAssignment raw);

	virtual ~SimulationEventManager();
};

#endif /* SCRATCH_AHSIMULATION_SIMULATIONEVENTMANAGER_H_ */
