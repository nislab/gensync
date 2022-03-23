# Colosseum

GenSync runs on Colosseum wireless network simulator.

Please see `./run_colosseum.sh -h`.

We suggest starting with the `-u` option and
`gensync-colosseum.tar.gz` base image that resides in
`file-proxy:/share/nas/sync-edge/images/`. Download the image to
`GENSYNC_ROOT/run/`.

When you modify the GenSync codebase run:

``` shell
$ cd run/
$ ./run_colosseum.sh -u gensync-colosseum.tar.gz
```

GenSync executables will be produced in
`/GENSYNC_ROOT_BASENAME/build/` in each container that you launch from
the `gensync-colosseum.tar.gz` image.

To try sample synchronization run:

``` shell
$ ./run_colosseum.sh sync-edge-??1 sync-edge-??2
```

where `sync-edge-??1` and `sync-edge-??2` are the server and client
containers currently running on the Colosseum simulator, respectively.
