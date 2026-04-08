#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#define stringify(x) #x

// ==========================================
// TOKEN BUCKET TX MODE (must be defined early, used throughout)
// ==========================================
// 0 = Current smooth pacing mode (rate limiter based)
// 1 = Token bucket mode: 1 packet from each VL-IDX every 1ms
#ifndef TOKEN_BUCKET_TX_ENABLED
#define TOKEN_BUCKET_TX_ENABLED 0
#endif

#if TOKEN_BUCKET_TX_ENABLED
// Per-port VL range size for token bucket mode
#define TB_VL_RANGE_SIZE_DEFAULT 70
#define TB_VL_RANGE_SIZE_NO_EXT  74   // Port 1, 7 (no external TX)
#define GET_TB_VL_RANGE_SIZE(port_id) \
    (((port_id) == 1 || (port_id) == 7) ? TB_VL_RANGE_SIZE_NO_EXT : TB_VL_RANGE_SIZE_DEFAULT)

// Token bucket window (ms) - can be fractional (e.g., 1.0, 1.4, 2.5)
#define TB_WINDOW_MS 1.05
#define TB_PACKETS_PER_VL_PER_WINDOW 1
#endif

// ==========================================
// LATENCY TEST CONFIGURATION
// ==========================================
// When enabled:
// - 1 packet is sent from each VLAN (first VL-ID is used)
// - TX timestamp is written to payload
// - Latency is calculated and displayed on RX
// - 5 second timeout
// - Normal mode resumes after test
// - IMIX disabled, MAX packet size (1518) is used

#ifndef LATENCY_TEST_ENABLED
#define LATENCY_TEST_ENABLED 0
#endif

#define LATENCY_TEST_TIMEOUT_SEC 5    // Packet wait timeout (seconds)
#define LATENCY_TEST_PACKET_SIZE 1518 // Test packet size (MAX)

// ==========================================
// IMIX (Internet Mix) CONFIGURATION
// ==========================================
// Custom IMIX profile: Distribution of different packet sizes
// Total ratio: 10% + 10% + 10% + 10% + 30% + 30% = 100%
//
// In a 10 packet cycle:
//   1x 100 byte  (10%)
//   1x 200 byte  (10%)
//   1x 400 byte  (10%)
//   1x 800 byte  (10%)
//   3x 1200 byte (30%)
//   3x 1518 byte (30%)  - MTU limit
//
// Average packet size: ~964 bytes

#define IMIX_ENABLED 0

// IMIX size levels (Ethernet frame size, including VLAN)
#define IMIX_SIZE_1 100 // Smallest
#define IMIX_SIZE_2 200
#define IMIX_SIZE_3 400
#define IMIX_SIZE_4 800
#define IMIX_SIZE_5 1200
#define IMIX_SIZE_6 1518 // MTU limit (1522 with VLAN, but 1518 is safe)

// IMIX pattern size (10 packet cycle)
#define IMIX_PATTERN_SIZE 10

// IMIX average packet size (for rate limiting)
// (100 + 200 + 400 + 800 + 1200*3 + 1518*3) / 10 = 964.4
#define IMIX_AVG_PACKET_SIZE 964

// IMIX minimum and maximum sizes
#define IMIX_MIN_PACKET_SIZE IMIX_SIZE_1
#define IMIX_MAX_PACKET_SIZE IMIX_SIZE_6

// IMIX pattern array (static definition - each worker uses its own offset)
// Order: 100, 200, 400, 800, 1200, 1200, 1200, 1518, 1518, 1518
#define IMIX_PATTERN_INIT {                             \
    IMIX_SIZE_1, IMIX_SIZE_2, IMIX_SIZE_3, IMIX_SIZE_4, \
    IMIX_SIZE_5, IMIX_SIZE_5, IMIX_SIZE_5,              \
    IMIX_SIZE_6, IMIX_SIZE_6, IMIX_SIZE_6}

