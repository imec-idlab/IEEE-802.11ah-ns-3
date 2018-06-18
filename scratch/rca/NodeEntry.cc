#include "NodeEntry.h"
#include "src/wifi/model/extension-headers.h"
#include "src/wifi/model/sta-wifi-mac.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("NodeEntry");
bool showLog = false;
Time NodeEntry::maxLatency = Time::Min();
Time NodeEntry::minLatency = Time::Max();
Time NodeEntry::minJitter = Time::Max();
Time NodeEntry::maxJitter = Time::Min();

NodeEntry::NodeEntry(int id, Statistics* stats, Ptr<Node> node, Ptr<NetDevice> device) :
				id(id), stats(stats), node(node), device(device) {
}

void NodeEntry::SetAssociation(std::string context, Mac48Address address) {
	this->isAssociated = true;

	// determine AID
	auto matches = Config::LookupMatches(
			"/NodeList/" + std::to_string(this->id)
	+ "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/");
	auto obj = matches.Get(0)->GetObject<StaWifiMac>();
	this->aId = obj->GetAID();

	//cout << "Associated with aId " << this->aId;

	this->associatedCallback();
}

void NodeEntry::UnsetAssociation(std::string context, Mac48Address address) {
	this->isAssociated = false;

	cout << "[" << this->id << "] " << Simulator::Now().GetMicroSeconds() << " "
			<< "Node is deassociated" << endl;

	this->deAssociatedCallback();
}


void NodeEntry::OnNrOfTransmissionsDuringRAWSlotChanged(std::string context, uint16_t oldValue, uint16_t newValue) {
	stats->get(this->id).NumberOfTransmissionsDuringRAWSlot = newValue;
}

void NodeEntry::OnS1gBeaconMissed(std::string context, bool nextBeaconIsDTIM) {
	stats->get(this->id).NumberOfBeaconsMissed++;
}

void NodeEntry::OnPhyTxBegin(std::string context, Ptr<const Packet> packet) {
	if(showLog) cout << Simulator::Now().GetMicroSeconds() << " [" << this->aId << "] "
			<< "Begin Tx " << packet->GetUid() << endl;
	txMap.emplace(packet->GetUid(), Simulator::Now());

	if (txMap.size() > 1)
		cout << "warning: more than 1 transmission active: " << txMap.size()
		<< " transmissions" << endl;

	/*if (aId >= lastBeaconAIDStart && aId <= lastBeaconAIDEnd) {
	 Time timeDiff = (Simulator::Now() - this->lastBeaconReceivedOn);
	 cout << "[" << this->id << "] " << Simulator::Now().GetMicroSeconds() << "µs" << " Tx started after " << timeDiff.GetMicroSeconds() << "µs since last s1g beacon" << endl;
	 }*/

	stats->get(this->id).NumberOfTransmissions++;
}

void NodeEntry::OnPhyTxEnd(std::string context, Ptr<const Packet> packet) {
	if(showLog) cout << Simulator::Now().GetMicroSeconds() << " [" << this->aId << "] "
			<< "End Tx " << packet->GetUid() << endl;

	if (txMap.find(packet->GetUid()) != txMap.end()) {
		Time oldTime = txMap[packet->GetUid()];
		txMap.erase(packet->GetUid());
		stats->get(this->id).TotalTransmitTime += (Simulator::Now() - oldTime);
	} else
		if(showLog) cout << "[" << this->id << "] " << Simulator::Now().GetMicroSeconds()
		<< " End tx for packet " << packet->GetUid()
		<< " without a begin tx" << endl;
}

void NodeEntry::OnPhyTxDrop(std::string context, Ptr<const Packet> packet, DropReason reason) {
	if(showLog) cout << "[" << this->aId << "] " << "Tx Dropped " << packet->GetUid()
					<< endl;

	if (txMap.find(packet->GetUid()) != txMap.end()) {
		Time oldTime = txMap[packet->GetUid()];
		txMap.erase(packet->GetUid());
		stats->get(this->id).TotalTransmitTime += (Simulator::Now() - oldTime);
	} else
		if(showLog) cout << "[" << this->id << "] " << Simulator::Now().GetMicroSeconds()
		<< " End tx for packet " << packet->GetUid()
		<< " without a begin tx" << endl;
	stats->get(this->id).NumberOfTransmissionsDropped++;

	stats->get(this->id).NumberOfDropsByReason[reason]++;


}

