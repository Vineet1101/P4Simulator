[Quick Note with Google Docs](https://docs.google.com/document/d/18QQqo4PK8ycuZovrbAacRtFtqWSIqS0v9hRiiUYgvU8/edit?usp=sharing)

# P4sim module in ns-3 for Programmable Data Plane simulation

P4sim is a P4-driven network simulator aiming at combining P4, the state-of-the-art programmable data plane language and ns-3, one of the most popular network simulators. P4sim is an open-source project under Apache License 2.0.

Our implementation was built upon the the P4-driven Network Simulator Module, as outlined in:
> Bai, Jiasong, Jun Bi, Peng Kuang, Chengze Fan, Yu Zhou, and Cheng Zhang. **NS4: Enabling programmable data plane simulation.** In Proceedings of the Symposium on SDN Research, pp. 1-7. 2018. Available at [ACM DL](https://dl.acm.org/doi/abs/10.1145/3185467.3185470)

## P4Sim: NS-3-Based P4 Simulation Environment

### Installation & Usage Guide

It is recommended to use a **virtual machine with Vagrant** to simplify the installation and ensure compatibility.  

### Virtual Machine as virtual env ###

`p4sim` integrates an **NS-3-based P4 simulation environment** with virtual machine configuration files sourced via sparse checkout from the [P4Lang Tutorials repository](https://github.com/p4lang/tutorials/tree/master).  

The `vm` directory contains Vagrant configurations and bootstrap scripts for Ubuntu-based virtual machines (**Ubuntu 24.04 recommended**). These pre-configured environments streamline the setup process, ensuring compatibility and reducing installation issues.  

**Tested with:**  
- **P4Lang Tutorials Commit:** `7273da1c2ac2fd05cea0a9dd0504184b8c955eae`  
- **Date:** `2025-01-25`  

### **Setup Instructions**  

#### **1. Build the Virtual Machine**  
```bash
vagrant up
```

#### **2. Clone the NS-3 Repository**  
```bash
git clone https://gitlab.com/nsnam/ns-3.git ns3
```

#### **3. Clone & Integrate `p4sim` into NS-3**  
```bash
git clone https://github.com/your-repo/p4sim.git
mv p4sim ns3/contrib/
```

#### **4. Set Up the Environment**  
```bash
cd ns3/contrib/p4sim
./set_pkg_config_env.sh
```

#### **5. Configure & Build NS-3**  
```bash
cd ../../  # Move to the NS-3 root directory
./waf configure --enable-examples --enable-tests
./waf build
```

#### **6. Run a Simulation Example**  
```bash
./waf --run "exampleA"
```

---

### **Notes**  
- Ensure you have **Vagrant** and **VirtualBox** installed before running `vagrant up`.  
- The setup script (`set_pkg_config_env.sh`) configures the required environment variables for P4Sim.  
- **Ubuntu 24.04** is the recommended OS for the virtual machine.  

---

## Simulation Parameters ##

## Simulation Script: ##

## Results Subdirectory ##