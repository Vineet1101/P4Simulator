[Quick Note with Google Docs](https://docs.google.com/document/d/18QQqo4PK8ycuZovrbAacRtFtqWSIqS0v9hRiiUYgvU8/edit?usp=sharing)

# P4sim module in ns-3 for Programmable Data Plane simulation

P4sim is a P4-driven network simulator aiming at combining P4, the state-of-the-art programmable data plane language and ns-3, one of the most popular network simulators. P4sim is an open-source project under Apache License 2.0.

Our implementation was built upon the the P4-driven Network Simulator Module, as outlined in:
> Bai, Jiasong, Jun Bi, Peng Kuang, Chengze Fan, Yu Zhou, and Cheng Zhang. **NS4: Enabling programmable data plane simulation.** In Proceedings of the Symposium on SDN Research, pp. 1-7. 2018. Available at [ACM DL](https://dl.acm.org/doi/abs/10.1145/3185467.3185470)


### Installation and usage instructions ###

The recommended operating system for installing this implementation is Ubuntu 24.04 LTS.


## Virtual Machine as virtual env

`p4sim` integrates an NS-3-based P4 simulation environment with virtual machine configuration files sourced via sparse checkout from the [P4Lang tutorials repository](https://github.com/p4lang/tutorials/tree/master). The `vm` directory contains setup files, including Vagrant configurations and bootstrap scripts, for Ubuntu-based virtual machines (`24.04` Recommended) that simplify the process of installing NS-3 and its dependencies. Using these pre-configured virtual machines is highly recommended for a streamlined setup, ensuring compatibility and minimizing installation issues.

This code is tested with the following commit from the P4Lang tutorials repository:
- Commit: `3acf7d1e9bb7635c76cc8fc60cd34f9919bb59d4`
- Date: `2024-Dec-01`
