#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <math.h>
#include <stdbool.h>

#define MMSI_LENGTH (9 + 1) // 9 chars + 1 null terminator

// Shows an error popup with given message and a close button 
void show_error_message(const char *message);


inline bool AreEqual(double a, double b)
{
    return fabs(a - b) < 0.00001;
}

#endif // GLOBAL_H_
