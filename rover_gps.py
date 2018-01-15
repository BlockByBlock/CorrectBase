import serial 
import re
#import pickle
import socket

ser = serial.Serial()
ser.baudrate = 9600
ser.port = 'COM7'
ser.open()

# Debug
#print(ser.is_open)

while(1):
	
	read = ser.readline()
	#print(read)  #DEBUG
	if read == '':
		continue
	elif 'PUBX' in read:
		client = socket.socket ( socket.AF_INET, socket.SOCK_STREAM )
		client.connect (('14.100.25.207', 5006))
		
		#print(read)
		sdata =  read.split(',')
		#print(sdata)

		for i, j in enumerate(sdata):
			if j == '$PUBX':
                                slat = sdata[i+3]
				slon = sdata[i+5]
				slat_deg = float(slat[0:2])
				slat_min = float(slat[2:4]) 
				slat_sec = (float(slat[5:10]) / 100000) * 60
				dlat = slat_deg + (slat_min / 60) + (slat_sec / 3600)

				slon_deg = float(slon[0:3])
				slon_min = float(slon[3:5])
				slon_sec = (float(slon[6:11]) / 100000) * 60
				dlon = slon_deg + (slon_min / 60) + (slon_sec / 3600)

				latlon = str(dlat) + ',' + str(dlon)
				print(latlon)
				client.send(latlon)

client.close()

#logging.basicConfig(
#	filename = 'gps.log',
#	filemode = 'w',
#	format = '',
#	level = logging.DEBUG
#	)
