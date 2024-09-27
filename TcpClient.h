#pragma once

#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>


class TcpClient {
private:
  SOCKET _sockId;
  sockaddr_in _addr;
  sockaddr_in6 _addr6;
  bool _isIPv6;
  uint16_t _serverPort;

  void initWinsock();

public:
  TcpClient(std::string ServerHostname, uint16_t ServerPort);


  TcpClient(sockaddr_in ServerAddr4, uint16_t ServerPort);


  TcpClient(sockaddr_in6 ServerAddr6, uint16_t ServerPort);


  TcpClient(const TcpClient &other) = delete;


  void operator=(const TcpClient &other) = delete;


  bool connect();


  int send(uint8_t *bytes, int size);


  int receive(uint8_t *bytes, int size);


  void close();


  ~TcpClient();
};
