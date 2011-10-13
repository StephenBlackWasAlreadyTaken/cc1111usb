#!/usr/bin/python
import sys, serial

port = "ACM0"
if len(sys.argv) > 1:
    port = sys.argv.pop()

dport = "/dev/tty" + port

s=serial.Serial(dport)

while True:
    sys.stdout.write(s.read(1))
    sys.stdout.flush()
