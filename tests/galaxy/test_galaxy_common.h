/*
 * SPDX-FileCopyrightText: (c) 2023 Tenstorrent Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#pragma once

#include <set>
#include <string>
#include <tuple>
#include <vector>
#include <sstream>

#include "device/tt_device.h"
#include "device/tt_xy_pair.h"

#include "fmt/core.h"

// static const std::string SOC_DESC_PATH = "./tests/soc_descs/wormhole_b0_8x10.yaml";

using chip_id_t = int;
using ethernet_channel_t = int;
using eth_coord_t = std::tuple<int, int, int, int>;  // x, y, rack, shelf
struct tt_multichip_core_addr {
    tt_multichip_core_addr() : core{}, chip{}, addr{} {}
    tt_multichip_core_addr(chip_id_t chip, tt_xy_pair core, std::uint64_t addr) : core(core), chip(chip), addr(addr) {}

    tt_xy_pair core;
    chip_id_t chip;
    std::uint64_t addr;
    std::string str() const {
        return fmt::format("(chip={},x={},y={},addr=0x{:x})", chip, core.x, core.y, addr);
    }
};

// SIMPLE DATAMOVEMENT API BASED ON UMD
// send one contiguous chunk of data from one sender core to a receiver core
void move_data(
    tt_SiliconDevice& device, tt_multichip_core_addr sender_core, tt_multichip_core_addr receiver_core, uint32_t size);

// send one contiguous chunk of data to a vector of receiver cores
void broadcast_data(
    tt_SiliconDevice& device,
    tt_multichip_core_addr sender_core,
    std::vector<tt_multichip_core_addr> receiver_cores,
    uint32_t size);
