FROM ubuntu:12.04

MAINTAINER Mate Soos version: 0.1

RUN apt-get update
RUN apt-get install -y python-software-properties

RUN add-apt-repository -y ppa:ubuntu-sdk-team/ppa
RUN add-apt-repository -y ppa:george-edison55/gcc4.7-precise
RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test
RUN apt-get update

RUN apt-get install -y libboost-program-options-dev libboost-test-dev gcc-4.7 g++-4.7 gcc g++
RUN apt-get remove -y g++-4.4 gcc-4.4
RUN apt-get install -y cmake git make
RUN apt-get install -y python-pip
RUN pip install lit

RUN groupadd -r cryptoms -g 433
RUN useradd -u 431 -r -g cryptoms -d /home/cryptoms -s /sbin/nologin -c "Docker image user" cryptoms
RUN mkdir /home/cryptoms
RUN chown -R cryptoms:cryptoms /home/cryptoms
USER cryptoms

RUN cd /home/cryptoms && git clone https://github.com/msoos/cryptominisat.git \
&& cd cryptominisat \
&& git submodule init \
&& git submodule update \
&& mkdir build && cd build \
&& cmake .. \
&& make -j4

USER root
RUN cd /home/cryptoms/cryptominisat/build/ && make install && make clean

CMD ["/home/cryptoms/cryptominisat/build/cryptominisat"]