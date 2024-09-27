#pragma once

#include "TcpClient.h"
#include <vector>
#include "ProtocolDefinitions.h"

class ABXClient {
  TcpClient _client;
  bool _serverConnected;
  bool isValidResponse(const Response &response);
  void convertToHostByteOrder(Response &response);

public:
  ABXClient(std::string hostname,uint16_t serverPort);
  ABXClient(sockaddr_in &ipv4Address, uint16_t serverPort);
  ABXClient(sockaddr_in6 &ipV6Address, uint16_t serverPort);
  std::vector<Response> streamAllPackets();
  Response resend(int32_t sequenceNumber);
};