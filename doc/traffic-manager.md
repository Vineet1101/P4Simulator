# Traffic Manager: VOQ + fabric + strict-priority egress

This document describes the high-fidelity switch Traffic Manager (TM) added to
the P4sim V1model core, the switch/channel changes that support it, the
configuration surface, and the examples and tests used to validate it.

The TM replaces the old output-only priority scheduler (`NSQueueingLogicPriRL`)
with an input-buffered **Virtual Output Queue (VOQ)** stage, a **fabric
scheduler** that matches inputs to outputs, and a per-output **strict-priority
egress** stage that serialises frames onto the wire at line rate.

The whole path is **opt-in and additive**: with `EnableVoqFabric=false` (the
default) the switch behaves exactly as before. Nothing on the legacy path was
removed.

---

## 1. Concepts

Three orthogonal ideas are easy to conflate, so we separate them explicitly:

| Concept | What it controls | Where it lives in the TM |
| --- | --- | --- |
| **VOQ** | *Queue organisation / buffering* — which queue a packet enters and where it waits on the **input** side, indexed per `[inPort][outPort][priority]`. Avoids head-of-line blocking. | `EnqueueToVoq()` + `m_voq` |
| **Fabric scheduling** | *Matching* — when several VOQs want to move, which input is granted to which output this round. | `RunFabricScheduler()` (priority-first maximal matching) |
| **Strict Priority (SP)** | *Scheduling on the output* — when several egress queues of a port hold packets, which priority is served first. | `SelectEgressPriority()` |
| **FIFO** | *Ordering within one queue* — packets leave a single queue in arrival order. | `std::deque` per `[out][prio]` |

The egress side is therefore **strict priority across the 8 priority queues of a
port, FIFO within each queue** — i.e. per output port it behaves like a `pfifo`
(strict-priority + FIFO) scheduler. Priority is a 3-bit field, so there are 8
levels; **higher value = higher priority** (7 is highest).

---

## 2. Datapath

```
   ingress pipeline
        │  (outPort, priority chosen by the P4 program)
        ▼
   EnqueueToVoq  ──►  VOQ[inPort][outPort][priority]     (input-buffered)
                            │
                            ▼
                     fabric scheduler                    (priority-first
                       grants in→out                      maximal matching)
                            │
                            ▼
                     egress[outPort][priority]           (per-port SP + FIFO)
                            │
                            ▼
                     egress pipeline + deparse
                            │
                            ▼
                     TransmitCallback  ──►  ns-3 NetDevice ──► wire
                            ▲                                    │
                            └──── NotifyEgressTxComplete ◄───────┘
                                  (frame finished serialising)
```

### Fabric policy

The default matching is **priority-first maximal matching**: for priority 7 down
to 0, for each still-unused input port, grant the first unused output port that
has a non-empty VOQ at that priority. One input grants at most one packet and one
output receives at most one packet per round. The policy is a single overridable
method (`DoRunFabricScheduler()`) so iSLIP / round-robin can be dropped in
without touching the rest of the TM.

### Event-driven vs manual

- **Manual (default object mode):** call `RunFabricScheduler()` /
  `DequeueFromVoq()` yourself; nothing touches the ns-3 event queue. Used by the
  low-level unit tests.
- **Event-driven (`EventDriven=true`):** `EnqueueToVoq()` arms a self-clocking
  fabric+egress loop via `Simulator::Schedule` (no threads) that moves packets
  VOQ → egress → wire, honouring the fabric/port rates and the
  arbitration/pipeline delays. This is the mode the V1model core uses.

### Completion-driven egress and the channel signal

By default the egress scheduler self-clocks each port's serialisation at
`PortRate`. With `EgressCompletionDriven=true` (the mode the core uses) the flow
is decoupled instead:

1. `EgressServiceEvent()` hands the frame to the `TransmitCallback` **first**,
   then waits — it does **not** immediately count the frame or re-arm the port.
2. The datapath (the switched-Ethernet channel / PHY) decides when the frame has
   finished serialising onto the wire and calls
   `NotifyEgressTxComplete(outPort, success)`.
3. Only on that signal does the TM count the frame as transmitted
   (`totalTransmitted`, `perPriorityTransmitted`, `perPortTxBytes`), clear the
   in-flight slot, and serve the next frame.