// ==========================================
// VLAN & VL-ID MAPPING (PORT-AWARE)
// ==========================================
//
// tx_vl_ids and rx_vl_ids can be DIFFERENT for each port!
// Ranges contain 128 VL-IDs and are defined as [start, start+128).
//
// Example (Port 0):
//   tx_vl_ids = {1027, 1155, 1283, 1411}
//   Queue 0 → VL ID [1027, 1155)  → 1027..1154 (128 entries)
//   Queue 1 → VL ID [1155, 1283)  → 1155..1282 (128 entries)
//   Queue 2 → VL ID [1283, 1411)  → 1283..1410 (128 entries)
//   Queue 3 → VL ID [1411, 1539)  → 1411..1538 (128 entries)
//
// Example (Port 2 - old default values):
//   tx_vl_ids = {3, 131, 259, 387}
//   Queue 0 → VL ID [  3, 131)  → 3..130   (128 entries)
//   Queue 1 → VL ID [131, 259)  → 131..258 (128 entries)
//   Queue 2 → VL ID [259, 387)  → 259..386 (128 entries)
//   Queue 3 → VL ID [387, 515)  → 387..514 (128 entries)
//
// Note: VLAN ID in the VLAN header (802.1Q tag) and VL-ID are different concepts.
// VL-ID is written to the last 2 bytes of the packet's DST MAC and DST IP.
// VLAN ID comes from the .tx_vlans / .rx_vlans arrays.
//
// When building packets:
//   DST MAC: 03:00:00:00:VV:VV  (VV: 16-bit of VL-ID)
//   DST IP : 224.224.VV.VV      (VV: 16-bit of VL-ID)
//
// NOTE: g_vlid_ranges is NO LONGER USED! Kept for reference only.
// Actual VL-ID ranges are read from port_vlans[].tx_vl_ids and rx_vl_ids.

typedef struct
{
  uint16_t start; // inclusive
  uint16_t end;   // exclusive
} vlid_range_t;

// DEPRECATED: These constant values are no longer used!
// tx_vl_ids/rx_vl_ids values from config are used for each port.
#define VLID_RANGE_COUNT 4
static const vlid_range_t g_vlid_ranges[VLID_RANGE_COUNT] = {
    {3, 131},   // Queue 0 (reference only)
    {131, 259}, // Queue 1 (reference only)
    {259, 387}, // Queue 2 (reference only)
    {387, 515}  // Queue 3 (reference only)
};

// DEPRECATED: These macros are no longer used!
// Use the port-aware functions in tx_rx_manager.c.
#define VL_RANGE_START(q) (g_vlid_ranges[(q)].start)
#define VL_RANGE_END(q) (g_vlid_ranges[(q)].end)
#define VL_RANGE_SIZE(q) (uint16_t)(VL_RANGE_END(q) - VL_RANGE_START(q)) // 128

// ==========================================
/* VLAN CONFIGURATION */
// ==========================================
#define MAX_TX_VLANS_PER_PORT 32
#define MAX_RX_VLANS_PER_PORT 32
#define MAX_PORTS_CONFIG 16

struct port_vlan_config
{
  uint16_t tx_vlans[MAX_TX_VLANS_PER_PORT]; // VLAN header tags
  uint16_t tx_vlan_count;
  uint16_t rx_vlans[MAX_RX_VLANS_PER_PORT]; // VLAN header tags
  uint16_t rx_vlan_count;

  // Initial VL-IDs for init (matches queue index)
  // In dynamic usage, you will iterate within these VL ranges.
  uint16_t tx_vl_ids[MAX_TX_VLANS_PER_PORT]; // {3,131,259,387}
  uint16_t rx_vl_ids[MAX_RX_VLANS_PER_PORT]; // {3,131,259,387}
};

