#pragma once
#include <cstdint>

#pragma pack(push, 1)

enum class  CallType : uint8_t {
	StreamAll = 1,
	Resend = 2
};

struct Request {
  CallType callType;
  uint8_t resendSeq;
};

enum class Indicator : uint8_t {
	B = 66,
	S = 83
};

struct Response {
  uint8_t symbol[4];
  Indicator indicator;
  uint32_t Quantity;
  uint32_t price;
  int32_t sequenceNumber;
};

#pragma pack(pop)