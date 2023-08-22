
#ifdef DEBUG
#define DEBUG_LOG(str) do { std::cout << str << std::endl; } while( false )
#else
#define DEBUG_LOG(str) do { } while ( false )
#endif

#include "tt_device.h"
#include "device/tt_cluster_descriptor_types.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include "yaml-cpp/yaml.h"

////////
// Device base
////////
tt_device::tt_device(const std::string& sdesc_path) : soc_descriptor_per_chip({}) {
}

tt_device::~tt_device() {
}

const tt_SocDescriptor *tt_device::get_soc_descriptor(chip_id_t chip) const { return &soc_descriptor_per_chip.at(chip); }