// Per-port VLAN/VL-ID template (queue index ↔ VL range start matches)
#define PORT_VLAN_CONFIG_INIT                                                                                                                                                                               \
  {                                                                                                                                                                                                         \
    /* Port 0 */                                                                                                                                                                                            \
    {.tx_vlans = {105, 106, 107, 108}, .tx_vlan_count = 4, .rx_vlans = {253, 254, 255, 256}, .rx_vlan_count = 4, .tx_vl_ids = {1027, 1155, 1283, 1411}, .rx_vl_ids = {3, 131, 259, 387}},     /* Port 1 */  \
        {.tx_vlans = {109, 110, 111, 112}, .tx_vlan_count = 4, .rx_vlans = {249, 250, 251, 252}, .rx_vlan_count = 4, .tx_vl_ids = {1539, 1667, 1795, 1923}, .rx_vl_ids = {3, 131, 259, 387}}, /* Port 2 */  \
        {.tx_vlans = {97, 98, 99, 100}, .tx_vlan_count = 4, .rx_vlans = {245, 246, 247, 248}, .rx_vlan_count = 4, .tx_vl_ids = {3, 131, 259, 387}, .rx_vl_ids = {3, 131, 259, 387}},          /* Port 3 */  \
        {.tx_vlans = {101, 102, 103, 104}, .tx_vlan_count = 4, .rx_vlans = {241, 242, 243, 244}, .rx_vlan_count = 4, .tx_vl_ids = {515, 643, 771, 899}, .rx_vl_ids = {3, 131, 259, 387}},                   \
  }

// ==========================================
// ATE TEST MODE - PORT VLAN CONFIGURATION
// ==========================================
// DPDK port VLAN/VL-ID mapping table for ATE test mode.
// Same structure as PORT_VLAN_CONFIG_INIT in normal mode.
// NOTE: These values are placeholders, to be changed according to ATE topology!
// Selected at runtime based on g_ate_mode flag.

#define ATE_PORT_VLAN_CONFIG_INIT                                                                                                                                                                           \
  {                                                                                                                                                                                                         \
    /* Port 0 */                                                                                                                                                                                            \
    {.tx_vlans = {105, 106, 107, 108}, .tx_vlan_count = 4, .rx_vlans = {237, 238, 239, 240}, .rx_vlan_count = 4, .tx_vl_ids = {1027, 1155, 1283, 1411}, .rx_vl_ids = {3, 131, 259, 387}},     /* Port 1 */  \
        {.tx_vlans = {109, 110, 111, 112}, .tx_vlan_count = 4, .rx_vlans = {233, 234, 235, 236}, .rx_vlan_count = 4, .tx_vl_ids = {1539, 1667, 1795, 1923}, .rx_vl_ids = {3, 131, 259, 387}}, /* Port 2 */  \
        {.tx_vlans = {97, 98, 99, 100}, .tx_vlan_count = 4, .rx_vlans = {229, 230, 231, 232}, .rx_vlan_count = 4, .tx_vl_ids = {3, 131, 259, 387}, .rx_vl_ids = {3, 131, 259, 387}},          /* Port 3 */  \
        {.tx_vlans = {101, 102, 103, 104}, .tx_vlan_count = 4, .rx_vlans = {225, 226, 227, 228}, .rx_vlan_count = 4, .tx_vl_ids = {515, 643, 771, 899}, .rx_vl_ids = {3, 131, 259, 387}},                   \
  }

// ==========================================
// TX/RX CORE CONFIGURATION
// ==========================================
// (Can be overridden via Makefile)
#ifndef NUM_TX_CORES
#define NUM_TX_CORES 2
#endif

#ifndef NUM_RX_CORES
#define NUM_RX_CORES 4
#endif

// ==========================================
// PORT-BASED RATE LIMITING
// ==========================================
// All ports use the same target rate

#ifndef TARGET_GBPS
#define TARGET_GBPS 3.6
#endif

#ifndef RATE_LIMITER_ENABLED
#define RATE_LIMITER_ENABLED 1
#endif

// Queue counts equal core counts
#define NUM_TX_QUEUES_PER_PORT NUM_TX_CORES
#define NUM_RX_QUEUES_PER_PORT NUM_RX_CORES

// ==========================================
// PACKET CONFIGURATION (Fixed fields)
// ==========================================
#define DEFAULT_TTL 1
#define DEFAULT_TOS 0
#define DEFAULT_VLAN_PRIORITY 0

// MAC/IP templates
#define DEFAULT_SRC_MAC "02:00:00:00:00:20"  // Fixed source MAC
#define DEFAULT_DST_MAC_PREFIX "03:00:00:00" // Last 2 bytes = VL-ID

