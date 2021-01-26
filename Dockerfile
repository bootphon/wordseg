# Use this file to build a docker image of wordseg:
#
#    sudo docker build -t wordseg
#
# Then open a bash session in docker with:
#
#    sudo docker run -it wordseg /bin/bash
#
# You can then use wordseg within docker. See the docker doc for
# advanced usage.

# Use an official Ubuntu as a parent image
FROM ubuntu:20.04

ENV TZ=America/New_York \
    DEBIAN_FRONTEND=noninteractive \
    LANG=C.UTF-8 \
    LC_ALL=C.UTF-8 \
    NCORES=4

# Install the dependencies to build wordseg
RUN apt-get update && apt-get install --no-install-recommends -y -qq \
  bsdmainutils \
  cmake \
  g++ \
  git \
  libboost-program-options-dev \
  make \
  python3 \
  python3-pip && \
  rm -rf /var/lib/apt/lists/*

# tests expect python to be available as executable 'python' not 'python3'
RUN ln -s /usr/bin/python3 /usr/bin/python && \
  pip3 install pytest pytest-cov

# Copy wordseg project to container
COPY . /wordseg

# Install wordseg and test it
RUN make clean && make install && make test
