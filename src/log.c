
#include <stdio.h>
#include "as65.h"

void as65_log(int err, char* msg){
    switch (err) {
        case LOG_INFO:
            printf("as65: info: %s\n", msg);
            break;
        case LOG_WARN:
            printf("as65: warning: %s\n", msg);
            break;
        case LOG_ERROR:
            printf("as65: error: %s\n", msg);
            break;

    }
}