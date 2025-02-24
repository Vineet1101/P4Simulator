
```c++
  ===== Here the packets size is 1000 bytes (This is sending rate in Application layer, when add
  header underlayer will be 1000 + 46 bytes (Check the PCAP file) =====`

  pps
  P4GlobalVar::g_switchBottleNeck =
      (uint64_t) (congestion_bottleneck * 1000 * 1000 / ((pktSize + 46) * 8));
  P4GlobalVar::g_switchBottleNeck =
      (uint64_t) (congestion_bottleneck * 1000 * 1000 / (pktSize * 8));
  NS_LOG_INFO ("*** Congestion bottleneck: "
               << congestion_bottleneck << " Mbps, packet size: " << pktSize
               << " Bytes, switch bottleneck: " << P4GlobalVar::g_switchBottleNeck << " pps");
```