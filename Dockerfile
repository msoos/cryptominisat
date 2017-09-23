FROM ubuntu:16.04

LABEL maintainer="Mate Soos"
LABEL version="5.0"
LABEL Description="An advanced SAT solver"

# get curl, etc
RUN apt-get update && apt-get install --no-install-recommends -y software-properties-common && rm -rf /var/lib/apt/lists/*
RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test && rm -rf /var/lib/apt/lists/*
RUN apt-get update && apt-get install --no-install-recommends -y libboost-program-options-dev gcc g++ make cmake wget python zlib1g-dev && rm -rf /var/lib/apt/lists/*

# get M4RI
RUN wget https://bitbucket.org/malb/m4ri/downloads/m4ri-20140914.tar.gz \
    && tar -xvf m4ri-20140914.tar.gz
WORKDIR m4ri-20140914
RUN ./configure \
    && make \
    && make install \
    && make clean

# set up build env
RUN groupadd -r cryptoms -g 433
RUN useradd -u 431 -r -g cryptoms -d /home/cryptoms -s /sbin/nologin -c "Docker image user" cryptoms
RUN mkdir /home/cryptoms
RUN chown -R cryptoms:cryptoms /home/cryptoms

# build CMS
USER cryptoms
COPY . /home/cryptoms/
WORKDIR /home/cryptoms/
RUN mkdir build
WORKDIR /home/cryptoms/build
RUN cmake .. \
    && make -j6 \
    && rm -rf CMakeFiles cmsat5-src

# set up for running
WORKDIR /home/cryptoms/build/
ENTRYPOINT ["/home/cryptoms/build/cryptominisat5"]

# --------------------
# HOW TO USE
# --------------------
# on file through STDIN:
#    zcat mizh-md5-47-3.cnf.gz | docker run --rm -i -a stdin -a stdout cms

# on a file:
#    docker run --rm -v `pwd`/myfile.cnf.gz:/in cms in

# echo through STDIN:
#    echo "1 2 0" | docker run --rm -i -a stdin -a stdout cms

# hand-written CNF:
#    docker run --rm -ti -a stdin -a stdout cms

