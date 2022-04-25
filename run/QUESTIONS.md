# Colosseum & SCOPE

Table of Contents:
- [Questions](#questions)
- [Bugs](#bugs)

<a name="questions"></a>
## Questions
- Can we get some reasonable default SCOPE configurations from the
  team at Northeastern to run scenarios with?
- Once we get SCOPE working, will we be able to ping two UEs on
  different BSs?
- Can SCOPE run scenarios on its own or should we run them separately?
- Can scenarios take more than 600 seconds (as on SCOPEs [GitHub
  repo](https://github.com/wineslab/colosseum-scope#scope-cellular-scenarios-for-colosseum-network-emulator))?
- Do certain scenarios assume certain numbers of nodes in the
  reservation?

<a name="bugs"></a>
## Bugs

### Bug 1: Basic SCOPE not operational

#### Steps to reproduce
1. Make a reservation of 4 `scope` containers (or use sync-edge's
   `scope-pass` which is the same container with password `123`).
2. In the first run:

``` shell
cd /root && ./radio_code/scope_config/remove_experiment_data.sh
cd /root/radio_api/ && python3 scope_start.py --config-file radio_interactive.conf
```
3. Wait until the following message:

``` shell
...
Setting LTE transceiver state to ACTIVE
```

4. In the other three run (the same command):

``` shell
cd /root && ./radio_code/scope_config/remove_experiment_data.sh
cd /root/radio_api/ && python3 scope_start.py --config-file radio_interactive.conf
```
5. On each of these 4 containers run:

``` shell
tmux a -t scope
```
- on the first node (the base station), you will see the `srsepc`
  output on the left and `srsenb` output on the right.
- on the other three nodes you will see the `srsue` outputs on the
  left and the `iperf3 -c ...` outputs on the left.

#### Expected behavior
We expect that `srsepc`, `srsenb`, and `srsue` do their task and
output the IP addresses for `iperf3` to use. These same IP addresses
would be enough for us to run GenSync through them.

##### Recent update
On April 25th at 3PM, this works. We indeed obtain IP addresses for
each UE and the base station (IP's are `172.16.0.3` , `172.16.0.4`,
`172.16.0.5` for UEs and `172.16.0.1` for the base station). `iperf`s
to base station and `ping`s from UE to UE work.
