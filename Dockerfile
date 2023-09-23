FROM ubuntu:16.04 as builder

LABEL maintainer="Mate Soos"
LABEL version="5.0"
LABEL Description="An advanced SAT solver"

# get curl, etc
RUN apt-get update && apt-get install --no-install-recommends -y software-properties-common && rm -rf /var/lib/apt/lists/*
RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test && rm -rf /var/lib/apt/lists/*
RUN apt-get update && apt-get install --no-install-recommends -y libboost-program-options-dev gcc g++ make cmake zlib1g-dev wget && rm -rf /var/lib/apt/lists/*

# set up build env
RUN groupadd -r solver -g 433
RUN useradd -u 431 -r -g solver -d /home/solver -s /sbin/nologin -c "Docker image user" solver
RUN mkdir -p /home/solver/cms
RUN chown -R solver:solver /home/solver

# build CMS
USER root
COPY . /home/solver/cms
WORKDIR /home/solver/cms
RUN mkdir build
WORKDIR /home/solver/cms/build
RUN cmake -DSTATICCOMPILE=ON .. \
    && make -j6 \
    && make install \
    && rm -rf *

# set up for running
FROM alpine:latest
COPY --from=builder /usr/local/bin/cryptominisat5 /usr/local/bin/
ENTRYPOINT ["/usr/local/bin/cryptominisat5"]

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

