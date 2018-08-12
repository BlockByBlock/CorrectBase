# Using RTKLIB

Using str2str to translate uBlox raw messages into RTCM3 messages and send out as NTRIP 

### Installation

Installing of RTKexplorer's RTKLIB 

```
$ git submodule update --init --recursive
$ cd ~BaseReader/RTKLIB/apps && make

# For invoking rtkrcv directly from bash instead of using full path
$ sudo make install
```



To run

```
$ str2str -in serial://ttyACM0:115200#ubx -out ntrips://user:foobar@192.168.1.109:2101/baseTest#rtcm3 -c m8p_5hz_usb_base.cmd -msg 1004,1005,1006,1007,1074,1075,1124,1125,1127

```



- Set input with baud rate

  ```
  -in serial://ttyACM0:115200#ubx
  ```

- Specify uBlox protocol 

  ```
  #ubx
  ```

- Set output to NTRIP server to specific caster's IP and port

  ```
  -out ntrips://user:foobar@192.168.1.109:2101/baseTest#rtcm3
  ```

  The user name and password should match the expected values on the SNIP application side, and a unique mountpoint (in this case, `baseTest`) should be used. 

- RTCM3 format

  ```
  -c m8p_5hz_usb_base.cmd
  ```

- RTCM3 messages 

  ```
  -msg 1004,1005,1006,1007,1074,1075,1124,1125,1127
  ```

  

Use ` -t 3` to debug



### Setting the baud rate on Linux

```
$ stty -F /dev/ttyACM0 115200
```

