// ResourceAccount: one resource's production and consumption over a frame, the
// energy or metal half of a unit's or player's UnitResources. The name follows
// 0x401320, which data/symbols.csv names SettleResourceAccount and which takes
// this record as its first argument.
#ifndef GAME_RESOURCE_ACCOUNT_H
#define GAME_RESOURCE_ACCOUNT_H

struct ResourceAccount {
    float produced;                    // +0x0
    float used;                        // +0x4
    float demand;                      // +0x8
    float backlog;                     // +0xc
    float lastProduced;                // +0x10
    float lastUsed;                    // +0x14
};

#endif
