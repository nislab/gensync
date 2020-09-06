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
- `..._params.cpisync` the data sets and the synchronization algorithm
  parameters as seen on the server side,
- `..._params.cpisync` the same as seen on the client side,
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
program accepts one parameter, the path to a `..._params.cpisync`
file. `Benchmarks` reconstruct both the recorded data sets and the
synchronization parameters from the provided file and runs the
synchronization again.

Finally, to tweak the synchronization algorithm for the recorded data
sets, you can edit the file that you pass to `Benchmarks`. Each
`..._params.cpisync` file starts with the synchronization algorithm
specification. Edited file can then be passed through `Benchmarks`
again to observe the new synchronization performance metrics in new
`..._observ.cpisync` files.
