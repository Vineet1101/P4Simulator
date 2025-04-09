# P4Sim: NS-3-Based P4 Simulation Environment

## Installation & Usage Guide

It is recommended to use a **virtual machine** with Vagrant to simplify the installation and ensure compatibility. 

# Virtual Machine as virtual env ##

`p4sim` integrates an NS-3-based P4 simulation environment with virtual machine configuration files sourced via sparse checkout from the [P4Lang Tutorials repository](https://github.com/p4lang/tutorials/tree/master).  

The `vm` directory contains Vagrant configurations and bootstrap scripts for Ubuntu-based virtual machines (Ubuntu 24.04 recommended). These pre-configured environments streamline the setup process, ensuring compatibility and reducing installation issues.  

Tested with:  
- P4Lang Tutorials Commit: `7273da1c2ac2fd05cea0a9dd0504184b8c955eae`  
- Date: `2025-01-25`

Notes:  
- Ensure you have `Vagrant` and `VirtualBox` installed before running `vagrant up dev`.
- The setup script (`set_pkg_config_env.sh`) configures the required environment variables for P4Sim.
- `Ubuntu 24.04` is the recommended OS for the virtual machine.

---

## Setup Instructions for ns-3 version 3.x - 3.35 (Build with `waf`)  

This has been tested with ns-3 repo Tag `ns-3.35`.

### 1. Build the Virtual Machine  
```bash
# with vm-ubuntu-24.04/Vagrantfile or vm-ubuntu-20.04/Vagrantfile
vagrant up dev
```

Please also **check the webpage**: [Introduction build venv of vm-ubuntu-24.04](https://github.com/p4lang/tutorials/tree/7273da1c2ac2fd05cea0a9dd0504184b8c955eae/vm-ubuntu-24.04#introduction), current version you need to install the tools by yourself: [install](https://github.com/p4lang/tutorials/tree/7273da1c2ac2fd05cea0a9dd0504184b8c955eae/vm-ubuntu-24.04#installing-open-source-p4-development-tools-on-the-vm)

This will create a virtual machine with name "P4 Tutorial Development" with the date. Please **test with `simple_switch` command**, to test if the `bmv2` is correct installed. Later we use the libs.

### 2. Clone the NS-3 Repository  
```bash
cd
mkdir workdir
cd workdir
git clone https://github.com/nsnam/ns-3-dev-git.git ns3.35
cd ns3.35
git checkout ns-3.35 
```

### 3. Clone & Integrate `p4sim` into NS-3  
```bash
cd /home/p4/workdir/ns3.35/contrib/
git clone https://github.com/HapCommSys/p4sim.git
```

### 4. Set Up the Environment (for external libs)
```bash
cd /home/p4/workdir/ns3.35/contrib/p4sim/ # p4sim root directory
git checkout waf
sudo ./set_pkg_config_env.sh
```

### 5. Patch for the ns-3 code
```bash
cd ../../ # in ns-3 root directory
git apply ./contrib/p4sim/doc/changes.patch 
```

### 6. Configure & Build NS-3  
```bash
# in ns-3 root directory
./waf configure --enable-examples --enable-tests
./waf build
```

### 7. Run a Simulation Example  
```bash
./waf --run "exampleA" # This will run exampleA (name).

# In the p4sim example, you may need to adjust the path of p4 and other files.
# For example:
# std::string p4JsonPath =
#       "/home/p4/workdir/ns3.35/contrib/p4sim/test/test_simple/test_simple.json";
#   std::string flowTablePath =
#       "/home/p4/workdir/ns3.35/contrib/p4sim/test/test_simple/flowtable_0.txt";
#   std::string topoInput = "/home/p4/workdir/ns3.35/contrib/p4sim/test/test_simple/topo.txt";
```

---

## Setup Instructions for ns-3 version 3.36 - 3.39 (Build with cmake)

This has been tested with ns-3 repo Tag `ns-3.39`. Because the virtual machine will build BMv2 and libs with **C++17**, and ns-3 p4simulator using the external inlucde file and libs, therefore the ns-3 also need to build with **C++17**.
The include file is: `/usr/local/include/bm`, the libs is `/usr/local/lib/libbmall.so`.


### 1. Build the Virtual Machine  
```bash
# with vm-ubuntu-24.04/Vagrantfile or vm-ubuntu-20.04/Vagrantfile
vagrant up dev
```

Please also **check the webpage**: [Introduction build venv of vm-ubuntu-24.04](https://github.com/p4lang/tutorials/tree/7273da1c2ac2fd05cea0a9dd0504184b8c955eae/vm-ubuntu-24.04#introduction), current version you need to install the tools by yourself: [install](https://github.com/p4lang/tutorials/tree/7273da1c2ac2fd05cea0a9dd0504184b8c955eae/vm-ubuntu-24.04#installing-open-source-p4-development-tools-on-the-vm)

This will create a virtual machine with name "P4 Tutorial Development" with the date. Please **test with `simple_switch` command**, to test if the `bmv2` is correct installed. Later we use the libs.

### 2. install Cmake
```bash
sudo apt update
sudo apt install cmake

```

### 3. Clone the NS-3 Repository  
```bash
cd
mkdir workdir
cd workdir
git clone https://github.com/nsnam/ns-3-dev-git.git ns3.39
cd ns3.39
git checkout ns-3.39
```

### 4. Clone & Integrate `p4sim` into NS-3  
```bash
cd /home/p4/workdir/ns3.39/contrib/
git clone https://github.com/HapCommSys/p4sim.git
```

### 5. Set Up the Environment (for external libs)
```bash
cd /home/p4/workdir/ns3.39/contrib/p4sim/ # p4sim root directory
sudo ./set_pkg_config_env.sh
```

### 6. Configure & Build NS-3  
```bash
# in ns-3 root directory
./ns3 configure --enable-tests --enable-examples
./ns3 build
```

### 7. Run a Simulation Example  
```bash
./waf --run "exampleA" # This will run exampleA (name).

# In the p4sim example, you may need to adjust the path of p4 and other files.
# For example:
# std::string p4JsonPath =
#       "/home/p4/workdir/ns3.35/contrib/p4sim/test/test_simple/test_simple.json";
#   std::string flowTablePath =
#       "/home/p4/workdir/ns3.35/contrib/p4sim/test/test_simple/flowtable_0.txt";
#   std::string topoInput = "/home/p4/workdir/ns3.35/contrib/p4sim/test/test_simple/topo.txt";
```

---

# Install on a local machine

Not been tested yet.

# References

[1] [add_specific_folder_with_submodule_to_a_repository](https://www.reddit.com/r/git/comments/sme7k4/add_specific_folder_with_submodule_to_a_repository/)
[2] [P4Lang Tutorials repository](https://github.com/p4lang/tutorials/tree/master)

# Appendix

```bash
# set_pkg_config_env.sh, the p4simulator module requires the first (bm)
pkg-config --list-all | grep xxx
    bm                             BMv2 - Behavioral Model
    simple_switch                  simple switch - Behavioral Model Target Simple Switch
    boost_system                   Boost System - Boost System

# SPD log from BMv2 should be blocked, ns-3 has it's own logging system.

```
