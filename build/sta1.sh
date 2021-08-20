#!/bin/bash

while true
do
    top -u root -n 1 | grep sid | awk '{printf "%s,--%s,%s\n",$1,$6,$9}' >> sta.txt
    sleep 5s
done