#!/usr/bin/env bash

this_dir=$(dirname $0)
cd $this_dir/CPISync

if [ $1 ]; then                 # server
    ./Benchmarks -p server_params_data.cpisync -m server
else                            # client
    ./Benchmarks -p client_params_data.cpisync -m client -r 192.168.1.1
fi