This separates *"which packet goes next"* (the TM's decision) from *"when the
wire is free"* (the PHY's), and guarantees `totalTransmitted` counts a frame only
**after** it is actually on the wire — never before.

The accounting is split accordingly: **queue-residence** work (buffer release,
egress/total delay, egress-dequeue trace) happens at dequeue; **on-wire**
counters happen on completion.

### Switched-Ethernet channel: propagation no longer blocks the next frame

`SwitchedEthernetChannel` models a full-duplex link with independent per-source
state. The sender is freed after **serialisation** (`txTime`), not after
**propagation** (`txTime + delay`): once a frame's last bit has left the port the
port returns to `IDLE` and may start the next frame, while the in-flight frame
keeps propagating toward the receiver. `GetState()` still reports `PROPAGATING`
for observers, and `IsBusy()` (which gates `TransmitStart`) is true only while a
frame is actively serialising. This keeps back-to-back frames flowing at line
rate instead of paying the propagation delay per frame.

---

## 3. Enabling the TM

Set one attribute on the P4 switch device before the simulation starts:

```cpp
P4Helper p4;
p4.SetDeviceAttribute("JsonPath",      StringValue(jsonPath));
p4.SetDeviceAttribute("FlowTablePath", StringValue(flowTablePath));
p4.SetDeviceAttribute("P4SwitchArch",  UintegerValue(0));   // V1model
p4.SetDeviceAttribute("EnableVoqFabric", BooleanValue(true));  // ← opt in
```

The core then creates the TM at start-up, sizes it to the number of attached
ports, seeds `PortRate` from the egress channel's `DataRate`, wires the
`TransmitCallback` to the egress send path, and enables `EventDriven` +
`EgressCompletionDriven`. The TM is disposed before the core is destroyed.

---

## 4. Configuration surface

All knobs are ns-3 attributes on `ns3::P4TrafficManager` (buffer limits are in
**bytes**; `0` means unlimited):

| Attribute | Type | Default | Meaning |
| --- | --- | --- | --- |
| `NumPorts` | uint32 | 0 | Number of ports N; allocates N·N·8 VOQs. Set by the core. |
| `GlobalBufferLimit` | uint64 | 0 | Total bytes across VOQ + egress. |
| `InputBufferLimit` | uint64 | 0 | Per-input-port bytes. |
| `VoqLimit` | uint64 | 0 | Per-VOQ `[in][out][prio]` bytes. |
| `EgressPortLimit` | uint64 | 0 | Per-output-port egress bytes. |
| `EgressQueueLimit` | uint64 | 0 | Per-egress-queue `[out][prio]` bytes. |
| `PortRate` | DataRate | 1Gbps | Output-port serialisation rate. |
| `FabricRate` | DataRate | 10Gbps | Fabric transfer rate. |
| `IngressPipelineDelay` | Time | 0 | Fixed ingress processing delay (applied before the fabric round). |
| `FabricArbitrationDelay` | Time | 0 | Fixed fabric arbitration delay per round. |
| `EgressPipelineDelay` | Time | 0 | Fixed egress processing delay. |
| `EventDriven` | bool | false | Self-clock the fabric+egress loop via ns-3 events. |
| `EgressCompletionDriven` | bool | false | Wait for `NotifyEgressTxComplete()` before counting a frame transmitted (requires `EventDriven`). |

Switch-level knob (on `ns3::P4SwitchNetDevice`):

| Attribute | Type | Default | Meaning |
| --- | --- | --- | --- |
| `EnableVoqFabric` | bool | false | Route egress through the VOQ+fabric TM instead of the legacy output-queued path. |

---

## 5. Statistics, traces, and drop reasons

`GetStats()` returns a cumulative `TmStats`:

| Counter | Meaning |
| --- | --- |
| `totalReceived` | packets offered to `EnqueueToVoq` |
| `totalVoqEnqueued` | accepted into a VOQ |
| `totalMovedToEgress` | dequeued from VOQ (granted by the fabric) |
| `totalEgressEnqueued` | accepted into an egress queue |
| `totalTransmitted` | serialised onto the wire (counted **after** transmit) |
| `totalDropped` | dropped for any reason |
| `dropsByReason[5]` | per-`TmDropReason` breakdown |
| `perPriorityTransmitted[8]` | transmitted per priority level |
| `perPortTxBytes[]` | bytes transmitted per output port |
| `AvgVoqDelay/AvgEgressDelay/AvgTotalDelay`, `maxQueueingDelay` | delay accumulators |

Trace sources: `VoqEnqueue`, `VoqDequeue`, `EgressEnqueue`, `EgressDequeue`,
`Drop`, `VoqWaitingDelay`, `EgressWaitingDelay`, `TotalDelay`.

Drop reasons (`TmDropReason`), checked in admission order:

1. `VOQ_GLOBAL_BUFFER_FULL` — global buffer would overflow
2. `VOQ_INPUT_BUFFER_FULL` — per-input-port buffer would overflow
3. `VOQ_QUEUE_FULL` — the target `VOQ[in][out][prio]` would overflow
4. `EGRESS_PORT_BUFFER_FULL` — per-output-port egress buffer would overflow
5. `EGRESS_QUEUE_FULL` — the target egress queue would overflow

---

## 6. Validation

Three levels of validation were run: a unit test suite (TM logic in isolation),
an end-to-end integration example (real P4 program through the switch), and a
throughput benchmark (goodput vs line rate). All results below are from the
current branch.

### 6.1 Unit test suite — `test/p4-traffic-manager-test-suite.cc`

9 QUICK cases covering enqueue/dequeue, priority scheduling, VOQ isolation for a
shared output, fabric matching, finite-buffer drops, delay measurement,
event-driven drain, egress strict priority, and egress drop:

```
$ ./test.py -s p4-traffic-manager
[1/1] PASS: TestSuite p4-traffic-manager
1 of 1 tests passed (1 passed, 0 skipped, 0 failed, 0 crashed, 0 valgrind errors)
```

Cases: `TmBasicEnqueueDequeueTest`, `TmPrioritySchedulingTest`,
`TmVoqSameOutputTest`, `TmFabricMatchingTest`, `TmBufferDropTest`,
`TmDelayMeasurementTest`, `TmEventDrivenDrainTest`, `TmEgressStrictPriorityTest`,
`TmEgressDropTest`.

### 6.2 Integration example — `examples/p4-voq-fabric-integration.cc`

Two hosts and one V1model switch running the `simple_v1model` IPv4-forwarding
program. The example is self-validating (non-zero exit on failure) and runs both
datapaths so the additive contract is checked directly.

```
$ ./ns3 run "p4-voq-fabric-integration --run=voq"
  rxBytes=296000  tmPresent=1  tmReceived=298  tmVoqEnqueued=298  tmTransmitted=298  tmDropped=0
  [PASS] V1model core exists
  [PASS] Traffic Manager created when EnableVoqFabric=true
  [PASS] Sink received data over the VOQ datapath
  [PASS] Packets entered a VOQ
  [PASS] TM serialised packets onto the wire
  [PASS] Transmitted count does not exceed VOQ-enqueued count
  [PASS] VOQ-enqueued count does not exceed offered count
=== ALL CHECKS PASSED (0 failure(s)) ===

$ ./ns3 run "p4-voq-fabric-integration --run=legacy"
  rxBytes=296000  tmPresent=0
  [PASS] V1model core exists
  [PASS] No Traffic Manager created when disabled (additive contract)
  [PASS] Sink received data over the legacy datapath
=== ALL CHECKS PASSED (0 failure(s)) ===
```

Both datapaths deliver the same 296 000 bytes; the VOQ path additionally shows
`received == voqEnqueued == transmitted` with zero drops (nothing lost inside the
TM), and the legacy path confirms no TM is created when the feature is off.

### 6.3 Throughput benchmark — `examples/p4-voq-fabric-throughput.cc`

A single saturating UDP flow (offered at 1.2× the egress line rate) is pushed
host0 → switch → host1. The topology uses a fast ingress link (10 Gbps) so the
sender NIC is never the limiter, and the switch egress port is the sole
bottleneck. Goodput at the sink is compared against the link's line rate; the
header-overhead ceiling is `payload / (payload + 14 + 20 + 8)` ≈ 97.09 % for a
1400-byte payload.

```
$ ./ns3 run "p4-voq-fabric-throughput --linkRate=100Mbps"
  [TM] received=2059 voqEnq=2059 transmitted=2059 dropped=0
  goodput=97.13 Mbps of 100.00 Mbps line (97.13% of line; header-overhead ceiling ~97.09%)
  [PASS] goodput >= 80.00% of line rate

$ ./ns3 run "p4-voq-fabric-throughput --linkRate=1000Mbps"
  [TM] received=20574 voqEnq=20574 transmitted=20574 dropped=0
  goodput=970.92 Mbps of 1000.00 Mbps line (97.09% of line; header-overhead ceiling ~97.09%)
  [PASS] goodput >= 80.00% of line rate
```

| Egress line rate | Goodput | % of line | Header-overhead ceiling | Drops |
| --- | --- | --- | --- | --- |
| 100 Mbps | 97.13 Mbps | 97.13 % | ~97.09 % | 0 |
| 1000 Mbps | 970.92 Mbps | 97.09 % | ~97.09 % | 0 |

The delivered goodput sits right at the header-overhead ceiling at both rates,
confirming the completion-driven egress serialises at true line rate with no
artificial timer bottleneck and no internal loss.

---

## 7. How to run

From the ns-3 root (with this module in `contrib/p4sim`):

```bash
# Unit tests
./test.py -s p4-traffic-manager

# End-to-end integration check (both datapaths)
./ns3 run "p4-voq-fabric-integration --run=voq"
./ns3 run "p4-voq-fabric-integration --run=legacy"

# Throughput benchmark (one link rate per invocation — bmv2 cannot be
# re-initialised within a single process)
./ns3 run "p4-voq-fabric-throughput --linkRate=100Mbps"
./ns3 run "p4-voq-fabric-throughput --linkRate=1000Mbps"
```

---

## 8. Source map

| File | Role |
| --- | --- |
| `utils/p4-traffic-manager.{h,cc}` | VOQ, fabric scheduler, egress SP, stats/traces, completion signal |
| `model/p4-core-v1model.{h,cc}` | Creates/wires/disposes the TM; opt-in egress branch; transmit + completion glue |
| `model/p4-switch-net-device.{h,cc}` | `EnableVoqFabric` attribute; propagates it to the core |
| `model/switched-ethernet-channel.{h,cc}` | Full-duplex link; sender freed after serialisation, not propagation |
| `test/p4-traffic-manager-test-suite.cc` | 9-case unit suite |
| `examples/p4-voq-fabric-integration.cc` | End-to-end additive-contract check |
| `examples/p4-voq-fabric-throughput.cc` | Near-line-rate goodput benchmark |