void NodeEntry::OnPhyRxBegin(std::string context, Ptr<const Packet> packet) {
	//cout << "[" << this->aId << "] " << Simulator::Now().GetMicroSeconds()
	//<< " Begin Rx " << packet->GetUid() << endl;
	rxMap.emplace(packet->GetUid(), Simulator::Now());

	if (rxMap.size() > 1)
		if(showLog) cout << "warning: more than 1 receive active: " << rxMap.size()
		<< " receives" << endl;
}

void NodeEntry::OnPhyRxEnd(std::string context, Ptr<const Packet> packet) {
	//cout  << Simulator::Now().GetMicroSeconds() << "[" << this->aId << "] "
	//<< " End Rx " << packet->GetUid() << endl;
	this->OnEndOfReceive(packet);
}

void NodeEntry::OnEndOfReceive(Ptr<const Packet> packet) {
	WifiMacHeader hdr;
	packet->PeekHeader(hdr);

	stats->get(this->id).NumberOfReceives++;
	if (rxMap.find(packet->GetUid()) != rxMap.end()) {
		Time oldTime = rxMap[packet->GetUid()];
		rxMap.erase(packet->GetUid());

		stats->get(this->id).TotalReceiveTime += (Simulator::Now() - oldTime);

		if (hdr.IsS1gBeacon()) {
			lastBeaconReceivedOn = Simulator::Now();

			auto pCopy = packet->Copy();
			S1gBeaconHeader s1gBeaconHeader;
			pCopy->RemoveHeader(hdr);
			pCopy->RemoveHeader(s1gBeaconHeader);

			//       cout << "[" << this->id << "] " <<  "Received S1g beacon " << endl;
			auto raw = s1gBeaconHeader.GetRPS().GetRawAssigmentObj();

			this->lastBeaconAIDStart = raw.GetRawGroupAIDStart();
			this->lastBeaconAIDEnd = raw.GetRawGroupAIDEnd();

			if (s1gBeaconHeader.GetTIM().GetDTIMCount() == 0) {
				// DTIM data
				auto pageBitmapLen = s1gBeaconHeader.GetpageSlice().GetPageBitmapLength();

				/*if(vmap != 0x0) {
				 std::cout << Simulator::Now().GetMicroSeconds() << "[" << this->id << "]" << " DTIM beacon received, VMAP: ";
				 for(int i = 31; i >= 0; i--)
				 cout << ((vmap >> i) & 0x01);
				 cout << endl;
				 }*/

				/*	std::cout << Simulator::Now().GetMicroSeconds() << "[" << this->id << "]" << " DTIM beacon received, VMAP: ";
				 for(int i = 31; i >= 0; i--)
				 cout << ((vmap >> i) & 0x01);
				 cout << endl;
				 */
				bool inPage = false;
				if (pageBitmapLen) {
					uint8_t myBlock = (this->aId >> 6) & 0x1f;
					uint8_t Ind, offset;
					Ind = myBlock / 8;
					offset = myBlock % 8;
					inPage = s1gBeaconHeader.GetpageSlice().GetPageBitmap()[Ind] & (1 << offset);
					// there is pending data at the AP
					if (inPage)
						rawTIMGroupFlaggedAsDataAvailableInDTIM = true;
				}
				else if (!inPage)
				{
					// no pending data at the AP
					rawTIMGroupFlaggedAsDataAvailableInDTIM = false;
				}
			}

			//cout << "S1g beacon received" << endl;
			//cout << "RAW Group: page=" << std::to_string(raw.GetRawGroupPage()) << ", aId start=" << std::to_string(lastBeaconAIDStart) << ", aId end=" << std::to_string(lastBeaconAIDEnd) << endl;
			//cout << "RAW Group uint32: " << std::to_string(raw.GetRawGroup()) << endl;
		}

		//s1gBeaconHeader.Print(cout);
	} else {
		// this can happen when the state is SWITCHING, TX, RX when receiving the message preamble.
		// The begin RxBegin will not be triggered, but an RxDrop will be generated

		// cout << "[" << this->id << "] " << Simulator::Now().GetMicroSeconds()
		//  	  <<" End rx for packet " << packet->GetUid() << " without a begin rx" << endl;
	}
}

