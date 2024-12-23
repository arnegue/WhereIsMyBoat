#ifndef AISSTREAM_H_
#define AISSTREAM_H_

struct AIS_DATA {
    double longitude;
    double latitude;
    char* time_utc;
    char* mmsi;
    char* shipName;
};

void setup_aisstream();
void get_position(double *latitude, double *longitude);

#endif 