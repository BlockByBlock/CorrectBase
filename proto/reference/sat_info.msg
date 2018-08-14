uint64 timestamp	# time since system start in ms
uint8 MAX_SAT = 20

uint8 count	# num of satellites available in info
uint8[MAX_SAT] svid	# space vehicle id
uint8[MAX_SAT] used	# 0: not used, 1: used
uint8[MAX_SAT] elevation	# Elevation (0: right on top of receive, 90: on the horizon) of sat
uint8[MAX_SAT] azimuth	# Direction of sat, 0: 0 deg, 255: 360 deg
uint8[MAX_SAT] snr	#dBHz, SNR of sat, range 0-99
