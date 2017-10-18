/*
 * drop-reason.h
 *
 *  Created on: Jul 11, 2016
 *      Author: dwight
 */

#ifndef SRC_WIFI_MODEL_DROP_REASON_H_
#define SRC_WIFI_MODEL_DROP_REASON_H_

namespace ns3 {

	enum DropReason {
		Unknown,
		PhyInSleepMode,
		PhyNotEnoughSignalPower,
		PhyUnsupportedMode,
		PhyPreampleHeaderReceptionFailed,
		PhyRxDuringChannelSwitching,
		PhyAlreadyReceiving,
		PhyAlreadyTransmitting,
		PhyPlcpReceptionFailed,
		MacNotForAP,
		MacAPToAPFrame,
		MacQueueDelayExceeded,
		MacQueueSizeExceeded,
		TCPTxBufferExceeded
	};

}
#endif /* SRC_WIFI_MODEL_DROP_REASON_H_ */
