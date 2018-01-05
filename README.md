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
2. Satellite ID (Up to 6 - USE mode only)
3. Satellite RSS (based on the 6 ID-ed satellites)