void NodeEntry::OnPhyRxDrop(std::string context, Ptr<const Packet> packet,
		DropReason reason) {
	this->OnEndOfReceive(packet);

	// THIS REQUIRES PACKET METADATA ENABLE!
	auto pCopy = packet->Copy();
	auto it = pCopy->BeginItem();
	while (it.HasNext()) {

		auto item = it.Next();
		Callback<ObjectBase *> constructor = item.tid.GetConstructor();

		ObjectBase *instance = constructor();
		Chunk *chunk = dynamic_cast<Chunk *>(instance);
		chunk->Deserialize(item.current);

		if (dynamic_cast<WifiMacHeader*>(chunk)) {
			WifiMacHeader* hdr = (WifiMacHeader*) chunk;

			if (hdr->GetAddr1() == node->GetDevice(0)->GetAddress()) {

				stats->get(this->id).NumberOfReceiveDroppedByDestination++;
				stats->get(this->id).NumberOfDropsByReason[reason]++;

				//packet->Copy()->Print(cout);
				//	cout  << Simulator::Now().GetMicroSeconds() << "[" << this->aId << "] "
				//		<< " Drop Rx for STA " << packet->GetUid() << endl;
			}
			//hdr->Print(cout);
			delete chunk;
			break;
		} else
			delete chunk;
	}

	stats->get(this->id).NumberOfReceivesDropped++;
}

void NodeEntry::OnPhyStateChange(std::string context, const Time start,	const Time duration, const WifiPhy::State state) {
	/*switch (state) {

	case WifiPhy::State::SLEEP:
		stats->get(this->id).TotalDozeTime += duration;
		break;

	case WifiPhy::State::IDLE:
	case WifiPhy::State::TX:
	case WifiPhy::State::RX:
	case WifiPhy::State::SWITCHING:
		stats->get(this->id).TotalActiveTime += duration;
		break;

	case WifiPhy::State::CCA_BUSY:
		// not sure why this is counted as the same as sleep
		// so state change is fired with the same duration for both SLEEP and CCA_BUSY
		break;
	}*/
	if  (stats->TimeWhenEverySTAIsAssociated > 0)
	{
		switch (state)
		{
		case WifiPhy::State::IDLE: //Idle
		stats->get(this->id).TotalIdleTime += duration;
		break;
		case WifiPhy::State::RX: //Rx
			stats->get(this->id).TotalRxTime += duration;
			break;
		case WifiPhy::State::TX: //Tx
			stats->get(this->id).TotalTxTime += duration;
			break;
		case WifiPhy::State::SLEEP: //Sleep
			stats->get(this->id).TotalSleepTime += duration;
			break;
		}
	}
	stats->get(this->id).EnergyRxIdle = (stats->get(this->id).TotalRxTime.GetSeconds() + stats->get(this->id).TotalIdleTime.GetSeconds()) * 4.4;
	stats->get(this->id).EnergyTx = stats->get(this->id).TotalTxTime.GetSeconds() * 7.2;

	/*cout << "[" << this->id << "] " << Simulator::Now().GetMicroSeconds() << "State change, new state is ";

	 switch (state) {
	 case WifiPhy::State::IDLE:
	 cout << "IDLE";
	 break;
	 case WifiPhy::State::TX:
	 cout << "TX";
	 break;
	 case WifiPhy::State::RX:
	 cout << "RX";
	 break;
	 case WifiPhy::State::SLEEP:
	 cout << "SLEEP";
	 break;
	 case WifiPhy::State::SWITCHING:
	 cout << "SWITCHING";
	 break;
	 case WifiPhy::State::CCA_BUSY:
	 cout << "CCA_BUSY";
	 break;

	 }

	 cout << ", duration is " << duration.GetMicroSeconds() << "µs" << ", start is " << start.GetMicroSeconds() << "µs";
	 cout << endl;
	 */

}

