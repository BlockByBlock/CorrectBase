import serial 
import re
import pickle
import socket

ser = serial.Serial()
ser.baudrate = 9600
ser.port = 'COM5'
ser.open()

client = socket.socket ( socket.AF_INET, socket.SOCK_STREAM )
client.connect (('localhost', 5005))

# Debug
print(ser.is_open)

while(1):
	read = ser.readline()
	#print(read)  #DEBUG
	if read == '':
		continue
	elif 'PUBX' in read:
		#print(read)
	
		n_index = []
		q_index = []
		satid = []
		satrss = []
		satnum = []

		sdata =  read.split(',')
		#print(sdata)

		for i, j in enumerate(sdata):
			if j == 'U':
				n_index.append(i-1)
				q_index.append(i+3)
			if j == '$PUBX':
				satnum.append(sdata[i+2])

		for a in n_index:
			satid.append(sdata[a])

		for b in q_index:
			satrss.append(sdata[b])

		final_satid = satid[0:6]
		final_satrss = satrss[0:6]

		print(satnum)
		print(final_satid)
		print(final_satrss)	

		dList = [satnum, final_satid, final_satrss]
		objList = pickle.dumps ( dList )
		client.send(objList)

client.close()

#logging.basicConfig(
#	filename = 'gps.log',
#	filemode = 'w',
#	format = '',
#	level = logging.DEBUG
#	)