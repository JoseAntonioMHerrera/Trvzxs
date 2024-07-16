/*#include "displayTimer.h"

void initBegginingOfTimes(DisplayDelta* delta){
    clock_gettime(CLOCK_MONOTONIC, &delta->start);
}

float getTimeDifference(DisplayDelta* delta) {
    struct timespec currentTime;
    clock_gettime(CLOCK_MONOTONIC, &currentTime);

    // Calculate the difference in seconds
    float timeDifference = (currentTime.tv_sec - delta->start.tv_sec) +
                           (currentTime.tv_nsec - delta->start.tv_nsec) / 1e9f;

    return timeDifference;
}
*/