SeqTsHeader GetSeqTSFromPacket(Ptr<const Packet> packet) {
	/*auto pCopy = packet->Copy();
	SeqTsHeader seqTs;
	pCopy->RemoveHeader(seqTs);

	if (seqTs.GetTs() > Time(0)) {
		// it was deserialized OK
		return seqTs;
	} else {*/
	SeqTsHeader sts;

	// new copy
	auto pCopy = packet->Copy();
	auto it = pCopy->BeginItem();
	while (it.HasNext()) {

		auto item = it.Next();
		if (item.tid.GetUid() != 0) {
			Callback<ObjectBase *> constructor = item.tid.GetConstructor();

			ObjectBase *instance = constructor();
			Chunk *chunk = dynamic_cast<Chunk *>(instance);
			chunk->Deserialize(item.current);

			if (dynamic_cast<SeqTsHeader*>(chunk)) {
				SeqTsHeader* hdr = (SeqTsHeader*) chunk;

				sts.SetSeq(hdr->GetSeq());
				sts.SetTs(hdr->GetTs());

				delete chunk;
				break;
			} else
				delete chunk;
		}
	}

	return sts;
	//}
}

void NodeEntry::OnTcpPacketSent(Ptr<const Packet> packet) {

	//SeqTsHeader seqTs = GetSeqTSFromPacket(packet);
	if(showLog) cout << Simulator::Now().GetMicroSeconds() << " [" << this->aId << "] " << "TCP packet sent " << endl;//with seq nr " << seqTs.GetSeq() << endl;

	/*
	// the packet is just sent, so track if it's received by a list of booleans
	// with the sequence number as index
	if (seqTs.GetSeq() >= seqNrReceived.size()) {
		for (int i = seqNrReceived.size(); i <= seqTs.GetSeq(); i++) {
			seqNrReceived.push_back(false);
		}
	}
	if (seqTs.GetSeq() >= seqNrReceivedAtAP.size()) {
		for (int i = seqNrReceivedAtAP.size(); i <= seqTs.GetSeq(); i++) {
			seqNrReceivedAtAP.push_back(false);
		}
	}
	 */
	stats->get(this->id).NumberOfSentPackets++;


}

void NodeEntry::OnTcpEchoPacketReceived(Ptr<const Packet> packet, Address from) {
	SeqTsHeader seqTs = GetSeqTSFromPacket(packet);
	if(seqTs.GetSeq() > 0) {
		auto timeDiff = (Simulator::Now() - seqTs.GetTs());
		stats->get(this->id).TotalPacketRoundtripTime += timeDiff;
		stats->get(this->id).NumberOfSuccessfulRoundtripPacketsWithSeqHeader++;
	}

	stats->get(this->id).NumberOfSuccessfulRoundtripPackets++;
	this->UpdateJitter(Simulator::Now() - this->timeSent);

	/*
	try {
		SeqTsHeader seqTs = GetSeqTSFromPacket(packet);

		// check if the packet wasn't already received to prevent counting them double
		if (seqTs.GetSeq() < 0 || seqTs.GetSeq() >= seqNrReceived.size()
				|| seqNrReceived[seqTs.GetSeq()]) {
			// but the packet was already received ?
			// probably a fragment?
			//cout << "Packet with seq nr " << seqTs.GetSeq()
			//		<< " falls outside expected array or is already received"
			//		<< endl;
			return;
		} else
			seqNrReceived[seqTs.GetSeq()] = true;

		auto timeDiff = (Simulator::Now() - seqTs.GetTs());

		cout << Simulator::Now().GetMicroSeconds() << " [" << this->id << "] "
		 << " Echo packet received back from AP ("
		 << InetSocketAddress::ConvertFrom(from).GetIpv4() << ") after "
		 << std::to_string(timeDiff.GetMicroSeconds()) << "µs" << endl;

		stats->get(this->id).NumberOfSuccessfulRoundtripPackets++;
		stats->get(this->id).TotalPacketRoundtripTime += timeDiff;

	} catch (std::runtime_error e) {

		// this occurs when packet is fragmented
		cerr << "Error: " << string(e.what()) << endl;
		cerr
				<< "ERROR: unable to get the packet header to determine the travel time"
				<< endl;
		//packet->Print(cout);
		//exit(1);
	}
	 */
}

void NodeEntry::OnTcpPacketReceivedAtAP(Ptr<const Packet> packet) {

	// due to TCP fragmentation not all sent/received packets will contain seq ts headers!
	SeqTsHeader seqTs = GetSeqTSFromPacket(packet);
	if(seqTs.GetSeq() > 0) {
		auto timeDiff = (Simulator::Now() - seqTs.GetTs());
		stats->get(this->id).TotalPacketSentReceiveTime += timeDiff;
		stats->get(this->id).NumberOfSuccessfulPacketsWithSeqHeader++;
		stats->get(this->id).TotalPacketPayloadSize += packet->GetSize();
	}

	stats->get(this->id).NumberOfSuccessfulPackets++;
}

