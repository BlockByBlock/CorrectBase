import serial 
import re
#import pickle
import socket

ser = serial.Serial()
ser.baudrate = 9600
ser.port = 'COM5'
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
		#client.connect (('180.255.10.106', 5005))
		client.connect(('14.100.25.207', 5005))


		#print(read)
	
		us_satid = []
		us_satrss = []
		cn_satid = []
		cn_satrss = []
		satnum = None

		sdata =  read.split(',')
		#print(sdata)

		for i, j in enumerate(sdata):
			if j == '$PUBX':
				if sdata[i+1] == '03':
					satnum = 'n' + sdata[i+2] 
				elif sdata[i+1] == '00':
					# HERE
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

					latlon = ',l' + str(dlat) + ',' + str(dlon)					
			if j == 'U' and len(sdata[i-1]) < 3:
				us_satid.append(',u' + sdata[i-1])
				us_satrss.append(',s' + sdata[i+3])
			if j == 'U' and len(sdata[i-1]) > 2:
				cn_satid.append(',c' + sdata[i-1])
				cn_satrss.append(',s' + sdata[i+3])

		idrss = ''.join(us_satid[:4] + cn_satid[:2] + us_satrss[:4] + cn_satrss[:2])

		if satnum == None:
			continue
		else:
			satinfo = str(satnum + idrss + latlon)
			print(satinfo)

		#dList = [satinfo]	
		#objList = pickle.dumps (satinfo)
		client.send(satinfo)

client.close()

#logging.basicConfig(
#	filename = 'gps.log',
#	filemode = 'w',
#	format = '',
#	level = logging.DEBUG
#	)
