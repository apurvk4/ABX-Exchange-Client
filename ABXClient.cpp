#include "ABXClient.h"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

ABXClient::ABXClient(std::string hostname, uint16_t serverPort)
    : _client(hostname, serverPort), _serverConnected{false} {
  if (!_client.connect()) {
    throw std::runtime_error("Failed to connect to the server.");
  }
  _serverConnected = true;
}

ABXClient::ABXClient(sockaddr_in &ipv4Address, uint16_t serverPort)
    : _client(ipv4Address, serverPort), _serverConnected{false} {
  if (!_client.connect()) {
    throw std::runtime_error("Failed to connect to the server.");
  }
  _serverConnected = true;
}

ABXClient::ABXClient(sockaddr_in6 &ipv6Address, uint16_t serverPort)
    : _client(ipv6Address, serverPort), _serverConnected{false} {
  if (!_client.connect()) {
    throw std::runtime_error("Failed to connect to the server.");
  }
  _serverConnected = true;
}

bool ABXClient::isValidResponse(const Response &response) {
  for (int i = 0; i < 4; i++) {
    if (!std::isalnum(
            response.symbol[i])) {
      return false;
    }
  }

  if (response.indicator != Indicator::B &&
      response.indicator != Indicator::S) {
    return false;
  }

  if (response.Quantity <= 0 || response.price <= 0) {
    return false;
  }

  if (response.sequenceNumber < 0) {
    return false;
  }
  return true;
}

void ABXClient::convertToHostByteOrder(Response &response) {
  response.Quantity = ntohl(response.Quantity); 
  response.price = ntohl(response.price);    
  response.sequenceNumber = ntohl(response.sequenceNumber);
}



std::vector<Response> ABXClient::streamAllPackets() {
  if (!_serverConnected) {
    if (!_client.connect()) {
      throw std::runtime_error("failed to connect to server");
    }
    _serverConnected = true;
  }
  std::vector<Response> responses;
  Request request;
  memset(&request, 0, sizeof(request));
  request.callType = CallType::StreamAll;

  uint8_t *requestPtr = reinterpret_cast<uint8_t *>(&request);
  int totalBytesSent = 0;
  int requestSize = sizeof(request);

  while (totalBytesSent < requestSize) {
    int bytesSent =
        _client.send(requestPtr + totalBytesSent, requestSize - totalBytesSent);
    if (bytesSent < 0) {
      break;
    }
    totalBytesSent += bytesSent;
  }
  if (totalBytesSent != requestSize) {
    throw std::runtime_error("Failed to send request.");
  }

  std::vector<uint8_t> dataBuffer; 
  bool receiving = true;

  uint8_t *tempBuffer = new uint8_t[sizeof(Response)]; 
  while (receiving) {
    memset(tempBuffer, 0, sizeof(Response));
    int bytesReceived = _client.receive(tempBuffer, sizeof(Response));

    if (bytesReceived > 0) {
      dataBuffer.insert(dataBuffer.end(), tempBuffer,
                        tempBuffer + bytesReceived);
    } else {
      receiving = false;
    } 
  }
  _serverConnected = false;
  _client.close();
  delete[] tempBuffer;

  size_t offset = 0;
  while (offset + sizeof(Response) <= dataBuffer.size()) {
    Response response;
    std::memset(&response, 0, sizeof(response));

    std::memcpy(&response, dataBuffer.data() + offset, sizeof(Response));
    convertToHostByteOrder(response);
    if (isValidResponse(response)) {
      responses.push_back(response);
    } else {
      std::cerr << "Invalid response packet received. Discarding." << std::endl;
    }
    offset += sizeof(Response);
  }
  return responses;
}


Response ABXClient::resend(int32_t sequenceNumber) {
  if (!_serverConnected) {
    if (!_client.connect()) {
      throw std::runtime_error("failed to connect to server for resending sequence number :" + std::to_string(sequenceNumber));
    }
    _serverConnected = true;
  }
  Request request;
  request.callType = CallType::Resend;

  request.resendSeq = static_cast<uint8_t>(sequenceNumber & 0xFF);

  uint8_t *requestPtr = reinterpret_cast<uint8_t *>(&request);
  int totalBytesSent = 0;
  int requestSize = sizeof(request);

  while (totalBytesSent < requestSize) {
    int bytesSent =
        _client.send(requestPtr + totalBytesSent, requestSize - totalBytesSent);
    if (bytesSent < 0) {
      throw std::runtime_error("Failed to send request to resend packet for sequence number : "
                               +std::to_string(sequenceNumber));
    }
    totalBytesSent += bytesSent;
  }

  uint8_t *resendPtr = reinterpret_cast<uint8_t *>(&(request.resendSeq));
  totalBytesSent = 0;
  int resendSize = sizeof(&(request.resendSeq));

  while (totalBytesSent < resendSize) {
    int bytesSent =
        _client.send(resendPtr + totalBytesSent, resendSize - totalBytesSent);
    if (bytesSent < 0) {
      throw std::runtime_error("Failed to send resend sequence number " +
                               std::to_string(sequenceNumber));
    }
    totalBytesSent += bytesSent;
  }

  std::vector<uint8_t> responseBuffer;
  bool receiving = true;

  uint8_t *tempBuffer = new uint8_t[sizeof(Response)];
  while (receiving) {
    memset(tempBuffer, 0, sizeof(Response));
    int bytesReceived = _client.receive(tempBuffer, sizeof(Response));

    if (bytesReceived > 0) {
      responseBuffer.insert(responseBuffer.end(), tempBuffer,
                            tempBuffer + bytesReceived);

      if (responseBuffer.size() >= sizeof(Response)) {
        receiving = false;

        if (responseBuffer.size() > sizeof(Response)) {
          responseBuffer.resize(sizeof(Response));
        }
      }
    } else {
      receiving = false;
    }
  }
  _client.close();
  _serverConnected = false;
  delete[] tempBuffer;

  if (responseBuffer.size() < sizeof(Response)) {
    throw std::runtime_error("Failed to receive complete response for resend sequence number : " + std::to_string(sequenceNumber));
  }
  Response response;
  std::memset(&response, 0, sizeof(response));
  std::memcpy(&response, responseBuffer.data(), sizeof(Response));
  convertToHostByteOrder(response);
  if (!isValidResponse(response)) {
    throw std::runtime_error("Received invalid response packet for resend " +
                             std::to_string(sequenceNumber));
  }
  return response;
}
