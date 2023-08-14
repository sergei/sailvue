#include "InitCanBoat.h"

#define GLOBALS
#include "pgn.h"

char *sep = " ";

char  closingBraces[16]; // } and ] chars to close sentence in JSON mode, otherwise empty string
int g_variableFieldRepeat[2]; // Actual number of repetitions
int g_variableFieldIndex;
typedef enum GeoFormats
{
    GEO_DD,
    GEO_DM,
    GEO_DMS
} GeoFormats;

bool       showRaw       = false;
bool       showData      = false;
bool       showJson      = false;
bool       showJsonEmpty = false;
bool       showJsonValue = false;
bool       showBytes     = false;
bool       showSI        = true; // Output everything in strict SI units
GeoFormats showGeo       = GEO_DD;


void initCanBoat(){
    // Initialize canboat library for PGNs
    fillLookups();
    fillFieldType(true);
    checkPgnList();
}
extern bool fieldPrintVariable(const Field   *field,
                                   const char    *fieldName,
                                   const uint8_t *data,
                                   size_t         dataLen,
                                   size_t         startBit,
                                   size_t        *bits)
{
    return false;
}
