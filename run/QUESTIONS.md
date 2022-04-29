# Colosseum & SCOPE

Table of Contents:
- [Public Questions](#public_questions)
- [Internal Questions](#questions)
- [Bugs](#bugs)
- [Public Conversations](#conversations)

<a name="public_questions"></a>
## Public Questions
General questions:
- Does RedHat have engineers dedicated to Colosseum development? If
  so, will they be able to walk us through the technical aspects of
  the Colosseum system?
- Can two SRNs on different base stations exchange TCP traffic (this
  is the question of routing in the core network)?
- What is a Colosseum scenario? What channel aspects does it capture?

Specific questions:
- If we want to emulate a scenario with "Number of Nodes: 50" and
  "Number of Base Station Nodes: 10" in a reservation with only 4
  nodes and 1 base station, what difference will it make with respect
  to the emulated channel conditions? See the scenario specifications
  [here](https://colosseumneu.freshdesk.com/support/solutions/articles/61000295793).
- Is it advised to change radio scenarios during the same reservation?
  If so, are there any general recommendations with regards to how to
  check whether the scenario loading has completed?
- Scenarios typically last for about 10 seconds and there is an option
  to "cycle" them. What does the "cycling" mean in terms of emulated
  channel conditions?
- In some reservations, some SRNs fail to connect to the radio
  hardware due to "Detected Radio-Link Failure" in srsLTE software. Is
  it possible to implement hardware health checks prior to SRN
  assignments such that a reservation always receives all functional
  SRNs (pairs of server/container + radio hardware).
- Is it expected that some Colosseum scenarios are not available to
  certain users? For example the scenario 1028 is listen on the
  [website](https://colosseumneu.freshdesk.com/support/solutions/articles/61000295793)
  but is actually not available to us via `colosseumcli rf scenario
  list`.

<a name="questions"></a>
## Internal Questions
- Can we get some reasonable default SCOPE configurations from the
  team at Northeastern to run scenarios with?
  - [April 26th, 2022]: Yes, they have updated `/share/nas/common/scope.tar.gz`.
- Once we get SCOPE working, will we be able to ping two UEs on
  different BSs?
  - Not sure yet. That would require some routing in the network core
    (moved to public).
- Can SCOPE run scenarios on its own or should we run them separately?
  - No, it has to be done via `colosseumcli`.
- Can scenarios take more than 600 seconds (as on SCOPEs [GitHub
  repo](https://github.com/wineslab/colosseum-scope#scope-cellular-scenarios-for-colosseum-network-emulator))?
  - Yes, they can be cycled using `-c` option.
- Do certain scenarios assume certain numbers of nodes in the
  reservation?
  - No, we can run any number of nodes (in accordance with the SCOPE
    config file we use).

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

### Bug 2: Some USRPs just don't work
This, for example, happens whenever we get `sync-edge-036` in a reservation.

As a workaround, we need to make a reservation with more than 2 UEs
and then pick two of them that can connect to USRP successfully.

Once you get `sync-edge-036` (or other nonfunctional SRN), one may
make another reservation (while the old one is still active) to rule
out the possibility of getting the same nonfunctional SRN.

### Bug 3: UID/GUID mapping issues in containers
All containers based on Ubuntu 16.04 have UID/GUID mapping issues
(including `/share/nas/common/scope.tar.gz`).

These are unprivileged containers but do not set up UID/GUID mapping
properly. For example:

``` shell
srn-user@sync-edge-scope-gensync-srn34:~$ ls -ld /home/srn-user/
drwxr-xr-x 2 nobody nogroup 5 Feb  8  2018 /home/srn-user/
```

Thus, nobody can write, read, or execute from the `/home/srn-user/`
directory, while at the same time only `srn-user` can read from the
shared storage (and container `root` does not).

I figured out that files owned by `sync-edge-admin` on `file-proxy`
(shared NAS server) map correctly to `srn-user` in the
containers. Thus, if my containers want to use files from the shared
NAS, the files need be owned by `sync-edge-admin`.

#### Workaround
To get around these privileges, one can:
- make a reservation with single container,
- copy files to `/share` within that container. For example:

``` shell
rsync -Pav DATA srn-user@sync-edge-034:/share/DATA
```
This works because `srn-user` in the container maps to `sync-edge-admin` on `file-proxy`.

<a name="conversations"></a>
## Public Conversations
- Novak's Google Groups [question](https://groups.google.com/g/colosseum-users/c/KauiPwqSWM0).
