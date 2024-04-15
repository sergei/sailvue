#ifndef IMU2NMEA_WMM_H
#define IMU2NMEA_WMM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Computes the magnetic declination for given point in time
 * in order to save ROM the geoid model is not included
 */
double computeMagDecl(double lat, double lon, double year);

#ifdef __cplusplus
};
#endif

#endif //IMU2NMEA_WMM_H
