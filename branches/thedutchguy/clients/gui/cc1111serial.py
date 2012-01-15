#!/usr/bin/python
import sys, serial

class cc1111serial():

	def __init__(self):
		self.port = "/dev/ttyACM0"
		self.serialport = serial.Serial(self.port,115200)

	def write(self,message):
		self.serialport.write(message)

	def close(self):
		self.serialport.close()