#define DEFAULT_SRC_IP "10.0.0.0"       // Fixed source IP
#define DEFAULT_DST_IP_PREFIX "224.224" // Last 2 bytes = VL-ID

// UDP ports
#define DEFAULT_SRC_PORT 100
#define DEFAULT_DST_PORT 100

// ==========================================
// STATISTICS CONFIGURATION
// ==========================================
#define STATS_INTERVAL_SEC 1 // Write statistics every N seconds

// DPDK External TX removed (was for raw socket ports 12/13)
#define DPDK_EXT_TX_ENABLED 0
#define DPDK_EXT_TX_PORT_COUNT 6 // Port 2,3,4,5 → Port 12 | Port 0,6 → Port 13
#define DPDK_EXT_TX_QUEUES_PER_PORT 4

// External TX target configuration
struct dpdk_ext_tx_target
{
  uint16_t queue_id;    // Queue index (0-3)
  uint16_t vlan_id;     // VLAN tag
  uint16_t vl_id_start; // VL-ID start
  uint16_t vl_id_count; // VL-ID count (32)
  uint32_t rate_mbps;   // Target rate (Mbps)
};

// External TX port configuration
struct dpdk_ext_tx_port_config
{
  uint16_t port_id;      // DPDK port ID
  uint16_t dest_port;    // Destination raw socket port (12 or 13)
  uint16_t target_count; // Number of targets (4)
  struct dpdk_ext_tx_target targets[DPDK_EXT_TX_QUEUES_PER_PORT];
};

#if TOKEN_BUCKET_TX_ENABLED
// ==========================================
// TOKEN BUCKET: DPDK External TX → Port 12
// ==========================================
// 4 VL-IDX per VLAN, each VL-IDX sends 1 packet per 1ms
// Per port: 4 VLAN × 4 VL = 16 VL → 16000 pkt/s

// Port 2: VLAN 97-100 → Port 12 (4 VL per VLAN)
#define DPDK_EXT_TX_PORT_2_TARGETS {                                                          \
    {.queue_id = 0, .vlan_id = 97, .vl_id_start = 4291, .vl_id_count = 4, .rate_mbps = 49},   \
    {.queue_id = 1, .vlan_id = 98, .vl_id_start = 4299, .vl_id_count = 4, .rate_mbps = 49},   \
    {.queue_id = 2, .vlan_id = 99, .vl_id_start = 4307, .vl_id_count = 4, .rate_mbps = 49},   \
    {.queue_id = 3, .vlan_id = 100, .vl_id_start = 4315, .vl_id_count = 4, .rate_mbps = 49},  \
}

// Port 3: VLAN 101-104 → Port 12 (4 VL per VLAN)
#define DPDK_EXT_TX_PORT_3_TARGETS {                                                          \
    {.queue_id = 0, .vlan_id = 101, .vl_id_start = 4323, .vl_id_count = 4, .rate_mbps = 49},  \
    {.queue_id = 1, .vlan_id = 102, .vl_id_start = 4331, .vl_id_count = 4, .rate_mbps = 49},  \
    {.queue_id = 2, .vlan_id = 103, .vl_id_start = 4339, .vl_id_count = 4, .rate_mbps = 49},  \
    {.queue_id = 3, .vlan_id = 104, .vl_id_start = 4347, .vl_id_count = 4, .rate_mbps = 49},  \
}

// Port 4: VLAN 113-116 → Port 12 (4 VL per VLAN)
#define DPDK_EXT_TX_PORT_4_TARGETS {                                                          \
    {.queue_id = 0, .vlan_id = 113, .vl_id_start = 4355, .vl_id_count = 4, .rate_mbps = 49},  \
    {.queue_id = 1, .vlan_id = 114, .vl_id_start = 4363, .vl_id_count = 4, .rate_mbps = 49},  \
    {.queue_id = 2, .vlan_id = 115, .vl_id_start = 4371, .vl_id_count = 4, .rate_mbps = 49},  \
    {.queue_id = 3, .vlan_id = 116, .vl_id_start = 4379, .vl_id_count = 4, .rate_mbps = 49},  \
}

