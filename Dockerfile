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

# Use an official miniconda as a parent image
FROM continuumio/miniconda

ENV LANG=C.UTF-8 LC_ALL=C.UTF-8

# Install the dependencies to build wordseg
RUN apt-get update && \
  apt-get install -y build-essential git bsdmainutils \
  cmake libboost-program-options-dev

RUN conda install python=3.8 pytest joblib scikit-learn

# Clone wordseg from github, install and test it
RUN git clone https://github.com/bootphon/wordseg.git && \
  cd wordseg && \
  mkdir -p build && \
  cd build && \
  cmake .. && \
  make && \
  make install

RUN cd /wordseg && pytest -v test