void NodeEntry::OnTcpCongestionWindowChanged(uint32_t oldval, uint32_t newval) {
	stats->get(this->id).TCPCongestionWindow = newval;
}

void NodeEntry::OnTcpRTOChanged(Time oldval, Time newval) {
	stats->get(this->id).TCPRTOValue = newval;
}

void NodeEntry::OnTcpRTTChanged(Time oldval, Time newval) {
	stats->get(this->id).TCPRTTValue = newval;
}

void NodeEntry::OnTcpStateChanged(TcpSocket::TcpStates_t oldval, TcpSocket::TcpStates_t newval) {
	tcpConnectedAtSTA = (newval == TcpSocket::TcpStates_t::ESTABLISHED);
	stats->get(this->id).TCPConnected = tcpConnectedAtSTA && tcpConnectedAtAP;
}

void NodeEntry::OnTcpStateChangedAtAP(TcpSocket::TcpStates_t oldval, TcpSocket::TcpStates_t newval) {
	tcpConnectedAtAP = (newval	== TcpSocket::TcpStates_t::ESTABLISHED);
	if(showLog) cout << "TCP connected at ap " << tcpConnectedAtAP;

	stats->get(this->id).TCPConnected = tcpConnectedAtSTA && tcpConnectedAtAP;
}

void NodeEntry::OnTcpRetransmission(Address to) {
	stats->get(this->id).NumberOfTCPRetransmissions++;
}

void NodeEntry::OnTcpRetransmissionAtAP() {
	//cout << "[" << this->id << "] " << Simulator::Now().GetMicroSeconds() << " RETRANSMISSION SCHEDULED FROM AP " << std::endl;
	stats->get(this->id).NumberOfTCPRetransmissionsFromAP++;
}

void NodeEntry::OnTcpSlowStartThresholdChanged(uint32_t oldVal, uint32_t newVal) {
	stats->get(this->id).TCPSlowStartThreshold = newVal;
}

void NodeEntry::OnTcpEstimatedBWChanged(double oldVal, double newVal) {
	stats->get(this->id).TCPEstimatedBandwidth = newVal;
}

void NodeEntry::OnUdpPacketSent(Ptr<const Packet> packet) { //works
	//cout << "[" << this->id << "] " << "UDP packet sent " << endl;
	auto pCopy = packet->Copy();

	SeqTsHeader seqTs;
	pCopy->RemoveHeader(seqTs);
	auto timeDiff = (Simulator::Now() - seqTs.GetTs());
	if (this->id == 90)
		cout << "++++++++++++++++++++++++++++++++++++++++++++++ TX at " << Simulator::Now() << "seq " << seqTs.GetSeq() << endl;
	stats->get(this->id).NumberOfSentPackets++;

	/*		cout << "[" << this->id << "] "  << Simulator::Now().GetMicroSeconds() <<
	 " Packet sent" << endl;
	 */

	/*	if (aId >= lastBeaconAIDStart && aId <= lastBeaconAIDEnd) {
	 cout << "[" << this->id << "] "  << Simulator::Now().GetMicroSeconds() <<
	 " Packet is enqueued during the RAW slot period of the node" << endl;
	 }
	 */
}

void NodeEntry::OnUdpEchoPacketReceived(Ptr<const Packet> packet, Address from) {
	//cout << "Echo packet received back from AP ("
	//	<< InetSocketAddress::ConvertFrom(from).GetIpv4() << ")" << endl;
	auto pCopy = packet->Copy();
	try {
		SeqTsHeader seqTs;
		pCopy->RemoveHeader(seqTs);
		auto timeDiff = (Simulator::Now() - seqTs.GetTs());
		if (seqTs.GetSeq() > 0)
			stats->get(this->id).NumberOfSuccessfulRoundtripPacketsWithSeqHeader++;
		stats->get(this->id).NumberOfSuccessfulRoundtripPackets++;
		stats->get(this->id).TotalPacketRoundtripTime += timeDiff;
		// RT jitter calculation [server to client direction]
		this->UpdateJitter(Simulator::Now() - this->timeSent);

	} catch (std::runtime_error e) {
		// packet fragmentation, unable to get the header from fragements
	}
}

