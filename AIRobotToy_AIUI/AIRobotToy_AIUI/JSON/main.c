#include <stdio.h>
#include <string.h>
#include "https.h"
#include "cloud_upload.h"

int main(int argc, char *argv[])
{
    // test cloud upload
    cloud_upload_start("/mnt/card");
    while(1) {
        sleep(10);
        // cloud_upload_stop();
    }

    return 0;
}
