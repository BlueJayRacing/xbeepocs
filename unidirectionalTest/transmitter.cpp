#include <iostream>
#include <fstream>
#include <cstring>
#include <string>

extern "C" {
  #include "../libraries/xbee_ansic_library/include/xbee/wpan.h"
}


std::string const SOURCE_DATA_FNAME = "source_data.txt";
uint64_t const RECEIVER_IPV4 = 0x0;

int main(void) {
  std::cout << "Starting the transmitter" << std::endl;
  
  /*
  // Open the file
  std::ifstream source_data_file(SOURCE_DATA_FNAME);
  if (!source_data_file.is_open()) {
    throw std::runtime_error("Could not open the source data file");
  }
  */
  
  // We're going to first test by packing a single message into a 0x10.
  // We'll make it a broadcast since we don't know what the destination
  // address is.
  
  
  // Each 0x10 has configs and then payload
  uint8_t FRAME_TYPE = 0;
  uint64_t BROADCAST_ADDRESS = 0x000000000000FFFF;
  uint8_t BROADCAST_RADIUS = 0x0;
  uint8_t TRANSMIT_OPTIONS = 0x0;
  
  uint8_t frameID = 0;
  std::string first_paylod = "this_is_a_26_byte_message";

  
  // Send first_message as a UART signal along fd = 3
  uint64_t GPIO_FD = 5;
  uint32_t BAUD_RATE = 230400;
	uint32_t NUM_PARITY_BITS = 0;
	uint32_t NUM_STOP_BITS = 2;
	uint32_t NUM_DATA_BITS = 8;

  
  
  
}