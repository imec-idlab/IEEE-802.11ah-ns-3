#ifndef TCP_CLIENT
#define TCP_CLIENT


#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "ns3/traced-value.h"
#include "ns3/random-variable-stream.h"
#include "ns3/tcp-socket.h"
#include "ns3/drop-reason.h"
#include "ns3/tcp-header.h"
#include "ns3/tcp-socket-base.h"

#include <queue>


namespace ns3 {

class Socket;
class Packet;


class TcpClient : public Application
{
public:
  static TypeId GetTypeId (void);

  typedef void (* PacketDroppedCallback)
                      (Ptr<Packet> packet, DropReason reason);

  TcpClient ();

  virtual ~TcpClient ();

  /**
   * \param ip destination ipv4 address
   * \param port destination port
   */
  void SetRemote (Ipv4Address ip, uint16_t port);

  /**
   * Set the data size of the packet (the number of bytes that are sent as data
   * to the server).  The contents of the data are set to unspecified (don't
   * care) by this call.
   *
   * \warning If you have set the fill data for the echo client using one of the
   * SetFill calls, this will undo those effects.
   *
   * \param dataSize The size of the echo data you want to sent.
   */
  void SetDataSize (uint32_t dataSize);

  /**
   * Get the number of data bytes that will be sent to the server.
   *
   * \warning The number of bytes may be modified by calling any one of the
   * SetFill methods.  If you have called SetFill, then the number of
   * data bytes will correspond to the size of an initialized data buffer.
   * If you have not called a SetFill method, the number of data bytes will
   * correspond to the number of don't care bytes that will be sent.
   *
   * \returns The number of data bytes.
   */
  uint32_t GetDataSize (void) const;

  // mainly to the automated test
  uint32_t GetPacketsSent (void) { return m_sent; };
  uint64_t GetBytesSent (void) { return m_bytesSent; };
  uint32_t GetPacketsReceived (void) { return m_recv; };
  uint64_t GetBytesReceived (void) { return m_bytesRecv; };

  typedef void (* PacketReceivedCallback)
              (Ptr<const Packet>, Address from);

  typedef void (* RetransmissionScheduledCallBack)
  		            (Address);



protected:
  virtual void DoDispose (void);

  virtual void Flush();

  virtual void Write(char* data, int size);

  virtual int Read(char* data, int size);

  virtual void StartApplication (void);
  virtual void StopApplication (void);

 virtual void  OnDataReceived();

 /**
   * Will read up to size bytes from the stream and convert it to a string
   * might be smaller if data is not yet available, won't block
   */
  std::string ReadString(int size);
  void WriteString(std::string, bool flush);

private:

  virtual void ReceivePacket (Ptr<Socket> socket);

  void Send (uint8_t* data, int size);



  void OnCongestionWindowChanged(uint32_t oldval, uint32_t newval);
  void OnRTOChanged(Time oldval, Time newval);
  void OnRTTChanged(Time oldval, Time newval);

  void OnTCPStateChanged(TcpSocket::TcpStates_t oldVal,TcpSocket::TcpStates_t newVal);

  void OnRetransmission(Address a);
  void OnTCPDataPacketSent(Ptr<const Packet> packet, const TcpHeader& header, Ptr<const TcpSocketBase> tcpSocket, bool isRetransmission);

  void OnTCPSlowStartThresholdChanged(uint32_t oldVal,uint32_t newVal);
  void OnTCPEstimatedBWChanged(double oldVal, double newVal);

  uint32_t m_size;

  uint32_t m_sent;
  uint32_t m_recv;
  uint64_t m_bytesSent;
  uint64_t m_bytesRecv;

  Ptr<Socket> m_socket;
  Ipv4Address m_peerAddress;
  uint16_t m_peerPort;

  TracedCallback<uint32_t,uint32_t>          m_cWnd;

  Ptr<UniformRandomVariable> m_rv;

  /// Callbacks for tracing the packet Tx events
  TracedCallback<Ptr<const Packet> > m_txTrace;

  TracedCallback<Ptr<const Packet>, Address> m_packetReceived;
  TracedCallback<Address> m_retransmission;

  TracedCallback<Time,Time>  m_rtoChanged;
  TracedCallback<Time,Time>  m_rttChanged;

  TracedCallback<TcpSocket::TcpStates_t,TcpSocket::TcpStates_t> m_tcpStateChanged;


  TracedCallback<uint32_t,uint32_t> m_slowStartThresholdChanged;

  TracedCallback<double,double>  m_currentEstimatedBandwidthChanged;

  std::queue<char> rxBuffer;
  std::queue<char> txBuffer;

  TracedCallback<Ptr<Packet>, DropReason> m_packetdropped;


};

} // namespace ns3


#endif /* TCP_CLIENT */
