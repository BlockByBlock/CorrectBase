# GPS position in WGS84 coordinates.

# the field 'timestamp' is for the position & velocity (microseconds)
uint64 timestamp	time since system start (microseconds)

int32 lat	# lat in 1E-7 degrees
int32 lon	# lon in 1E-7 degres
int32 alt	# alt in 1E-3 meters above MSL (mm)
int32 alt_ellipsoid	# alt in 1E-3 meters above Ellipsoid (mm)

float32 s_variance_m_s	# GPS speed accuracy estimate (m/s)
float32 c_variance_rad	# GPS course accuracy estimate (rad)
uint8 fix_type	# 0-1: no fix, 2: 2D fix, 3: 3D fix, 4: RTCM code differential, 5: Real-Time Kinematic, float, 6: Real-Time Kinematic, fixed, 8: Extrapolated. Some applications will not use the value of this field unless it is at least two, so always correctly fill in the fix

float eph	# GPS horizontal position accuracy (m)
float epv	# GPS vertical position accuracy (m)

float32 hdop	# hdop
float32 vdop	# vdop

int32 noise_per_ms	# GPS noise per ms
int32 jamming_indicator	# indicate jamming occur

float32 vel_m_s	# GPS ground speed (m/s)
float32 vel_n_m_s	# GPS north velocity (m/s)
float32 vel_e_m_s	# GPS east velocity (m/s)
float32 vel_d_m_s	# GPS down velocity (m/s)

float32 cog_rad	# Course over ground (Not heading but direction of movement), -PI..PI, (rad)
bool vel_ned_valid	# True if NED velocity is valid

int32 timestamp_time_relative	# timestamp + timestamp_time_relative = Time of the UTC timestamp since system start (ms)
uint64 time_utc_usec	# Timestamp (microseconds, UTC), this is the timestamp which comes from the gps module. It might be unavailable right after cold start, indicated by a value of 0

uint8 satellites_used	# Number of satellites used

float32 heading	# heading in NED. Set to NaN if not set (used for dual antenna GPS), (rad, [-PI, PI])