// Port 5: VLAN 117-120 → Port 12 (4 VL per VLAN)
#define DPDK_EXT_TX_PORT_5_TARGETS {                                                          \
    {.queue_id = 0, .vlan_id = 117, .vl_id_start = 4387, .vl_id_count = 4, .rate_mbps = 49},  \
    {.queue_id = 1, .vlan_id = 118, .vl_id_start = 4395, .vl_id_count = 4, .rate_mbps = 49},  \
    {.queue_id = 2, .vlan_id = 119, .vl_id_start = 4403, .vl_id_count = 4, .rate_mbps = 49},  \
    {.queue_id = 3, .vlan_id = 120, .vl_id_start = 4411, .vl_id_count = 4, .rate_mbps = 49},  \
}

#else
// ==========================================
// NORMAL MODE: DPDK External TX → Port 12
// ==========================================
// Port 2: VLAN 97-100, VL-ID 4291-4322
// NOTE: Total external TX must not exceed Port 12's 1G capacity
// 4 ports × 220 Mbps = 880 Mbps total (within 1G limit)
#define DPDK_EXT_TX_PORT_2_TARGETS {                                                          \
    {.queue_id = 0, .vlan_id = 97, .vl_id_start = 4291, .vl_id_count = 8, .rate_mbps = 230},  \
    {.queue_id = 1, .vlan_id = 98, .vl_id_start = 4299, .vl_id_count = 8, .rate_mbps = 230},  \
    {.queue_id = 2, .vlan_id = 99, .vl_id_start = 4307, .vl_id_count = 8, .rate_mbps = 230},  \
    {.queue_id = 3, .vlan_id = 100, .vl_id_start = 4315, .vl_id_count = 8, .rate_mbps = 230}, \
}

// Port 3: VLAN 101-104, VL-ID 4323-4354 (8 per queue, no overlap)
#define DPDK_EXT_TX_PORT_3_TARGETS {                                                          \
    {.queue_id = 0, .vlan_id = 101, .vl_id_start = 4323, .vl_id_count = 8, .rate_mbps = 230}, \
    {.queue_id = 1, .vlan_id = 102, .vl_id_start = 4331, .vl_id_count = 8, .rate_mbps = 230}, \
    {.queue_id = 2, .vlan_id = 103, .vl_id_start = 4339, .vl_id_count = 8, .rate_mbps = 230}, \
    {.queue_id = 3, .vlan_id = 104, .vl_id_start = 4347, .vl_id_count = 8, .rate_mbps = 230}, \
}

// Port 4: VLAN 113-116, VL-ID 4355-4386
#define DPDK_EXT_TX_PORT_4_TARGETS {                                                          \
    {.queue_id = 0, .vlan_id = 113, .vl_id_start = 4355, .vl_id_count = 8, .rate_mbps = 230}, \
    {.queue_id = 1, .vlan_id = 114, .vl_id_start = 4363, .vl_id_count = 8, .rate_mbps = 230}, \
    {.queue_id = 2, .vlan_id = 115, .vl_id_start = 4371, .vl_id_count = 8, .rate_mbps = 230}, \
    {.queue_id = 3, .vlan_id = 116, .vl_id_start = 4379, .vl_id_count = 8, .rate_mbps = 230}, \
}

// Port 5: VLAN 117-120, VL-ID 4387-4418 → Port 12
#define DPDK_EXT_TX_PORT_5_TARGETS {                                                          \
    {.queue_id = 0, .vlan_id = 117, .vl_id_start = 4387, .vl_id_count = 8, .rate_mbps = 230}, \
    {.queue_id = 1, .vlan_id = 118, .vl_id_start = 4395, .vl_id_count = 8, .rate_mbps = 230}, \
    {.queue_id = 2, .vlan_id = 119, .vl_id_start = 4403, .vl_id_count = 8, .rate_mbps = 230}, \
    {.queue_id = 3, .vlan_id = 120, .vl_id_start = 4411, .vl_id_count = 8, .rate_mbps = 230}, \
}
#endif

// ==========================================
// PORT 0 and PORT 6 → PORT 13 (100M copper)
// ==========================================
// Port 0: 45 Mbps, Port 6: 45 Mbps = Total 90 Mbps