void NodeEntry::OnCoapPacketSent(Ptr<const Packet> packet) {
	stats->get(this->id).NumberOfSentPackets++;
}
/* Jitter is calculated only for packet delivered in order
 * Dropped packets are ignored
 * */
void NodeEntry::UpdateJitter (Time timeDiff)
{
	if (stats->get(this->id).NumberOfSuccessfulRoundtripPackets == 1)
	{
		delayFirst = timeDiff;
	}
	else
	{
		delaySecond = timeDiff;
		Time var = Abs(delayFirst - delaySecond);
		stats->get (this->id).jitter = var;
		stats->get (this->id).jitterAcc += pow (var.GetMilliSeconds (), 2);
		if (NodeEntry::maxJitter < var)
			NodeEntry::maxJitter = var;
		if (NodeEntry::minJitter > var)
			NodeEntry::minJitter = var;
		delayFirst = delaySecond;

	}
}
void NodeEntry::OnCoapPacketReceived(Ptr<const Packet> packet, Address from) {
	auto pCopy = packet->Copy();
	try {
		SeqTsHeader seqTs;
		pCopy->RemoveHeader(seqTs);
		// time from the moment server sends a reply until the moment client receives it
		auto timeDiff = (Simulator::Now() - seqTs.GetTs());
		//cout << "=========================CLIENT SEQ " << seqTs.GetSeq() << endl;
		//if (seqTs.GetSeq() >= 0) //allways true
		stats->get(this->id).NumberOfSuccessfulRoundtripPacketsWithSeqHeader++;

		stats->get(this->id).NumberOfSuccessfulRoundtripPackets++;
		stats->get(this->id).TotalPacketRoundtripTime += timeDiff; //time from S to C is accumulated here and later added to the time from C to S
		this->UpdateJitter(Simulator::Now() - this->timeSent);
		uint32_t currentSequenceNumber = seqTs.GetSeq();
		//cout << "============================================================================ Client " << this->id << " received seq" << currentSequenceNumber << endl;

		Time newNow = Simulator::Now();
		if (currentSequenceNumber == stats->get(this->id).m_prevPacketSeqClient + 1)
		{
			stats->get(this->id).m_interPacketDelayClient.push_back(newNow - stats->get(this->id).m_prevPacketTimeClient);
			stats->get(this->id).interPacketDelayAtClient = newNow - stats->get(this->id).m_prevPacketTimeClient;
			//cout << "============================================================================ interPacketDelayAtClient " << this->id << " is" << newNow - stats->get(this->id).m_prevPacketTimeClient << endl;

		}
		else if (currentSequenceNumber > stats->get(this->id).m_prevPacketSeqClient + 1)
		{
			NS_LOG_INFO ("Packet(s) with seq number(s) ");
			for (uint32_t i = stats->get(this->id).m_prevPacketSeqClient + 1; i < currentSequenceNumber; i++)
				NS_LOG_INFO (std::to_string(i) << " ");
			NS_LOG_INFO ("is(are) lost in path Server -> Client");

			stats->get(this->id).m_interPacketDelayClient.push_back(newNow - stats->get(this->id).m_prevPacketTimeClient);
			stats->get(this->id).interPacketDelayAtClient = newNow - stats->get(this->id).m_prevPacketTimeClient;

		}
		stats->get(this->id).m_prevPacketSeqClient = currentSequenceNumber;
		stats->get(this->id).m_prevPacketTimeClient = newNow;

	} catch (std::runtime_error e) {
		// packet fragmentation, unable to get the header from fragements
	}

}

