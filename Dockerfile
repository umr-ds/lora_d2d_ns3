FROM ubuntu:20.04

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    gcc \
    g++ \
    mercurial \
    git \
    gsl-bin \
    libgslcblas0 \
    libgsl-dev \
    flex \
    bison \
    libfl-dev \
    tcpdump \
    sqlite \
    sqlite3 \
    libsqlite3-dev \
    cmake \
    libc6-dev \
    libc6-dev-i386 \
    wget \
    vim \
    python3 \
    python3-dev \
    python3-setuptools \
    python3-monotonic \
    python-subprocess32 \
    python3-pip \
    build-essential \
    qt5-default \
    openjdk-17-jdk \
    parallel

RUN pip3 install cxxfilt

WORKDIR /root

ARG NS3_VERSION=3.35

# Fetch and install ns-3
RUN wget http://www.nsnam.org/release/ns-allinone-$NS3_VERSION.tar.bz2 \
    && tar -xf ns-allinone-$NS3_VERSION.tar.bz2

COPY ./lora /root/ns-allinone-$NS3_VERSION/ns-$NS3_VERSION/src/lora/

RUN cd ns-allinone-$NS3_VERSION \
    && ./build.py --disable-netanim --enable-examples --enable-tests --build-option -v \
    && cd ns-$NS3_VERSION \
    && ./waf install

# ns-3 does not use default library paths, so we have to add this one
ENV LD_LIBRARY_PATH=/usr/local/lib

WORKDIR /root/ns-allinone-$NS3_VERSION/ns-$NS3_VERSION/

# Add the experiment stuff
COPY entrypoint.sh /root/entrypoint.sh
ENTRYPOINT ["/root/entrypoint.sh"]
