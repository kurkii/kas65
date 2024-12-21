

#include <stdio.h>
#include "kas65.h"

void kas65_log(int err, char* msg){
    switch (err) {
        case LOG_INFO:
            printf("kas65: info: %s\n", msg);
            break;
        case LOG_WARN:
            printf("kas65: warning: %s\n", msg);
            break;
        case LOG_ERROR:
            printf("kas65: error: %s\n", msg);
            break;

    }
}