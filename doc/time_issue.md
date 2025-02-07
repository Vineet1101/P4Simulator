## `pcap` and tracing

When measuring throughput and time, skip the initial ARP packets sent over the network.

```
(p4dev-python-venv) p4@p4:~/workdir/ns3.35$ ./waf --run "p4-ipv4-forwarding-test --pcap=true"
/home/p4/workdir/ns3.35/bindings/python/wscript:321: SyntaxWarning: invalid escape sequence '\d'
  m = re.match( "^castxml version (\d\.\d)(-)?(\w+)?", castxml_version_line)
Waf: Entering directory `/home/p4/workdir/ns3.35/build'
Waf: Leaving directory `/home/p4/workdir/ns3.35/build'
Build commands will be stored in build/compile_commands.json
'build' finished successfully (0.757s)
*** Congestion bottleneck: 1000 Mbps, packet size: 1000 pps, switch bottleneck: 119502 ns
*** Reading topology from file: /home/p4/workdir/ns3.35/contrib/p4sim/test/test_simple/topo.txt with format: CsmaTopo
*** Host number: 2, Switch number: 1
*** Link from host 1 to  switch0 with data rate  and delay 
*** Link from host 2 to  switch0 with data rate  and delay 
host 0: connect with switch 0 port 0
host 1: connect with switch 0 port 1
switch 0 connect with: h0 h1 
Node IP and MAC addresses:
Node 0: IP = 10.1.1.1, MAC = 00:00:00:00:00:01
Node 0: IP = 0x0a010101, MAC = 0x000000000001
Node 1: IP = 10.1.1.2, MAC = 00:00:00:00:00:03
Node 1: IP = 0x0a010102, MAC = 0x000000000003
*** P4Simulator mode
*** Installing P4 bridge [ 0 ]device with configuration: 
P4JsonPath = /home/p4/workdir/ns3.35/contrib/p4sim/test/basic/basic.json, 
FlowTablePath = /home/p4/workdir/ns3.35/contrib/p4sim/test/test_simple/flowtable_0.txt, 
ViewFlowTablePath = /home/p4/workdir/ns3.35/contrib/p4sim/test/test_simple/flowtable_0.txt
Running simulation...
Simulate Running time: 2706ms
Total Running time: 7933ms
Run successfully!
client_start_time: 3.008client_stop_time: 5.992sink_start_time: 3.01405sink_stop_time: 5.99202
======================================
Final Simulation Results:
Total Transmitted Bytes: 374000 bytes in time 2.984
Total Received Bytes: 374000 bytes in time 2.97797
Final Transmitted Throughput: 1.00268 Mbps
Final Received Throughput: 1.00471 Mbps
======================================


PCAP In sender side (0-0): 
first ARP pkt: 03:014000
first UDP pkt: 03:014028  (ID: 0x0000)
last UDP pkt: 05:992002 (ID: 0x0175)

PCAP In receiver side (2-0): 
first ARP pkt: 03:014011
first UDP pkt: 03:014053  (ID: 0x0000)
last UDP pkt: 05:992023 (ID: 0x0175)
```