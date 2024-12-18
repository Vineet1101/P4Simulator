# p4 simulator module

[Quick Note with Google Docs](https://docs.google.com/document/d/18QQqo4PK8ycuZovrbAacRtFtqWSIqS0v9hRiiUYgvU8/edit?usp=sharing)

## Virtual Machine as virtual env

`p4sim` integrates an NS-3-based P4 simulation environment with virtual machine configuration files sourced via sparse checkout from the [P4Lang tutorials repository](https://github.com/p4lang/tutorials/tree/master). The `vm` directory contains setup files, including Vagrant configurations and bootstrap scripts, for Ubuntu-based virtual machines (`24.04` Recommended) that simplify the process of installing NS-3 and its dependencies. Using these pre-configured virtual machines is highly recommended for a streamlined setup, ensuring compatibility and minimizing installation issues.

This code is tested with the following commit from the P4Lang tutorials repository:
- Commit: `3acf7d1e9bb7635c76cc8fc60cd34f9919bb59d4`
- Date: `2024-Dec-01`
