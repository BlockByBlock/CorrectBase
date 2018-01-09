# BaseReader

### Setting up the M8P GPS Base Station

1. Launch u-center, disable NMEA in 'View' > 'Message View'
2. Enable '03 (Satellite Data)' under 'NMEA' in 'View' > 'Message View'

3. Set the TCP IP and Port on server.py
    ```
    $ python server
    ```

4. Set the TCP IP, TCP Port, Serial Port on sgps.py
    ```
    $ python sgps.py
    ```
    
### GPS Format

1. Number of satellites
2. Satellite ID (4 GPS and 2 others - USE only)
3. Satellite RSS (based on the 6 ID-ed satellites)


### Satellite Numbering

GPS: 1-32
Beidou: 159-163,33-64
IMES: 173-182
GLONASS: 65-96, 255
SBAS: 120-158
Galileo: 211-246
QZSS: 193-197

### PUBX Format

Message Structure:
    ```
    $PUBX,03,GT{,sv,s,az,el,cno,lck},*cs<CR><LF>
    ```
Example:
    ```
    $PUBX,03,11,23,-,,,45,010,29,-,,,46,013,07,-,,,42,015,08,U,067,31,42,025,10,U,195,33,46,026,18,U,32
    6,08,39,026,17,-,,,32,015,26,U,306,66,48,025,27,U,073,10,36,026,28,U,089,61,46,024,15,-,,,39,014*0D
    ```
1. $PUBX - header
2. msgID - 03
3. Number of satellite
4. Satellite ID
5. Satellite Status (U,e,-)
6. Azimuth (0-359)
7. elevation (0-90)
8. Signal strength (0-99)
9. Clock lock (0-64)

