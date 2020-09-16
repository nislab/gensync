# Repeatable Synchronization
When `CPISync` is used as a library in other projects, we are normally
interested in measuring its efficiency. If we need to isolate the
behavior of `CPISync` itself from the program it is included in, we
need to record the data sets and the synchronization algorithm
parameters that are used in each `CPISync` invocation.

We allow for recording the data sets and the synchronization algorithm
parameters by building `CPISync` with a `RECORD` compile time option:

``` shell
$ cmake -DRECORD=cpisync .
```

When this option is enabled, the following files are produced in the
directory provided in `RECORD`:
- `..._params_data.cpisync` the data sets and the synchronization algorithm
  parameters as seen on the server side,
- `..._params_data.cpisync` the same as seen on the client side,
- `..._observ.cpisync` the synchronization algorithm parameters and
  the synchronization performance metrics as seen on the server side,
- `..._observ.cpisync` the same as seen on the client.

Each file name is prepended with a unique string.

If the server and the client `GenSync`s are run on the separate
machines, each machine will contain the corresponding files.

## Using produced files
The files that are produced in the directory set in `RECORD` are
everything you need to reproduce the behavior of the synchronization
algorithm on the data sets from the application that uses `CPISync` as
a library.

`Benchmarks` program (`../Benchmarks/Runner.cpp`) is a program that
you can use to rerun the synchronization that is recorded. This
program accepts one parameter, the path to a `..._params_data.cpisync`
or `..._params.cpisync` file. `Benchmarks` reconstruct both the
recorded data sets and the synchronization parameters from the
provided file and runs the synchronization again.

Finally, to tweak the synchronization algorithm for the recorded data
sets, you can edit the file that you pass to `Benchmarks`. Each
`..._params_data.cpisync` and `..._params.cpisync` file starts with
the synchronization algorithm specification. Edited file can then be
passed through `Benchmarks` again to observe the new synchronization
performance metrics in the new `..._observ.cpisync` files.

## Saving disk space
`..._params_data.cpisync` files are proportional in size with the data
sets you synchronize. Thus, `CPISync` tries its best to keep only one
copy of `...params_data.cpisync` in your `RECORD` directory. If your
`RECORD` already contains a file with the name ending in
`_params_data.cpisync`, `CPISync` will not make a new one. Instead, a
`..._params.cpisync` file will be created. The difference between
`..._parms.cpisync` and `...params_data.cpisync` files is only that
the former contain a reference while the latter contain the full copy
of the data you synchronize.

For example, a `...params_data.cpisyn` looks like this:

``` shell
Sync protocol (as in GenSync.h): 1
m_bar: 510
bits: 64
epsilon: 8
partitions/pFactor(for InterCPISync): 0
hashes: true
--------------------------------------------------------------------------------
^>Xv`aqmzr]\aSZaa^upyXL=
sZzKwpSpgGCW|xrSqgIXJuJ=
}Je`IRf@qXdqeHbSXJGikCL=
...
--------------------------------------------------------------------------------
VfBpRSWdKm`Y>k\smRb}vjH=
^PSA_xckkXQGdznQnGa}qcD=
sZzKwpSpgGCW|xrSqgIXJuJ=
...

```

While a `..._params.cpisync` looks like this:

``` shell
Sync protocol (as in GenSync.h): 1
m_bar: 510
bits: 32
epsilon: 12
partitions/pFactor(for InterCPISync): 0
hashes: true
--------------------------------------------------------------------------------
Reference: CommSocket_1600209310687907210_params_data.cpisync
```

The reference file is looked for in the same directory where the file
that contains the reference resides.

It is important to note that if you run both the `CPISync` client and
the server on the same machine, it may happen that you get multiple
`..._params_data.cpisync` and no `..._params.cpisync` files. This
happens because both the client and the server processes use the same
`RECORD` directory. One of the processes can be just that much slower
than the other to find itself inspecting the `RECORD` directory before
the faster process has done writing its `..._params_data.cpisync`. For
that reason, the slower process will also produce a
`..._params_data.cpisync` file instead of the disk space-optimized
`..._params.cpisync`.

We leave this as it is because we prefer **not** to slowdown
synchronization itself in order to optimize record files IO.
