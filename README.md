# p4 simulator module

[Quick Note with Google Docs](https://docs.google.com/document/d/18QQqo4PK8ycuZovrbAacRtFtqWSIqS0v9hRiiUYgvU8/edit?usp=sharing)


## Virtual Machine

`p4sim` integrates an NS-3-based P4 simulation environment with virtual machine configuration files sourced via sparse checkout from the [P4Lang tutorials repository](https://github.com/p4lang/tutorials/tree/master). The `vm` directory contains setup files, including Vagrant configurations and bootstrap scripts, for Ubuntu-based virtual machines (`20.04` and `24.04`) that simplify the process of installing NS-3 and its dependencies. Using these pre-configured virtual machines is highly recommended for a streamlined setup, ensuring compatibility and minimizing installation issues.