#!/bin/bash



sudo iw phy phy1 interface add mon7 type monitor

sudo iw dev wlan1 del


sudo ifconfig mon7 up


#set chanel 11

sudo iw dev mon7 set freq 2462


