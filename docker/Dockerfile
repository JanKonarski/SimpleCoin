FROM ubuntu:latest

ARG VERSION="CRYPTOPP_8_9_0"
ARG CORES="6"

RUN apt update && apt upgrade -y
RUN apt install -y g++ cmake gdb git
RUN apt install -y libboost-all-dev #libssl-dev #libcrypto++-dev

WORKDIR /code
RUN git clone https://github.com/abdes/cryptopp-cmake.git \
    && cd cryptopp-cmake \
    && git checkout $VERSION
WORKDIR /code/cryptopp-cmake/build
RUN cmake .. \
    && make -j $CORES \
    && make install
WORKDIR /
RUN rm -r code
