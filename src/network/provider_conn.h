// HapinetProviderConnEntry: one service provider's DirectPlay connection data,
// 8 bytes, the elements of the array HAPINET_getconnections fills (the lobby's
// DPLAY_CONNECTIONS buffer and the +0x435 array of the HAPINET session): the
// connection blob the EnumConnections callback copies out of DirectPlay, and
// its size. Thaldren's name and fields (structs.csv). The one declaration of
// the record, for network/hapinet.cpp, which fills it, and the frontend files
// that copy one out to open a connection.
#ifndef PROVIDER_CONN_H
#define PROVIDER_CONN_H

struct HapinetProviderConnEntry {
    void* pConnectionBlob;             // +0x0, the connection data
    unsigned long dwCbConnectionBlob;  // +0x4, its size in bytes
};

#endif
