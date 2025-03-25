⭐ GSoC 2025 with P4Language: Check out the [project idea](https://github.com/p4lang/gsoc/blob/main/2025/ideas_list.md#project-5) and become a contributor to P4 through Google Summer of Code!

# P4sim module in ns-3 for Programmable Data Plane simulation

P4sim is a P4-driven network simulator aiming at combining P4, the state-of-the-art programmable data plane language and ns-3, one of the most popular network simulators. P4sim is an open-source project under Apache License 2.0.

P4sim with reference to behavioral models: [behavioral-model](https://github.com/p4lang/behavioral-model).

Our implementation was built upon the the P4-driven Network Simulator Module, as outlined in:
> Bai, Jiasong, Jun Bi, Peng Kuang, Chengze Fan, Yu Zhou, and Cheng Zhang. **"NS4: Enabling programmable data plane simulation."** In Proceedings of the Symposium on SDN Research, pp. 1-7. 2018. Available at [ACM DL](https://dl.acm.org/doi/abs/10.1145/3185467.3185470)

> Fan, Chengze, Jun Bi, Yu Zhou, Cheng Zhang, and Haisu Yu. **"NS4: A P4-driven network simulator."** In Proceedings of the SIGCOMM Posters and Demos, pp. 105-107. 2017. Available at [ACM DL](https://dl.acm.org/doi/10.1145/3123878.3132002)

⭐ More detail check our paper submitted on 21 March 2025: [P4sim: Programming Protocol-independent Packet Processors in ns-3](https://arxiv.org/abs/2503.17554)

## P4Sim: NS-3-Based P4 Simulation Environment

See [here](doc/vm-env.md).

## Simulation Parameters for P4 switch ##

In a P4 switch, the functionality is defined by the user's P4 program and the configuration of the flow table. But the following parameters can also be configured in ns-3 `ns3::P4SwitchNetDevice`.

| Parameter             | Description                                                         |
|-----------------------|----------------------------------------------------------------------|
| EnableTracing         | Enable or disable tracing in the switch                              |
| EnableSwap            | Enable or disable swapping of the P4 configuration                   |
| P4SwitchArch          | Switch architecture: 0 for v1model, 1 for PSA, 2 for PNA             |
| JsonPath              | Path to the compiled P4 JSON file (*.json)                           |
| FlowTablePath         | Path to the flow table configuration file                            |
| InputBufferSizeLow    | Input buffer size for low-priority packets (external packets)        |
| InputBufferSizeHigh   | Input buffer size for high-priority packets (internal packets)       |
| QueueBufferSize       | Total size of the queue buffer                                       |
| SwitchRate            | Switch processing rate in packets per second (pps)                   |
| ChannelType           | Channel type: 0 for CSMA, 1 for point-to-point (P2P), default is CSMA|

Note: 1. When using a CSMA channel, make sure the ARP packets are correctly handled in the P4 scripts.
    2. Buffer configuration only useful if the P4SwitchArch include that buffer.
    3. EnableTracing: The implementation is not yet complete and currently only supports basic measurements such as throughput.

## Simulation Examples: ##

See [here](doc/examples.md).

## Results Subdirectory ##



## Doxygen Documents ##

```bash
# install the doxygen, check the version
sudo apt update
sudo apt install doxygen graphviz dia
doxygen --version
# after install, reconfigure ns-3
./ns3 configure --enable-tests --enable-examples --enable-python-bindings
./ns3 build
./ns3 docs doxygen
# the doc will gene in ./doc/html/index.html
xdg-open build/doxygen/html/index.html

# PS: if using vm, could using the http server, add port forwarding in Vritualbox config.
# like port 127.0.0.1:2233 -> 8080
python3 -m http.server --directory ./doc/html 8080 # set in vm (remote)
http://127.0.0.1:2233 # open in local host
```