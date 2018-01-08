import re
from time import sleep

text = '$PUBX,03,20,4,-,,,37,064,10,U,000,42,16,000,12,e,062,19,25,000,14,U,311,18,25,000,18,U,046,59,17,000,20,-,122,23,,000,21,U,185,50,39,064,24,-,035,-1,,000,25,-,096,47,,000,26,U,204,14,36,064,27,-,254,01,,000,29,-,143,09,13,000,31,U,249,49,41,064,32,U,333,22,12,000,159,-,093,48,,000,161,-,114,82,,000,162,-,092,26,,000,163,-,268,38,32,000,33,-,027,33,,000,34,-,079,73,,000*3E'
sdata = text.split(',')
#print(sdata)
us_satid = []
us_satrss = []
cn_satid = []
cn_satrss = []
satnum = []

for i, j in enumerate(sdata):
	if j == '$PUBX':
		satnum.append(sdata[i+2])
	if j == 'U':
		us_satid.append(sdata[i-1])
		us_satrss.append(sdata[i+3])
	if j == 'e' or j == '-' and len(sdata[i-1]) > 2 :
		cn_satid.append(sdata[i-1])
		cn_satrss.append(sdata[i+3])
			
satid = us_satid[:4] + cn_satid[:2]
satrss = us_satrss[:4] + cn_satrss[:2]

print(satnum)
print(satid)
print(satrss)



