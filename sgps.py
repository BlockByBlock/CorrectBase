import serial 
import re
import pickle
import socket

ser = serial.Serial()
ser.baudrate = 9600
ser.port = 'COM5'
ser.open()

# Debug
print(ser.is_open)

while(1):
	
	read = ser.readline()
	#print(read)  #DEBUG
	if read == '':
		continue
	elif 'PUBX' in read:
		#client = socket.socket ( socket.AF_INET, socket.SOCK_STREAM )
		#client.connect (('192.168.1.121', 5005))

		#print(read)
	
		us_satid = []
		us_satrss = []
		cn_satid = []
		cn_satrss = []

		sdata =  read.split(',')
		#print(sdata)

		for i, j in enumerate(sdata):
			if j == '$PUBX':
				satnum = 'n' + sdata[i+2] 
			if j == 'U':
				us_satid.append(',u' + sdata[i-1])
				us_satrss.append(',s' + sdata[i+3])
			if j == 'U' and len(sdata[i-1]) > 2 :
				cn_satid.append(',c' + sdata[i-1])
				cn_satrss.append(',s' + sdata[i+3])

		idrss = ''.join(us_satid[:4] + cn_satid[:2] + us_satrss[:4] + cn_satrss[:2])

		satinfo = str(satnum + idrss)
		print(satinfo)

		#dList = [satnum, final_satid, final_satrss]	
		#objList = pickle.dumps ( dList )
		#client.send(objList)

#client.close()

#logging.basicConfig(
#	filename = 'gps.log',
#	filemode = 'w',
#	format = '',
#	level = logging.DEBUG
#	)