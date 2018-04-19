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
FROM ubuntu:16.04

ENV LANG=C.UTF-8 LC_ALL=C.UTF-8

# Set the working directory to /wordseg
WORKDIR /wordseg

# Install the dependencies to build wordseg
RUN apt-get update && \
    apt-get install -y build-essential git \
       cmake libboost-program-options-dev wget

# Install Python from Anaconda distribution
RUN wget https://repo.continuum.io/miniconda/Miniconda3-latest-Linux-x86_64.sh -O ~/miniconda.sh && \
    bash ~/miniconda.sh -b -p /miniconda && \
    rm ~/miniconda.sh

ENV PATH /miniconda/bin:$PATH

RUN conda create --name wordseg python=3.6 pytest joblib && \
    /bin/bash -c "source activate wordseg"

# Clone wordseg from github
RUN git clone https://github.com/bootphon/wordseg.git .

# Install wordseg
RUN mkdir -p build && cd build && cmake .. && make && make install

# Test the installation
RUN pytest -v test/
