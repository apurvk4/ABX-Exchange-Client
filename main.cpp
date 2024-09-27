#include "ABXClient.h"
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>

void resendMissingSequences(ABXClient &client,
                                      std::vector<Response> &responses) {
  std::set<int32_t> receivedSequences;

  for (const auto &response : responses) {
    receivedSequences.insert(response.sequenceNumber);
  }

  int32_t minSeq = responses.empty() ? 0 : responses.front().sequenceNumber;
  int32_t maxSeq = responses.empty() ? 0 : responses.back().sequenceNumber;

  for (int32_t seq = minSeq; seq <= maxSeq; ++seq) {
    if (receivedSequences.find(seq) == receivedSequences.end()) {
        std::cout << "requesting resend for sequence number : " << seq << "\n";
        Response resendResponse = client.resend(seq);
        auto it =
            std::lower_bound(responses.begin(), responses.end(), resendResponse,
                             [](const Response &a, const Response &b) {
                               return a.sequenceNumber < b.sequenceNumber;
                             });
        responses.insert(it, resendResponse);
    }
  }
}



void writeResponsesToJson(const std::vector<Response> &responses,
                          const std::string &filename) {
  std::ofstream file(filename);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open the file for writing.");
  }

  file << "[\n";

  for (size_t i = 0; i < responses.size(); ++i) {
    const Response &response = responses[i];


    file << "  {\n";
    file << "    \"symbol\": \""
         << std::string(reinterpret_cast<const char *>(response.symbol), 4)
         << "\",\n";
    file << "    \"indicator\": "
         << "\"" << static_cast<char>(response.indicator) << "\",\n";
    file << "    \"quantity\": " << response.Quantity << ",\n";
    file << "    \"price\": " << response.price << ",\n";
    file << "    \"sequenceNumber\": " << response.sequenceNumber << "\n";
    file << "  }";
    if (i != responses.size() - 1) {
      file << ",";
    }
    file << "\n";
  }
  file << "]\n";

  file.close();
}

int main() {
  try {

    ABXClient client{"127.0.0.1", 3000};

    std::vector<Response> responses = client.streamAllPackets();
    std::cout << "received initial response with " << responses.size()
              << " packets\n";

    resendMissingSequences(client,responses);

    writeResponsesToJson(responses, "response.json");
    std::cout << "response written to file\n";
  } catch (const std::exception &ex) {
    std::cerr << "Failed to write Response JSON file, with Error: " << ex.what() << '\n';
    return -1;
  }

  return 0;
}
