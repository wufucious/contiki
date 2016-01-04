# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------------#
# Simple application to relay data to Ubidots from a Contiki serial-based conn
# ----------------------------------------------------------------------------#
import serial
from time import sleep
from ubidots import ApiClient

# Use as default
PORT = "/dev/ttyUSB0"

# ----------------------------------------------------------------------------#
# Create a serial object and connect to mote over USB
# ----------------------------------------------------------------------------#
def connectMote(port):
	try:
		ser = serial.Serial(port, 115200,timeout=0, parity=serial.PARITY_NONE,
							stopbits=serial.STOPBITS_ONE, bytesize=serial.EIGHTBITS)
	except:
		sys.exit("Error connecting to the USB device, aborting")
	return ser

# ----------------------------------------------------------------------------#
# Parse the serial data and publish to Ubidots, this assumes the following:
# \r\n API_KEY \t VAR_KEY \t VALUE \r\n
# ----------------------------------------------------------------------------#
def process_data(raw):
	# Search for start and end of frame and slice, discard incomplete
	if "\r\n" in raw:
		raw = raw[(raw.index("\r\n") + 2):]
		if "\r\n" in raw:
			raw = raw[:raw.index("\r\n")]
			# We should have a full frame, parse based on tab and create a list
			ubidots_raw = raw.split("\t")
			# Create a Ubidots client and get a pointer to the variable
			client = ApiClient(ubidots_raw[0])
			try:
				my_variable = client.get_variable(ubidots_raw[1])
			except Exception, e:
				print "Ubidots error: %s" % e
				return
			# Update the variable
			my_variable.save_value({'value':int(ubidots_raw[2])})
# ----------------------------------------------------------------------------#
# MAIN APP
# ----------------------------------------------------------------------------#
if __name__=='__main__':

	# Create the serial object and connect to the mode
	# Do not check the serial port object as the function already does it
	s = connectMote(PORT)
	
	# Loop forever and wait for serial data
	while True:
		queue = s.inWaiting()
		if queue > 0:
			data = s.read(1000)
			process_data(data)
		sleep(0.2) 	