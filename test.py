import re
from time import sleep

text = '$PUBX,03,20,4,-,,,37,064,10,U,000,42,16,000,12,e,062,19,25,000,14,U,311,18,25,000,18,U,046,59,17,000,20,-,122,23,,000,21,U,185,50,39,064,24,-,035,-1,,000,25,-,096,47,,000,26,U,204,14,36,064,27,-,254,01,,000,29,-,143,09,13,000,31,U,249,49,41,064,32,U,333,22,12,000,159,-,093,48,,000,161,-,114,82,,000,162,-,092,26,,000,163,-,268,38,32,000,33,-,027,33,,000,34,-,079,73,,000*3E'
sdata = text.split(',')
print(sdata)
n_index = []
q_index = []
satid = []
satrss = []
satnum = []

for i, j in enumerate(sdata):
	if j == 'U':
		n_index.append(i-1)
		q_index.append(i+3)
	if j == '$PUBX':
		satnum = sdata[i+2]

for a in n_index:
	satid.append(sdata[a])

for b in q_index:
	satrss.append(sdata[b])

final_satid = satid[0:6]
final_satrss = satrss[0:6]

print(satnum)
print(final_satid)
print(final_satrss)

