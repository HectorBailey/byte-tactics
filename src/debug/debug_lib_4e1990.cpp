// Decompiled by Opus. Names are provisional.
// Under the global critical section (GetNameTableLock), re-inserts a name key
// into the name map's tree at +0x0 (its LowerBound, Erase and InsertOrFind
// are the name map's, 0x4e2580, 0x4dfea0 and 0x4e2250): an existing entry is
// erased first, otherwise the "changed" flag at +0x10 is set. Erase is
// erase(iterator) and InsertOrFind is insert(key), both returning through a
// hidden pointer.
#include <windows.h>

class NameTableLock {
public:
    CRITICAL_SECTION cs;
};

NameTableLock* GetNameTableLock();

// The key: a C string ordered by strcmp.
class NameKey {
public:
    char* name;                        // +0x0
    bool LessThan(const NameKey& other) const;
};

// The name map's value_type and mapped text (debug_lib.cpp defines them).
namespace std { template<class T1, class T2> struct pair; }
struct NameMapText;

struct NameMapNode {
    NameMapNode* left;                 // +0x0
    NameMapNode* parent;               // +0x4
    NameMapNode* right;                // +0x8
    NameKey key;                       // +0xc
};

class NameMapIter {
public:
    NameMapNode* ptr;

    NameMapIter() {}
    NameMapIter(NameMapNode* p) : ptr(p) {}
    bool operator==(const NameMapIter& other) const { return ptr == other.ptr; }
    bool operator!=(const NameMapIter& other) const { return !(*this == other); }
};

class NameMapInsertResult {
public:
    NameMapIter first;
    bool second;

    NameMapInsertResult() {}
};

struct NameKeyLess {
    bool operator()(const NameKey& a, const NameKey& b) const
    {
        return a.LessThan(b);
    }
};

class NameMapTree {
public:
    // Defined in debug_lib_4e2580.cpp, keyed by the name's C string.
    NameMapIter LowerBound(const char* const& key);
    NameMapIter Erase(NameMapIter it);
    // Defined in debug_lib.cpp (0x4e2250), on the map's value_type.
    NameMapInsertResult InsertOrFind(const std::pair<const NameKey, NameMapText>& v);
};

class NameTable : public NameMapTree {
public:
    NameKeyLess compare;               // +0x0
    NameMapNode* head;                 // +0x4
    bool multi;                        // +0x8
    int size;                          // +0xc
    bool changed;                      // +0x10

    NameMapIter End() { return NameMapIter(head); }
    NameMapIter Find(const NameKey& key)
    {
        NameMapIter p = LowerBound((const char* const&)key.name);
        return (p == End() || compare(key, p.ptr->key)) ? End() : p;
    }
    void Upsert(const NameKey& key);
};

// FUNCTION: 0x4e1990
void NameTable::Upsert(const NameKey& key)
{
    NameTableLock* lock = GetNameTableLock();
    EnterCriticalSection(&lock->cs);
    NameMapIter it = Find(key);
    if (it != End())
        Erase(it);
    else
        changed = 1;
    InsertOrFind((const std::pair<const NameKey, NameMapText>&)key);
    LeaveCriticalSection(&lock->cs);
}
