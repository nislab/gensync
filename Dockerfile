FROM debian:bullseye

MAINTAINER Novak Boskov <boskov@bu.edu>

RUN apt-get update -y && apt-get install -y build-essential cmake libntl-dev libgmp3-dev libcppunit-dev

COPY . /gensync

RUN cd gensync; rm -rf build/ || true; cmake -B build; cmake --build build -j $(nproc)
