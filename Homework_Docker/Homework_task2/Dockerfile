FROM ubuntu:20.04
COPY . /usr/src/Dockertest2
WORKDIR /usr/src/Dockertest2
RUN apt update
RUN apt install -y g++
RUN g++ -std=c++17 -pthread -o Test2 Homework_Task1.cpp