#if TOKEN_BUCKET_TX_ENABLED
// ==========================================
// TOKEN BUCKET: DPDK External TX → Port 13
// ==========================================
// Port 0: 3 VLAN × 1 VL = 3 VL → 3000 pkt/s (VLAN 108 excluded)
// Port 6: 3 VLAN × 1 VL = 3 VL → 3000 pkt/s (VLAN 124 excluded)
#define DPDK_EXT_TX_PORT_0_TARGETS {                                                         \
    {.queue_id = 0, .vlan_id = 105, .vl_id_start = 4099, .vl_id_count = 1, .rate_mbps = 13}, \
    {.queue_id = 1, .vlan_id = 106, .vl_id_start = 4103, .vl_id_count = 1, .rate_mbps = 13}, \
    {.queue_id = 2, .vlan_id = 107, .vl_id_start = 4107, .vl_id_count = 1, .rate_mbps = 13}, \
}

#define DPDK_EXT_TX_PORT_6_TARGETS {                                                         \
    {.queue_id = 0, .vlan_id = 121, .vl_id_start = 4115, .vl_id_count = 1, .rate_mbps = 13}, \
    {.queue_id = 1, .vlan_id = 122, .vl_id_start = 4119, .vl_id_count = 1, .rate_mbps = 13}, \
    {.queue_id = 2, .vlan_id = 123, .vl_id_start = 4123, .vl_id_count = 1, .rate_mbps = 13}, \
}

#else
// Port 0: VLAN 105-108, VL-ID 4099-4114 → Port 13 (total 45 Mbps)
#define DPDK_EXT_TX_PORT_0_TARGETS {                                                         \
    {.queue_id = 0, .vlan_id = 105, .vl_id_start = 4099, .vl_id_count = 4, .rate_mbps = 45}, \
    {.queue_id = 1, .vlan_id = 106, .vl_id_start = 4103, .vl_id_count = 4, .rate_mbps = 45}, \
    {.queue_id = 2, .vlan_id = 107, .vl_id_start = 4107, .vl_id_count = 4, .rate_mbps = 45}, \
    {.queue_id = 3, .vlan_id = 108, .vl_id_start = 4111, .vl_id_count = 4, .rate_mbps = 45}, \
}

// Port 6: VLAN 121-124, VL-ID 4115-4130 → Port 13 (total 45 Mbps)
#define DPDK_EXT_TX_PORT_6_TARGETS {                                                         \
    {.queue_id = 0, .vlan_id = 121, .vl_id_start = 4115, .vl_id_count = 4, .rate_mbps = 45}, \
    {.queue_id = 1, .vlan_id = 122, .vl_id_start = 4119, .vl_id_count = 4, .rate_mbps = 45}, \
    {.queue_id = 2, .vlan_id = 123, .vl_id_start = 4123, .vl_id_count = 4, .rate_mbps = 45}, \
    {.queue_id = 3, .vlan_id = 124, .vl_id_start = 4127, .vl_id_count = 4, .rate_mbps = 45}, \
}
#endif

