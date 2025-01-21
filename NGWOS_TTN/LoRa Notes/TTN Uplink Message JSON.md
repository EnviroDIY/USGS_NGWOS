https://www.thethingsindustries.com/docs/the-things-stack/concepts/data-formats/

{
  "end_device_ids" : {
    "device_id" : "dev1",                    // Device ID
    "application_ids" : {
      "application_id" : "app1"              // Application ID
    },
    "dev_eui" : "0004A30B001C0530",          // DevEUI of the end device
    "join_eui" : "800000000000000C",         // JoinEUI of the end device (also known as AppEUI in LoRaWAN versions below 1.1)
    "dev_addr" : "00BCB929"                  // Device address known by the Network Server
  },
  "correlation_ids" : [ "as:up:01...", ... ],// Correlation identifiers of the message
  "received_at" : "2020-02-12T15:15..."      // ISO 8601 UTC timestamp at which the message has been received by the Application Server
  "uplink_message" : {
    "session_key_id": "AXA50...",            // Join Server issued identifier for the session keys used by this uplink
    "f_cnt": 1,                              // Frame counter
    "f_port": 1,                             // Frame port
    "frm_payload": "gkHe",                   // Frame payload (Base64)
    "decoded_payload" : {                    // Decoded payload object, decoded by the device payload formatter
      "temperature": 1.0,
      "luminosity": 0.64
    },
    "rx_metadata": [{                        // A list of metadata for each antenna of each gateway that received this message
      "gateway_ids": {
        "gateway_id": "gtw1",                // Gateway ID
        "eui": "9C5C8E00001A05C4"            // Gateway EUI
      },
      "time": "2020-02-12T15:15:45.787Z",    // ISO 8601 UTC timestamp at which the uplink has been received by the gateway
      "timestamp": 2463457000,               // Timestamp of the gateway concentrator when the message has been received
      "rssi": -35,                           // Received signal strength indicator (dBm)
      "channel_rssi": -35,                   // Received signal strength indicator of the channel (dBm)
      "snr": 5.2,                            // Signal-to-noise ratio (dB)
      "uplink_token": "ChIKEA...",           // Uplink token injected by gateway, Gateway Server or fNS
      "channel_index": 2                     // Index of the gateway channel that received the message
      "location": {                          // Gateway location metadata (only for gateways with location set to public)
        "latitude": 37.97155556731436,       // Location latitude
        "longitude": 23.72678801175413,      // Location longitude
        "altitude": 2,                       // Location altitude
        "source": "SOURCE_REGISTRY"          // Location source. SOURCE_REGISTRY is the location from the Identity Server.
      }
    }],
    "settings": {                            // Settings for the transmission
      "data_rate": {                         // Data rate settings
        "lora": {                            // LoRa modulation settings
          "bandwidth": 125000,               // Bandwidth (Hz)
          "spreading_factor": 7              // Spreading factor
        }
      },
      "coding_rate": "4/6",                  // LoRa coding rate
      "frequency": "868300000",              // Frequency (Hz)
    },
    "received_at": "2020-02-12T15:15..."     // ISO 8601 UTC timestamp at which the uplink has been received by the Network Server
    "consumed_airtime": "0.056576s",         // Time-on-air, calculated by the Network Server using payload size and transmission settings
    "locations": {                           // End device location metadata
      "user": {
        "latitude": 37.97155556731436,       // Location latitude
        "longitude": 23.72678801175413,      // Location longitude
        "altitude": 10,                      // Location altitude
        "source": "SOURCE_REGISTRY"          // Location source. SOURCE_REGISTRY is the location from the Identity Server.
      }
    },
    "version_ids": {                          // End device version information
        "brand_id": "the-things-products",    // Device brand
        "model_id": "the-things-uno",         // Device model
        "hardware_version": "1.0",            // Device hardware version
        "firmware_version": "quickstart",     // Device firmware version
        "band_id": "EU_863_870"               // Supported band ID
    },
    "network_ids": {                          // Network information
      "net_id": "000013",                     // Network ID
      "tenant_id": "tenant1",                 // Tenant ID
      "cluster_id": "eu1"                     // Cluster ID
    }
  },
  "simulated": true,                         // Signals if the message is coming from the Network Server or is simulated.
}
