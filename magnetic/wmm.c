#include "GeomagnetismHeader.h"

// Used the wmm_point.c as the reference to write this function
double computeMagDecl(double lat, double lon, double year) {
    MAGtype_MagneticModel *MagneticModels[1], *TimedMagneticModel;
    MAGtype_Ellipsoid Ellip;
    MAGtype_CoordSpherical CoordSpherical;
    MAGtype_CoordGeodetic CoordGeodetic;
    MAGtype_Date UserDate;
    MAGtype_GeoMagneticElements GeoMagneticElements;
    MAGtype_Geoid Geoid;
    char filename[] = "WMM.COF";
    int NumTerms, nMax = 0;
    int epochs = 1;

    MAG_robustReadMagModels(filename, &MagneticModels, epochs);

    if(nMax < MagneticModels[0]->nMax) nMax = MagneticModels[0]->nMax;
    NumTerms = ((nMax + 1) * (nMax + 2) / 2);
    TimedMagneticModel = MAG_AllocateModelMemory(NumTerms); /* For storing the time modified WMM Model parameters */
    MAG_SetDefaults(&Ellip, &Geoid); /* Set default values and constants */

    CoordGeodetic.phi = lat;
    CoordGeodetic.lambda = lon;
    CoordGeodetic.HeightAboveEllipsoid = 0;
    CoordGeodetic.HeightAboveGeoid = 0;
    CoordGeodetic.UseGeoid = 0;

    UserDate.DecimalYear = year;

    MAG_GeodeticToSpherical(Ellip, CoordGeodetic, &CoordSpherical); /*Convert from geodetic to Spherical Equations: 17-18, WMM Technical report*/
    MAG_TimelyModifyMagneticModel(UserDate, MagneticModels[0], TimedMagneticModel); /* Time adjust the coefficients, Equation 19, WMM Technical report */
    MAG_Geomag(Ellip, CoordSpherical, CoordGeodetic, TimedMagneticModel, &GeoMagneticElements); /* Computes the geoMagnetic field elements and their time change*/
    MAG_CalculateGridVariation(CoordGeodetic, &GeoMagneticElements);

    return GeoMagneticElements.Decl;
}
