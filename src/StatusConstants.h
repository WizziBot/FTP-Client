#pragma once

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