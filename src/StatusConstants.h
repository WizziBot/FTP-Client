#pragma once

#include <inttypes.h>

#define S_READY 220
#define S_STATUS_INDICATOR 211
#define S_GOODBYE 221
#define S_PASSIVE_MODE 227

#define D1_PRELIMINARY '1'
#define D1_COMPLETION '2'
#define D1_INTERMEDIATE '3'
#define D1_TRANSIENT_NEGATIVE '4'
#define D1_FAILURE '5'

#define CONTROL_PORT 21
#define DATA_PORT_MIN 35000
#define DATA_PORT_MAX 60000

enum eTransferMode {
    PASSIVE,
    ACTIVE
};

enum eConnStatus {
    CONN_NOT_INIT,
    CONN_FAILED,
    CONN_SUCCESS,
    CONN_TERM
};

enum eDataMode {
    DATA_ACTIVE,
    DATA_PASSIVE
};

enum eDataType {
    DATA_ASCII,
    DATA_BINARY
};