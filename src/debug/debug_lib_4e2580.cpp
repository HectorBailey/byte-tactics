// Decompiled by Opus. Names are provisional.
// The name map's tree (debug/name_map_tree.h) as its lower_bound sees it,
// keyed by the name's C string: keys are ordered by strcmp, and DAT_005292c4
// is the tree's _Nil node. The iterator has constructors, so it is returned
// through a hidden pointer.
#include <string.h>
#include <yvals.h>

struct NameMapNode {
    NameMapNode* left;                 // +0x0
    NameMapNode* parent;               // +0x4
    NameMapNode* right;                // +0x8
    const char* key;                   // +0xc
};

extern NameMapNode* DAT_005292c4;

struct NameKeyLess {
    bool operator()(const char* const& a, const char* const& b) const
    {
        return a != b && strcmp(a, b) < 0;
    }
};

class NameMapIter {
public:
    NameMapNode* ptr;

    NameMapIter() {}
    NameMapIter(NameMapNode* p) : ptr(p) {}
};

class NameMapTree {
public:
    NameKeyLess compare;               // +0x0
    NameMapNode* head;                 // +0x4

    NameMapNode* Lbound(const char* const& key)
    {
        std::_Lockit lock;
        NameMapNode* x = head->parent;
        NameMapNode* y = head;
        while (x != DAT_005292c4) {
            if (compare(x->key, key))
                x = x->right;
            else
                y = x, x = x->left;
        }
        return y;
    }
    NameMapIter LowerBound(const char* const& key);
};

// FUNCTION: 0x4e2580
NameMapIter NameMapTree::LowerBound(const char* const& key)
{
    return NameMapIter(Lbound(key));
}
