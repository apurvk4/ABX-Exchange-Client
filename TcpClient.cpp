#include "TcpClient.h"
#include <cstring> 
#include <iostream>
#include <stdexcept>


TcpClient::TcpClient(std::string ServerHostname, uint16_t ServerPort)
    : _isIPv6(false), _serverPort(ServerPort) {

  initWinsock();


  memset(&_addr, 0, sizeof(_addr));
  memset(&_addr6, 0, sizeof(_addr6));

  addrinfo hints = {}, *result = nullptr;
  hints.ai_socktype = SOCK_STREAM; 
  hints.ai_family = AF_UNSPEC;    

  int res = getaddrinfo(ServerHostname.c_str(), nullptr, &hints, &result);
  if (res != 0) {
    WSACleanup();
    throw std::runtime_error("Failed to resolve hostname");
  }


  addrinfo *ptr = result;
  while (ptr != nullptr) {
    if (ptr->ai_family == AF_INET) { 
      _addr = *reinterpret_cast<sockaddr_in *>(ptr->ai_addr);
      _addr.sin_port = htons(ServerPort); 
      _isIPv6 = false;
      break;
    } else if (ptr->ai_family == AF_INET6) {
      _addr6 = *reinterpret_cast<sockaddr_in6 *>(ptr->ai_addr);
      _addr6.sin6_port = htons(ServerPort); 
      _isIPv6 = true;
      break;
    }
    ptr = ptr->ai_next;
  }


  if (ptr == nullptr) {
    freeaddrinfo(result);
    WSACleanup();
    throw std::runtime_error(
        "No valid IPv4 or IPv6 address found for hostname");
  }

  freeaddrinfo(result);
}

TcpClient::TcpClient(sockaddr_in ServerAddr4, uint16_t ServerPort)
    : _addr(ServerAddr4), _isIPv6(false), _serverPort(ServerPort) {

  initWinsock();


  memset(&_addr, 0, sizeof(_addr));

  _addr.sin_port = htons(ServerPort);
}

TcpClient::TcpClient(sockaddr_in6 ServerAddr6, uint16_t ServerPort)
    : _addr6(ServerAddr6), _isIPv6(true), _serverPort(ServerPort) {
  initWinsock();


  memset(&_addr6, 0, sizeof(_addr6));

  _addr6.sin6_port = htons(ServerPort);
}

void TcpClient::initWinsock() {
  WSADATA wsaData;
  int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (res != 0) {
    throw std::runtime_error("Failed to initialize Winsock");
  }
}

bool TcpClient::connect() {
  if (_isIPv6) {

    _sockId = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
  } else {
    _sockId = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  }

  if (_sockId == INVALID_SOCKET) {
    std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
    return false;
  }

  int result;
  if (_isIPv6) {
    result = ::connect(_sockId, reinterpret_cast<sockaddr *>(&_addr6),
                       sizeof(_addr6));
  } else {
    result =
        ::connect(_sockId, reinterpret_cast<sockaddr *>(&_addr), sizeof(_addr));
  }

  if (result == SOCKET_ERROR) {
    std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
    closesocket(_sockId);
    return false;
  }

  return true;
}

int TcpClient::send(uint8_t *bytes, int size) {
  int bytesSent = ::send(_sockId, reinterpret_cast<char *>(bytes), size, 0);
  if (bytesSent == SOCKET_ERROR) {
    std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
    return -1;
  }
  return bytesSent;
}

int TcpClient::receive(uint8_t *bytes, int size) {
  int bytesRead = ::recv(_sockId, reinterpret_cast<char *>(bytes), size, 0);
  if (bytesRead == SOCKET_ERROR) {
    std::cerr << "Receive failed: " << WSAGetLastError() << std::endl;
    return -1;
  }
  return bytesRead;
}

void TcpClient::close() {
  closesocket(_sockId);
  _sockId = INVALID_SOCKET;
}

TcpClient::~TcpClient() {
  if (_sockId != INVALID_SOCKET) {
    close();
  }
  WSACleanup();
}