// All external TX port configurations
// Port 2,3,4,5 → Port 12 (1G) | Port 0,6 → Port 13 (100M)
#if TOKEN_BUCKET_TX_ENABLED
// Token bucket: Port 0,6 have 3 targets (VLAN 108/124 excluded from P13)
#define DPDK_EXT_TX_PORTS_CONFIG_INIT {                                                        \
    {.port_id = 2, .dest_port = 12, .target_count = 4, .targets = DPDK_EXT_TX_PORT_2_TARGETS}, \
    {.port_id = 3, .dest_port = 12, .target_count = 4, .targets = DPDK_EXT_TX_PORT_3_TARGETS}, \
    {.port_id = 4, .dest_port = 12, .target_count = 4, .targets = DPDK_EXT_TX_PORT_4_TARGETS}, \
    {.port_id = 5, .dest_port = 12, .target_count = 4, .targets = DPDK_EXT_TX_PORT_5_TARGETS}, \
    {.port_id = 0, .dest_port = 13, .target_count = 3, .targets = DPDK_EXT_TX_PORT_0_TARGETS}, \
    {.port_id = 6, .dest_port = 13, .target_count = 3, .targets = DPDK_EXT_TX_PORT_6_TARGETS}, \
}
#else
#define DPDK_EXT_TX_PORTS_CONFIG_INIT {                                                        \
    {.port_id = 2, .dest_port = 12, .target_count = 4, .targets = DPDK_EXT_TX_PORT_2_TARGETS}, \
    {.port_id = 3, .dest_port = 12, .target_count = 4, .targets = DPDK_EXT_TX_PORT_3_TARGETS}, \
    {.port_id = 4, .dest_port = 12, .target_count = 4, .targets = DPDK_EXT_TX_PORT_4_TARGETS}, \
    {.port_id = 5, .dest_port = 12, .target_count = 4, .targets = DPDK_EXT_TX_PORT_5_TARGETS}, \
    {.port_id = 0, .dest_port = 13, .target_count = 4, .targets = DPDK_EXT_TX_PORT_0_TARGETS}, \
    {.port_id = 6, .dest_port = 13, .target_count = 4, .targets = DPDK_EXT_TX_PORT_6_TARGETS}, \
}
#endif

// ==========================================
// DTN PORT-BASED STATISTICS MODE
// ==========================================
// STATS_MODE_DTN=1: DTN per-port statistics table (16 rows, DTN Port 0-15)
//   - RX queue steering: rte_flow VLAN match (each queue = 1 VLAN = 1 DTN port)
//   - Zero overhead Gbps calculation via HW per-queue stats
//   - PRBS validation per DTN port
//
// STATS_MODE_DTN=0: Legacy server per-port table (4 rows, Server Port 0-3)
//   - RX queue steering: RSS (hash based)
//   - HW total port stats
//   - PRBS validation per server port

#ifndef STATS_MODE_DTN
#define STATS_MODE_DTN 1
#endif

// DTN port count: 16 DPDK ports (4 server ports × 4 VLANs)
#define DTN_PORT_COUNT 16
#define DTN_DPDK_PORT_COUNT 16

// 1 VLAN per DTN port
#define DTN_VLANS_PER_PORT 1

// ==========================================
// DTN PORT MAPPING TABLE
// ==========================================
// Each DTN port: TX/RX from DTN perspective
//   DTN RX = Server sends → DTN receives (server_tx_port, rx_vlan)
//   DTN TX = DTN sends → Server receives (server_rx_port, tx_vlan)
//
// DTN Port 0-15: DPDK ports (4 ports × 4 VLANs)
// Pairs: Port 0↔1, Port 2↔3

struct dtn_port_map_entry {
    uint16_t dtn_port_id;       // DTN port number (0-15)

    // DTN RX (Server → DTN): Server sends from this VLAN
    uint16_t rx_vlan;           // Server TX VLAN (DTN receives this VLAN)
    uint16_t rx_server_port;    // Server DPDK port (the one that TXs)
    uint16_t rx_server_queue;   // Server TX queue index (0-3)

    // DTN TX (DTN → Server): DTN sends from this VLAN
    uint16_t tx_vlan;           // Server RX VLAN (DTN sends from this VLAN)
    uint16_t tx_server_port;    // Server DPDK port (the one that RXs)
    uint16_t tx_server_queue;   // Server RX queue index (0-3)
};

