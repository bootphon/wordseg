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

# Set the working directory to /wordseg
WORKDIR /wordseg

# Install the dependencies to build wordseg
RUN apt-get update && \
    apt-get install -y build-essential git python3 python3-pip \
       cmake libboost-program-options-dev

# Provide the 'python' command (only because on that image we don't
# have python2 installed)
RUN ln -s /usr/bin/python3 /usr/bin/python

# Clone wordseg from github
RUN git clone https://github.com/bootphon/wordseg.git .

# Install wordseg
RUN mkdir -p build && cd build && cmake .. && make && make install

# Test the installation
RUN pytest -v test/