void NodeEntry::OnUdpPacketReceivedAtAP(Ptr<const Packet> packet) {
	auto pCopy = packet->Copy();
	try {

		SeqTsHeader seqTs;
		pCopy->RemoveHeader(seqTs);
		auto timeDiff = (Simulator::Now() - seqTs.GetTs());

		//cout << "[" << this->id << "] " << "UDP packet received at AP after "
		//	<< std::to_string(timeDiff.GetMicroSeconds()) << "µs" << endl;
		if (this->id == 90)
			cout << "++++++++++++++++++++++++++++++++++++++++++++++ RX at " << Simulator::Now() << "seq " << seqTs.GetSeq() << endl;
		stats->get(this->id).NumberOfSuccessfulPackets++;
		stats->get(this->id).TotalPacketSentReceiveTime += timeDiff;
		stats->get(this->id).latency = timeDiff;
		//cout << "id = " << this->id << " ; latency = " << timeDiff << " ; seq =  " << seqTs.GetSeq() << endl;
		stats->get(this->id).TotalPacketPayloadSize += packet->GetSize();

	} catch (std::runtime_error e) {
		// packet fragmentation, unable to get header
	}
}


void NodeEntry::OnCoapPacketReceivedAtServer(Ptr<const Packet> packet) {
	auto pCopy = packet->Copy();
	try {

		SeqTsHeader seqTs;
		pCopy->RemoveHeader(seqTs);
		// time from the moment client sends a packet to the moment server receives it - latency
		auto timeDiff = (Simulator::Now() - seqTs.GetTs());
		this->timeSent = seqTs.GetTs();
		if (NodeEntry::maxLatency < timeDiff)
			NodeEntry::maxLatency = timeDiff;
		if (NodeEntry::minLatency > timeDiff)
			NodeEntry::minLatency = timeDiff;

		//if (seqTs.GetSeq() >= 0) allways true
		stats->get(this->id).NumberOfSuccessfulPacketsWithSeqHeader++;
		stats->get(this->id).NumberOfSuccessfulPackets++;
		stats->get(this->id).TotalPacketSentReceiveTime += timeDiff;

		uint32_t currentSequenceNumber = seqTs.GetSeq();
		if (currentSequenceNumber == 0)
		{
			stats->get(this->id).m_prevPacketSeqServer = currentSequenceNumber;
			stats->get(this->id).m_prevPacketTimeServer = Simulator::Now();
		}
		else if (currentSequenceNumber == stats->get(this->id).m_prevPacketSeqServer + 1)
		{
			Time newNow = Simulator::Now();
			stats->get(this->id).m_interPacketDelayServer.push_back(newNow - stats->get(this->id).m_prevPacketTimeServer);
			stats->get(this->id).interPacketDelayAtServer = newNow - stats->get(this->id).m_prevPacketTimeServer;
			stats->get(this->id).m_time.push_back(newNow);
			stats->get(this->id).m_prevPacketSeqServer = currentSequenceNumber;
			stats->get(this->id).m_prevPacketTimeServer = newNow;
		}
		else if (currentSequenceNumber > stats->get(this->id).m_prevPacketSeqServer + 1)
		{
			NS_LOG_INFO ("Packet(s) with seq number(s) ");
			for (uint32_t i = stats->get(this->id).m_prevPacketSeqServer + 1; i < currentSequenceNumber; i++)
				NS_LOG_INFO (std::to_string(i) << " ");
			NS_LOG_INFO ("is(are) lost in path Client->Server");

			Time newNow = Simulator::Now();
			stats->get(this->id).m_interPacketDelayServer.push_back(newNow - stats->get(this->id).m_prevPacketTimeServer);
			stats->get(this->id).interPacketDelayAtServer = newNow - stats->get(this->id).m_prevPacketTimeServer;
			stats->get(this->id).m_time.push_back(newNow);
			stats->get(this->id).m_prevPacketSeqServer = currentSequenceNumber;
			stats->get(this->id).m_prevPacketTimeServer = newNow;

		}
		stats->get(this->id).TotalPacketPayloadSize += packet->GetSize() - 4 - 7; //deduct coap hdr & opts, only payload here
		//std::cout << packet->GetSize() << std::endl;
	} catch (std::runtime_error e) {
		// packet fragmentation, unable to get header
	}
}

void NodeEntry::OnMacPacketDropped(std::string context, Ptr<const Packet> packet, DropReason reason) {
	//cout << "============================Mac Packet Dropped!, reason:" << reason << endl;
	stats->get(this->id).NumberOfDropsByReason[reason]++;

}

void NodeEntry::OnTcpPacketDropped(Ptr<Packet> packet, DropReason reason) {
	//cout << "Mac Packet Dropped!, reason:" << reason << endl;
	stats->get(this->id).NumberOfDropsByReason[reason]++;
}


