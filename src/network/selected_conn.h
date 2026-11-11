// SelectedConnection: the service provider connection the player picked on the
// connect screen, its DirectPlay GUID and its provider's connection block.
// One definition for src/frontend/multi.cpp and src/frontend/multi_443ff0.cpp,
// which each had their own copy of it. Needs GUID, so include it after
// <windows.h> or after a local GUID.
#ifndef SELECTED_CONN_H
#define SELECTED_CONN_H

#include "provider_conn.h"

#pragma pack(push, 1)

struct SelectedConnection {
    GUID guid;                           // +0x0
    HapinetProviderConnEntry conn;       // +0x10
};

#pragma pack(pop)

#endif // SELECTED_CONN_H
