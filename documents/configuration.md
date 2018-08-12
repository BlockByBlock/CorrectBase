# Configuration

### Required Software

- U-Center Windows 8.29 ([Download from here](https://www.u-blox.com/en/product/u-center-windows))



### Steps for use with RTKLIB

##### Configuring receiver transmitted messages

1. Open *Message View* sub-window in u-Center
2. Under *Message View*, find **RXM-RAWX** message, enable the **RAWX** and **RXM-MEASX** message
3. Verify the **UBX RXM-MEASX** and **UBX RXM-RAWX** message are shown in *Packet Console*
4. Can disable other **NMEA** message, RTKLIB uses **RXM-RAWX**



##### Sending over serial ports

This step is necessary for both base and rover units

1. Open up *Configuration View* and select **MSG(Messages)** 
2. Look for **02-15 RXM-RAWX**, check **UART1** and **USB**
3. Click *Send* to write configuration
4. Repeat step for **02-14 RXM-MEASX** message



##### Setting baud rate

Run at higher baud rate for RTKLIB to process them in time, setting is available in *Configuration View*

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



##### Saving configurations to GNSS receiver

Saving settings into the receiver in *Configuration View*, so that the receiver starts up in the same configuration after power cycle. This also avoid the need to rely on RTKLIB's command settings for startup/shutdown. 

1. Select *CFG(Configuration)* under *Configuration View*
2. Under *UBX - CFG(Config) - CFG(Configuration)*, select **Save current configuration** 
3. For *Devices*, choose **0 - BBR** and **1 - Flash**
4. Under *Save* column, select all
5. Click *Send* at the bottom left of the console to write all these configuration values to the internal memory of uBlox M8P
6. Unplug and re-plug the receiver and check *Packet Console* that the configuration works!