void NodeEntry::OnTcpFirmwareUpdated(Time totalFirmwareTransferTime) {
	stats->get(this->id).FirmwareTransferTime = totalFirmwareTransferTime;
}


void NodeEntry::OnTcpIPCameraStreamStateChanged(bool newStateIsStreaming) {
	if(newStateIsStreaming) {
		stats->get(this->id).TimeStreamStarted = Simulator::Now();
		stats->get(this->id).IPCameraTotalDataSent = 0;
		stats->get(this->id).IPCameraTotalDataReceivedAtAP = 0;
		stats->get(this->id).IPCameraTotalTimeSent = Time(0);
	}
	else {
		// stopped stream
		stats->get(this->id).IPCameraTotalTimeSent = (Simulator::Now() - stats->get(this->id).TimeStreamStarted);
	}
}

void NodeEntry::OnTcpIPCameraDataSent(uint16_t nrOfBytes) {
	stats->get(this->id).IPCameraTotalDataSent += nrOfBytes;
}

void NodeEntry::OnTcpIPCameraDataReceivedAtAP(uint16_t nrOfBytes) {
	stats->get(this->id).IPCameraTotalDataReceivedAtAP += nrOfBytes;
}

void NodeEntry::OnCollision(std::string context, uint32_t nrOfBackoffSlots) {
	if(showLog) cout << "Collision sensed" << endl;
	stats->get(this->id).NumberOfCollisions++;
	stats->get(this->id).TotalNumberOfBackedOffSlots += nrOfBackoffSlots;
}

void NodeEntry::OnTransmissionWillCrossRAWBoundary(std::string context, Time txDuration, Time remainingTimeInRawSlot) {
	cout << "Transmission cancelled, tx duration " << txDuration << ", remaining time " << remainingTimeInRawSlot << endl;

	stats->get(this->id).NumberOfTransmissionsCancelledDueToCrossingRAWBoundary++;
}

void NodeEntry::OnMacTxRtsFailed(std::string context, Mac48Address address) {
	//cout  << Simulator::Now().GetMicroSeconds() << " [" << this->aId << "] "
	//	<< " MAC Tx Rts Failed" << endl;
	stats->get(this->id).NumberOfMACTxRTSFailed++;
}

void NodeEntry::OnMacTxDataFailed(std::string context, Mac48Address address) {
	//cout  << Simulator::Now().GetMicroSeconds() << " [" << this->aId << "] "
	//		<< " MAC Tx Data Failed" << endl;
	stats->get(this->id).NumberOfMACTxMissedACK++;
}

void NodeEntry::OnMacTxFinalRtsFailed(std::string context, Mac48Address address) {
	//cout  << Simulator::Now().GetMicroSeconds() << " [" << this->aId << "] "
	stats->get(this->id).NumberOfMACTxRTSFailed++;
}

void NodeEntry::OnMacTxFinalDataFailed(std::string context, Mac48Address address) {
	//cout  << Simulator::Now().GetMicroSeconds() << " [" << this->aId << "] "
	//		<< " MAC Tx Final data Failed" << endl;
	stats->get(this->id).NumberOfMACTxMissedACKAndDroppedPacket++;
}

void NodeEntry::SetAssociatedCallback(std::function<void()> assocCallback) {
	this->associatedCallback = assocCallback;
}

void NodeEntry::SetDeassociatedCallback(std::function<void()> assocCallback) {
	this->deAssociatedCallback = assocCallback;
}

void NodeEntry::UpdateQueueLength() {
	Ptr<WifiNetDevice> wifiDe;
	wifiDe = this->device->GetObject<WifiNetDevice>();
	PointerValue ptr1;
	wifiDe->GetMac()->GetAttribute("BE_EdcaTxopN", ptr1);
	Ptr<EdcaTxopN> BE_edca = ptr1.Get<EdcaTxopN>();
	PointerValue ptr2;
	BE_edca->GetAttribute("Queue", ptr2);
	Ptr<WifiMacQueue> BE_edca_queue = ptr2.Get<WifiMacQueue>();
	this->queueLength = BE_edca_queue->GetSize();

	//cout << Simulator::Now().GetSeconds() << "s " << "Queue length of " << this->id << " is " << this->queueLength << endl;
}


NodeEntry::~NodeEntry() {
}

