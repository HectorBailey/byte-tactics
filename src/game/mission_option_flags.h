// MissionOptionFlags: the four mission option flags LoadMatch hands to
// ApplyMissionOptionFlags (0x496e10), whose original decorated name gives this
// record its name. The three flag words map onto the ViewFlags bits at
// g_game+0x14281: flag_4 to bit0, flag_8 to bit1 and flag_c to bit2. The field
// names stay placeholders; naming them is #6538's job.
#ifndef MISSION_OPTION_FLAGS_H
#define MISSION_OPTION_FLAGS_H

struct MissionOptionFlags {
    int value;                          // +0x0
    int flag_4;                         // +0x4
    int flag_8;                         // +0x8
    int flag_c;                         // +0xc
};

#endif
