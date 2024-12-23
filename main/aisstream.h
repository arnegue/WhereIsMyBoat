#ifndef AISSTREAM_H_
#define AISSTREAM_H_

#include <stdbool.h>

// Received data via AISStream
struct AIS_DATA
{
    double longitude; // Current longitude
    double latitude;  // Current latitude
    char *time_utc;   // Timepoint of last AIS data
    int mmsi;         // MMSI of ship
    char *shipName;   // Name of ship
    bool isValid;     // Validity of this struct
};

// Setup for web socket task
void setup_aisstream();

// Returns last AIS-Data struct
struct AIS_DATA* get_last_ais_data();

#endif
