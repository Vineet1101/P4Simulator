⭐ **Contributions are welcome!** Feel free to submit additional examples to help expand this collection.

The tested results is put in [p4sim-artifact-icns3
](https://github.com/HapCommSys/p4sim-artifact-icns3).

# Examples

The example consists of two parts:

* A simulation script in ns-3, which is used to build the simulation environment.

* A set of files in the `p4src` directory, including the network topology description (`topo.txt`), the P4 program(`*.p4, *.p4i, *.json`), and the corresponding flow table configuration (`flowtable_x.txt`).


## Example List

| Example Name     | Description                                      | Architecture     |
|------------------|--------------------------------------------------|------------------|
| basic_tunnel [1] | Tunnel with custom header                        | V1model          |
| firewall [2]     | A basic stateful firewall                        | V1model          |
| ipv4_forward     | Basic forwarding based on `ip_dst`               | V1model          |
| load_balance [3] | Load balancing in spine-leaf network             | V1model          |
| p4_basic [4]     | Basic forwarding based on `ip_dst`               | V1model          |
| qos              | Forwarding with priority                         | V1model          |
| fat tree         | Large-scale network with fat tree topo           | V1model          |
| simple_pna       | IPv4 forwarding with PNA architecture            | PNA              |
| simple_psa       | IPv4 forwarding with PSA architecture            | PSA              |
| simple_v1model   | IPv4 forwarding with V1model architecture        | V1model          |


## References

- [1] basic_tunnel in [p4lang/tutorials/baisc_tunnel](https://github.com/p4lang/tutorials/tree/master/exercises/basic_tunnel)
- [2] firewall in [p4lang/tutorials/firewall](https://github.com/p4lang/tutorials/tree/master/exercises/firewall)
- [3] load_balance in [p4lang/tutorials/load_balance](https://github.com/p4lang/tutorials/tree/master/exercises/load_balance)
- [4] p4_basic in [p4lang/tutorials/basic](https://github.com/p4lang/tutorials/tree/master/exercises/basic)