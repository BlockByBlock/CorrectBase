# RTCM3 Message Type

RTCM 3.2 messages - based off from [RTCM cheat sheet](https://www.use-snip.com/kb/knowledge-base/an-rtcm-message-cheat-sheet/), PX4 docs and [AlexisTM](https://github.com/AlexisTM/rtk_ros). 

:x: - Indicate not use



### Common Message Types

-  :x: **1004** - Extended L1&L2 GPS RTK Observables for GPS RTK Use 
- **1005** - Station coordinates XYZ for antenna reference point (Base position).
-  :x: **1006** - Stationary RTK Reference Station ARP plus the Antenna Height  
-  :x: **1007** - Antenna Descriptor 
- :x: **1012** - Extended L1&L2 GLONASS RTK Observables 



### Multiple Signal Messages (MSM) Message Types

- **1077** - Full GPS pseudo-ranges, carrier phases, Doppler and signal strength (high resolution).
  - 1071 - 1077 is range of messages GPS uses
  - For uBlox with RTKLIB - use 1077
- **1087** - Full GLONASS pseudo-ranges, carrier phases, Doppler and signal strength (high resolution).
  - 1081 - 1087 is range of messages GLONASS uses
  - For uBlox with RTKLIB - use 1087
- **1127** - Full Beidou pseudo-ranges, carrier phases, Doppler and signal strength (high resolution).
  - 1121 - 1127 is range of message Beidou uses 
- :x: 4001 - 4095 are proprietary messages
  - **4095** - Ashtech
  - **4093** - NovAtel
  - **4072** - Mitsubishi Electric Corp