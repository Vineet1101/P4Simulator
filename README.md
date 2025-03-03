[Quick Note with Google Docs](https://docs.google.com/document/d/18QQqo4PK8ycuZovrbAacRtFtqWSIqS0v9hRiiUYgvU8/edit?usp=sharing)

# P4sim module in ns-3 for Programmable Data Plane simulation

P4sim is a P4-driven network simulator aiming at combining P4, the state-of-the-art programmable data plane language and ns-3, one of the most popular network simulators. P4sim is an open-source project under Apache License 2.0.

Our implementation was built upon the the P4-driven Network Simulator Module, as outlined in:
> Bai, Jiasong, Jun Bi, Peng Kuang, Chengze Fan, Yu Zhou, and Cheng Zhang. **NS4: Enabling programmable data plane simulation.** In Proceedings of the Symposium on SDN Research, pp. 1-7. 2018. Available at [ACM DL](https://dl.acm.org/doi/abs/10.1145/3185467.3185470)

# P4Sim: NS-3-Based P4 Simulation Environment

See `vm-env.md`.

## Simulation Parameters ##

## Simulation Script: ##

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