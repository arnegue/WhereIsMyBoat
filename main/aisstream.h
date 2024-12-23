#ifndef AISSTREAM_H_
#define AISSTREAM_H_

#include <stdbool.h>

struct AIS_DATA {
    double longitude;
    double latitude;
    char* time_utc;
    int mmsi;
    char* shipName;
    bool isValid;
};

void setup_aisstream();
struct AIS_DATA get_last_ais_data();

#endif
