#!/usr/bin/python

import sys
import datetime
import time
time.sleep(60)
current = str(datetime.datetime.now())
file = open("on_off.log","a")
file.write("On @ "current)
