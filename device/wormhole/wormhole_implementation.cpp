// SPDX-FileCopyrightText: (c) 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "wormhole_implementation.h"

#include "src/firmware/riscv/wormhole/host_mem_address_map.h"
#include "src/firmware/riscv/wormhole/eth_interface.h"

#include "device/tt_device.h"

constexpr std::uint32_t NOC_ADDR_LOCAL_BITS = 36; // source: noc_parameters.h, common for WH && BH
constexpr std::uint32_t NOC_ADDR_NODE_ID_BITS = 6; // source: noc_parameters.h, common for WH && BH

namespace tt::umd {

std::tuple<xy_pair, xy_pair> wormhole_implementation::multicast_workaround(xy_pair start, xy_pair end) const {
    // When multicasting there is a rare case where including the multicasting node in the box can result in a backup
    // and the multicasted data not reaching all endpoints specified. As a workaround we exclude the pci endpoint from
    // the multicast. This doesn't cause any problems with making some tensix cores inaccessible because column 0 (which
    // we are excluding) doesn't have tensix.
    start.x = start.x == 0 ? 1 : start.x;
    return std::make_tuple(start, end);
}

tlb_configuration wormhole_implementation::get_tlb_configuration(uint32_t tlb_index) const {
    if (tlb_index >= wormhole::TLB_BASE_INDEX_16M) {
        return tlb_configuration{
            .size = wormhole::DYNAMIC_TLB_16M_SIZE,
            .base = wormhole::DYNAMIC_TLB_16M_BASE,
            .cfg_addr = wormhole::DYNAMIC_TLB_16M_CFG_ADDR,
            .index_offset = tlb_index - wormhole::TLB_BASE_INDEX_16M,
            .offset = wormhole::TLB_16M_OFFSET,
        };
    } else if (tlb_index >= wormhole::TLB_BASE_INDEX_2M) {
        return tlb_configuration{
            .size = wormhole::DYNAMIC_TLB_2M_SIZE,
            .base = wormhole::DYNAMIC_TLB_2M_BASE,
            .cfg_addr = wormhole::DYNAMIC_TLB_2M_CFG_ADDR,
            .index_offset = tlb_index - wormhole::TLB_BASE_INDEX_2M,
            .offset = wormhole::TLB_2M_OFFSET,
        };
    } else {
        return tlb_configuration{
            .size = wormhole::DYNAMIC_TLB_1M_SIZE,
            .base = wormhole::DYNAMIC_TLB_1M_BASE,
            .cfg_addr = wormhole::DYNAMIC_TLB_1M_CFG_ADDR,
            .index_offset = tlb_index - wormhole::TLB_BASE_INDEX_1M,
            .offset = wormhole::TLB_1M_OFFSET,
        };
    }
}

std::optional<std::tuple<std::uint64_t, std::uint64_t>> wormhole_implementation::describe_tlb(
    std::int32_t tlb_index) const {
    std::uint32_t TLB_COUNT_1M = 156;
    std::uint32_t TLB_COUNT_2M = 10;
    std::uint32_t TLB_COUNT_16M = 20;

    std::uint32_t TLB_BASE_1M = 0;
    std::uint32_t TLB_BASE_2M = TLB_COUNT_1M * (1 << 20);
    std::uint32_t TLB_BASE_16M = TLB_BASE_2M + TLB_COUNT_2M * (1 << 21);
    if (tlb_index < 0) {
        return std::nullopt;
    }

    if (tlb_index >= 0 && tlb_index < TLB_COUNT_1M) {
        std::uint32_t size = 1 << 20;
        return std::tuple(TLB_BASE_1M + size * tlb_index, size);
    } else if (tlb_index >= 0 && tlb_index < TLB_COUNT_1M + TLB_COUNT_2M) {
        auto tlb_offset = tlb_index - TLB_COUNT_1M;
        auto size = 1 << 21;
        return std::tuple(TLB_BASE_2M + tlb_offset * size, size);
    } else if (tlb_index >= 0 and tlb_index < TLB_COUNT_1M + TLB_COUNT_2M + TLB_COUNT_16M) {
        auto tlb_offset = tlb_index - (TLB_COUNT_1M + TLB_COUNT_2M);
        auto size = 1 << 24;
        return std::tuple(TLB_BASE_16M + tlb_offset * size, size);
    }

    return std::nullopt;
}

std::pair<std::uint64_t, std::uint64_t> wormhole_implementation::get_tlb_data(
    std::uint32_t tlb_index, const tlb_data &data) const {
    std::uint32_t TLB_COUNT_1M = 156;
    std::uint32_t TLB_COUNT_2M = 10;
    std::uint32_t TLB_COUNT_16M = 20;

    if (tlb_index < TLB_COUNT_1M) {
        return data.apply_offset(wormhole::TLB_1M_OFFSET);
    } else if (tlb_index < TLB_COUNT_1M + TLB_COUNT_2M) {
        return data.apply_offset(wormhole::TLB_2M_OFFSET);
    } else if (tlb_index < TLB_COUNT_1M + TLB_COUNT_2M + TLB_COUNT_16M) {
        return data.apply_offset(wormhole::TLB_16M_OFFSET);
    } else {
        throw std::runtime_error("Invalid TLB index for Wormhole arch");
    }
}

tt_driver_host_address_params wormhole_implementation::get_host_address_params() const {
    return {::wormhole::host_mem::address_map::ETH_ROUTING_BLOCK_SIZE, ::wormhole::host_mem::address_map::ETH_ROUTING_BUFFERS_START};
}

tt_driver_eth_interface_params wormhole_implementation::get_eth_interface_params() const {
    return {
        ETH_RACK_COORD_WIDTH,
        CMD_BUF_SIZE_MASK,
        MAX_BLOCK_SIZE,
        REQUEST_CMD_QUEUE_BASE,
        RESPONSE_CMD_QUEUE_BASE,
        CMD_COUNTERS_SIZE_BYTES,
        REMOTE_UPDATE_PTR_SIZE_BYTES,
        CMD_DATA_BLOCK,
        CMD_WR_REQ,
        CMD_WR_ACK,
        CMD_RD_REQ,
        CMD_RD_DATA,
        CMD_BUF_SIZE,
        CMD_DATA_BLOCK_DRAM,
        ETH_ROUTING_DATA_BUFFER_ADDR,
        REQUEST_ROUTING_CMD_QUEUE_BASE,
        RESPONSE_ROUTING_CMD_QUEUE_BASE,
        CMD_BUF_PTR_MASK,
        CMD_ORDERED,
        CMD_BROADCAST,
    };
}

tt_driver_noc_params wormhole_implementation::get_noc_params() const {
    return {NOC_ADDR_LOCAL_BITS, NOC_ADDR_NODE_ID_BITS};
}

}  // namespace tt::umd