// DTN Port Mapping Table (4 server ports × 4 VLANs = 16 DTN ports)
// Pairs: Port 0↔1, Port 2↔3
// Format: {dtn_port, rx_vlan, rx_srv_port, rx_srv_queue, tx_vlan, tx_srv_port, tx_srv_queue}
#define DTN_PORT_MAP_INIT {                                                                         \
    /* DTN 0-3:   Server TX=Port0(VLAN 105-108), Server RX=Port1(VLAN 249-252) */                   \
    {.dtn_port_id = 0,  .rx_vlan = 105, .rx_server_port = 0, .rx_server_queue = 0,                  \
                         .tx_vlan = 249, .tx_server_port = 1, .tx_server_queue = 0},                 \
    {.dtn_port_id = 1,  .rx_vlan = 106, .rx_server_port = 0, .rx_server_queue = 1,                  \
                         .tx_vlan = 250, .tx_server_port = 1, .tx_server_queue = 1},                 \
    {.dtn_port_id = 2,  .rx_vlan = 107, .rx_server_port = 0, .rx_server_queue = 2,                  \
                         .tx_vlan = 251, .tx_server_port = 1, .tx_server_queue = 2},                 \
    {.dtn_port_id = 3,  .rx_vlan = 108, .rx_server_port = 0, .rx_server_queue = 3,                  \
                         .tx_vlan = 252, .tx_server_port = 1, .tx_server_queue = 3},                 \
    /* DTN 4-7:   Server TX=Port1(VLAN 109-112), Server RX=Port0(VLAN 253-256) */                   \
    {.dtn_port_id = 4,  .rx_vlan = 109, .rx_server_port = 1, .rx_server_queue = 0,                  \
                         .tx_vlan = 253, .tx_server_port = 0, .tx_server_queue = 0},                 \
    {.dtn_port_id = 5,  .rx_vlan = 110, .rx_server_port = 1, .rx_server_queue = 1,                  \
                         .tx_vlan = 254, .tx_server_port = 0, .tx_server_queue = 1},                 \
    {.dtn_port_id = 6,  .rx_vlan = 111, .rx_server_port = 1, .rx_server_queue = 2,                  \
                         .tx_vlan = 255, .tx_server_port = 0, .tx_server_queue = 2},                 \
    {.dtn_port_id = 7,  .rx_vlan = 112, .rx_server_port = 1, .rx_server_queue = 3,                  \
                         .tx_vlan = 256, .tx_server_port = 0, .tx_server_queue = 3},                 \
    /* DTN 8-11:  Server TX=Port2(VLAN 97-100),  Server RX=Port3(VLAN 241-244) */                   \
    {.dtn_port_id = 8,  .rx_vlan = 97,  .rx_server_port = 2, .rx_server_queue = 0,                  \
                         .tx_vlan = 241, .tx_server_port = 3, .tx_server_queue = 0},                 \
    {.dtn_port_id = 9,  .rx_vlan = 98,  .rx_server_port = 2, .rx_server_queue = 1,                  \
                         .tx_vlan = 242, .tx_server_port = 3, .tx_server_queue = 1},                 \
    {.dtn_port_id = 10, .rx_vlan = 99,  .rx_server_port = 2, .rx_server_queue = 2,                  \
                         .tx_vlan = 243, .tx_server_port = 3, .tx_server_queue = 2},                 \
    {.dtn_port_id = 11, .rx_vlan = 100, .rx_server_port = 2, .rx_server_queue = 3,                  \
                         .tx_vlan = 244, .tx_server_port = 3, .tx_server_queue = 3},                 \
    /* DTN 12-15: Server TX=Port3(VLAN 101-104), Server RX=Port2(VLAN 245-248) */                   \
    {.dtn_port_id = 12, .rx_vlan = 101, .rx_server_port = 3, .rx_server_queue = 0,                  \
                         .tx_vlan = 245, .tx_server_port = 2, .tx_server_queue = 0},                 \
    {.dtn_port_id = 13, .rx_vlan = 102, .rx_server_port = 3, .rx_server_queue = 1,                  \
                         .tx_vlan = 246, .tx_server_port = 2, .tx_server_queue = 1},                 \
    {.dtn_port_id = 14, .rx_vlan = 103, .rx_server_port = 3, .rx_server_queue = 2,                  \
                         .tx_vlan = 247, .tx_server_port = 2, .tx_server_queue = 2},                 \
    {.dtn_port_id = 15, .rx_vlan = 104, .rx_server_port = 3, .rx_server_queue = 3,                  \
                         .tx_vlan = 248, .tx_server_port = 2, .tx_server_queue = 3},                 \
}

// VLAN → DTN port lookup (fast access)
// Index = VLAN ID, Value = DTN port number (0xFF = undefined)
#define DTN_VLAN_LOOKUP_SIZE 257  // VLAN 0-256
#define DTN_VLAN_INVALID 0xFF

#endif /* CONFIG_H */
