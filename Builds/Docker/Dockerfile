# calld

# use the ubuntu base image
FROM ubuntu:16.04
MAINTAINER CALLCHAIN FOUNDATION <landoyjx@163.com>

# make sure the package repository is up to date
RUN apt-get update

# install the dependencies
RUN apt-get -y install git scons ctags pkg-config protobuf-compiler libprotobuf-dev libssl-dev python-software-properties nodejs libboost-all-dev

WORKDIR /root

# move to root directory and strip
RUN cp build/calld calld; strip calld

# copy default config
RUN cp doc/calld-example.cfg calld.cfg

EXPOSE 5050 50338 5020

# launch calld when launching the container
ENTRYPOINT ./calld --net -q
