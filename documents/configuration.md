# Configuration

### Required Software

- U-Center Windows 18.06 - *updated 31 Aug 2018* ([Download from here](https://www.u-blox.com/en/product/u-center-windows))



### Steps for use with GPS Driver

##### Configuring receiver transmitted messages

1. Open *Message View* (F9) sub-window in u-Center
2. Disable **NMEA** message



##### Sending over serial ports

This step is necessary for both base and rover units

1. Open up *Configuration View* (F10) and select **MSG(Messages)** 
2. Look for **F5-05 RTCM3.3 1005**, tick **UART1** and **USB**
3. Click *Send* to write configuration
4. Repeat step 1-3 for 
   * F5-4D RTCM3.3 1077
   * F5-57 RTCM3.3 1087 
   * F5-7F RTCM3.3 1127



##### Saving configurations to GNSS receiver

Saving settings into the receiver in *Configuration View*, so that the receiver starts up in the same configuration after power cycle. This also avoid the need to rely on GPSDriver's command settings for startup/shutdown. 

1. Select *CFG(Configuration)* under *Configuration View*
2. Under *UBX - CFG(Config) - CFG(Configuration)*, select **Save current configuration** 
3. For *Devices*, choose **0 - BBR** and **1 - Flash**
4. Under *Save* column, select all
5. Click *Send* at the bottom left of the console to write all these configuration values to the internal memory of uBlox M8P
6. Unplug and re-plug the receiver and check *Packet Console* that the configuration works!



##### Setting the survey-in

Under **UBX - CFG (Config) - TMODE3 (Time Mode 3)**, select `1 - Survey-in` under Mode

* Recommended *Minimum Observation Time*: 90s
* Recommended *Required Position Accuracy*: 2.5m

Remember to click *Send* after setting up!



##### Setting baud rate

Run at higher baud rate for GPSdriver to process them in time, setting is available in *Configuration View* (F10) and select **PRT(Ports)**

```
Target:       1 - UART1 (or serial port of choice)
Protocol in:  0+1+5 - UBX-NMEA-RTCM3 
              (this is which protocol the receiver accepts for corrections)
Protocol out: 0+1 - UBX-NMEA 
              (output protocol, in this case uBlox binary and NMEA sentences)
Baudrate:     115200 (at least this rate or higher for RTKLIB processing)

(These following are pretty much untouched, but tweakable here if needed)
Databits:  8
Stopbits:  1
Parity:    None
Bit Order: LSB First
```

Click *Send* to write configuration to receiver.

Disconnect and reconfigure U-Center to reconnect to the receiver.



##### Setting the rate

Update measurement period in *Configuration View* (F10) and select **RATE(Rates)**

Recommended: 200ms - 1000ms





