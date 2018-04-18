#ifndef STATISTICS_H
#define STATISTICS_H

#include "NodeStatistics.h"

using namespace std;
using namespace ns3;

class Statistics {
private:
    vector<NodeStatistics> nodeStatistics;
    
public:
    Time TotalSimulationTime;
    Time TimeWhenEverySTAIsAssociated;
    
    Statistics();
    Statistics(int nrOfNodes);
    NodeStatistics& get(int index);

    int getNumberOfNodes() const;

};

#endif /* STATISTICS_H */

