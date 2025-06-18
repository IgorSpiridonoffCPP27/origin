FROM ubuntu:16.04
COPY . /usr/src/Dockertest
WORKDIR /usr/src/Dockertest
RUN apt update
RUN apt install -y g++ nano
RUN g++ -o Test main.cpp
