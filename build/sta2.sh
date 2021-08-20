#!/bin/bash
###
 # @Author: Cai Deng
 # @Date: 2021-03-12 14:00:38
 # @LastEditors: Cai Deng
 # @LastEditTime: 2021-03-12 14:01:58
 # @Description: 
### 

while true
do
    sleep 2s
    top -u root -n 1 | grep sid | awk '{printf "%s,--%s,%s\n",$1,$6,$9}' >> sta2.txt
    sleep 8s
done