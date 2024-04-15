# Simplified version of magnetic variation computation 
Used the source code downloaded from http://www.ngdc.noaa.gov/geomag/WMM/DoDWMM.shtml

## Use:
Simply call computeMagDecl() with your coordinates and year. Year can be decimal

## Modifications dome:
I used the wmm_point.c file of the original distribution to create  computeMagDecl(). 
To remove the dependency on a file system I converted file WMM.COF to C array in wmm-2020.h
To save space in ROM I removed the geoid and just used ellipsoid. Should be good enough for recreational sailing
