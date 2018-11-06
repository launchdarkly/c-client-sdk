#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

#include "ldapi.h"
#include "ldinternal.h"

int
main(int argc, char **argv)
{
    char *const deviceid = LDi_deviceid();

    if (deviceid) {
        printf("Device ID is: %s", deviceid);
    } else {
        printf("ERROR: failed to get device ID");
    }

    free(deviceid);
}
