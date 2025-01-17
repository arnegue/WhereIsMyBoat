#ifndef AISSTREAM_H_
#define AISSTREAM_H_

#include <stdbool.h>
#include "global.h"

enum Validity
{
    NO_CONNECTION,
    CONNECTION_BUT_NO_DATA,
    CONNECTION_BUT_CORRUPT_DATA,
    VALID
};

// Received data via AISStream
struct AIS_DATA
{
    double longitude;       // Current longitude
    double latitude;        // Current latitude
    char *time_utc;         // Timepoint of last AIS data
    int mmsi;               // MMSI of ship
    char *shipName;         // Name of ship
    enum Validity validity; // Validity of this struct
};

// Setup for web socket task
void setup_aisstream(const char mmsi[MMSI_LENGTH]);

// Sets new MMSI
void set_mmsi(const char mmsi[MMSI_LENGTH]);

// Returns last AIS-Data struct
struct AIS_DATA *get_last_ais_data();

#endif // AISSTREAM_H_
