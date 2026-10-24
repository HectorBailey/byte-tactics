// The DAMAGE table of a weapon definition: the sorted (unit name, damage)
// entries the TDF [DAMAGE] subsection fills in, Thaldren's
// WeaponDef_DamageMap and WeaponDef_DamagePair. WeaponDef points at one at
// +0x64; 0x499cd0 looks a unit name up in it.
//
// The pair holds a StringRef, and StringRef has no header yet (#7201), so the
// includer declares its own before this one.
#ifndef WEAPON_DAMAGE_H
#define WEAPON_DAMAGE_H

#include <vector>

// One entry: a unit name and its damage.
struct WeaponDef_DamagePair {
    StringRef unitName;                // +0x0
    int damage;                        // +0x4

    WeaponDef_DamagePair(const StringRef& n, int v) : unitName(n), damage(v) {}
};

#pragma pack(push, 1)
// The table, kept sorted by name.
struct WeaponDef_DamageMap {
    char less;                         // +0x0
    std::vector<WeaponDef_DamagePair> entries; // +0x1

    WeaponDef_DamagePair* LowerBound(const char* key)
    {
        WeaponDef_DamagePair* first = entries.begin();
        WeaponDef_DamagePair* last = entries.end();
        while (first != last) {
            WeaponDef_DamagePair* mid = first + (last - first) / 2;
            if (Less(mid->unitName.ptr, key))
                first = mid + 1;
            else
                last = mid;
        }
        return first;
    }
};
#pragma pack(pop)

#endif
