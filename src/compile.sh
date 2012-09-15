#!/bin/bash

g++ -std=gnu++0x -I/home/faramir/opt/shared/include/ -L/home/faramir/opt/shared/lib/ -lucw -lcharset -lpthread -o extractor extractor.cc
