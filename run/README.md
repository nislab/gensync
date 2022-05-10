# Colosseum

GenSync runs on Colosseum wireless network simulator.

Please see `./run_colosseum.sh -h`.

We suggest starting with the `-u` option and
`scope-gensync.tar.gz` base image that resides in
`file-proxy:/share/nas/sync-edge/images/`. Download the image to
`GENSYNC_ROOT/run/`.

When you modify the GenSync codebase run:

``` shell
$ cd run/
$ ./run_colosseum.sh -u scope-gensync.tar.gz
```

GenSync executables will be produced in
`/GENSYNC_ROOT_BASENAME/build/` in each container that you launch from
the `scope-gensync.tar.gz` image.

To try sample synchronization:
1. Make a reservation with 4 `scope-gensync` SRN's.
2. Run:

``` shell
$ ./run_colosseum.sh sync-edge-??1 sync-edge-??2 sync-edge-??3 sync-edge-??4 1031
```

where `sync-edge-???` are the host names of the containers currently
running on the Colosseum simulator, respectively. The first one will
get set up as a base station, the other 3 as UEs.

Due to the duration of these simulations, we typically run the above
command on a "command center" machine in a `tmux` session.

## Configuration
In a `scope.tar.gz`-based container (including `scope-gensync.tar.gz`)
`/root/radio_api/radio_interactive.conf is:

``` json
{
  "capture-pkts": "False",
  "colosseumcli": "True",
  "iperf": "False",
  "users-bs": "3",
  "write-config-parameters": "True",
  "network-slicing": "True",
  "global-scheduling-policy": "0",
  "slice-scheduling-policy": "[1, 1, 1]",
  "tenant-number": "3",
  "slice-allocation": "{0: [0, 5], 1: [6, 11], 2: [12, 16]}",
  "slice-users": "{0: [3, 6, 9, 14, 17, 20, 25, 28, 31, 36, 39, 42], 1: [4, 7, 10, 15, 18, 21, 26, 29, 32, 37, 40, 43], 2: [2, 5, 8, 11, 13, 16, 19, 22, 24, 27, 30, 33, 35, 38, 41, 44]}",
  "custom-ue-slice": "True",
  "force-dl-modulation": "False",
  "heuristic-params": "{'buffer_thresh_bytes': [1000, 2000], 'thr_thresh_mbps': [0.25, 0.75]}",
  "bs-config": "{'dl_freq': 980000000, 'ul_freq': 1020000000, 'n_prb': 50}",
  "ue-config": "{'dl_freq': 980000000, 'ul_freq': 1020000000, 'force_ul_amplitude': 0.9}"
}
```

Which is the default configuration with only `"iperf": "True"`
changed.
