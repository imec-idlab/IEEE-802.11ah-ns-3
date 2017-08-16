#include "Statistics.h"

Statistics::Statistics() {
}

Statistics::Statistics(int nrOfNodes) {
    for(int i = 0; i < nrOfNodes; i++)
        this->nodeStatistics.push_back(NodeStatistics());    
}
NodeStatistics& Statistics::get(int index) {
    return this->nodeStatistics.at(index);
}

int Statistics::getNumberOfNodes() const {
    return this->nodeStatistics.size();
}

