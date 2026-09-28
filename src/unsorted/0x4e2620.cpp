// Decompiled by space-bunny-free. Names are provisional.
// The red-black tree insert of MSVC 5's <xtree> (_Tree::_Insert) for the
// name table at +0: the hint node and the value are passed in and the new
// node comes back through a hidden pointer because iterator has constructors.
// The node is 0x208 bytes: left/parent/right, a 504-byte value (a
// Class_004e1a30 key plus a 500-byte buffer) and the colour at +0x204,
// 0 = red and 1 = black. DAT_005292c4 is the tree's _Nil, DAT_00529e58 the
// pooled node free list. See 0x4e17c0.cpp for the singleton's constructor and
// 0x4df380.cpp for the same node layout.
// Two things differ from the SP3 <XTREE> header: the else arm that would make
// _Right(_Y) = _Z is not in the original at all, and the "straight line" arm
// rotates _Parent(_X) without first assigning it to _X, so the recolouring
// after the rotation names the old _X. Both are latent here: the only caller
// passes _X == _Nil and a lower bound as _Y, so key_compare is always true
// and the missing else is never reached.
//
// NOT MATCHED yet (46.2%, best of 3 real runs). What still differs:
//  * The link-up block (_Left(_Y) = _Z and the three _Head updates) is placed
//    INLINE by this source, after the ++_Size tests, while the original puts
//    it COLD: all three conditions jump forward to it and it lives after the
//    epilogue and the ret, at 0x4e2926..0x4e294b, 37 bytes. That single
//    layout difference misaligns the whole rest of the function, so the
//    biggest win left is finding the source shape that makes MSVC 5 sink that
//    block. Tried and did not work: a do{}while(0) wrapper, an empty else.
//  * Because of that, the third test of the || chain is inverted here
//    (neg eax) where the original branches on the result of
//    Class_004e1a30::FUN_004e1a30.
//  * The loop back edge and the final _Color(_Root()) = _Black store use edi
//    and an immediate 1 here, where the original keeps _Black live in ebx
//    across the whole loop (mov ebx,1 just before it). A named local for the
//    colour is the obvious thing to try next.
// The value copy needed a user-declared copy assignment: with a plain POD
// struct MSVC 5 emits a bare rep movsd, the original has the
// test edi,edi / je self-pointer guard first, which is worth 2 instructions.
#include <string.h>
#include <yvals.h>

class Class_004e1a30 {
public:
    char* name;                        // +0x0
    int FUN_004e1a30(const Class_004e1a30& other) const;
};

struct Value_004e2620 {
    Class_004e1a30 key;                // +0x0
    char text[500];                    // +0x4

    Value_004e2620& operator=(const Value_004e2620& v)
    {
        if (this != &v)
            memcpy(this, &v, sizeof(Value_004e2620));
        return *this;
    }
};

struct Node_004e2620 {
    Node_004e2620* left;               // +0x0
    Node_004e2620* parent;             // +0x4
    Node_004e2620* right;              // +0x8
    Value_004e2620 value;              // +0xc
    int color;                         // +0x204
};

extern Node_004e2620* DAT_005292c4;    // the tree's _Nil

class Class_004e2b60 {
public:
    void* FUN_004e2b60(unsigned int n);
};

struct Less_004e2620 {
    bool operator()(const Class_004e1a30& a, const Class_004e1a30& b) const
    {
        return a.FUN_004e1a30(b);
    }
};

class Iter_004e2620 {
public:
    Node_004e2620* ptr;

    Iter_004e2620() {}
    Iter_004e2620(Node_004e2620* p) : ptr(p) {}
};

class Class_004e2620 {
public:
    Less_004e2620 compare;             // +0x0
    Node_004e2620* head;               // +0x4
    int nilref;                        // +0x8
    unsigned int size;                 // +0xc

    Node_004e2620* root() { return head->parent; }
    void Lrotate(Node_004e2620* _X)
    {
        std::_Lockit _Lk;
        Node_004e2620* _R = _X->right;
        _X->right = _R->left;
        if (_R->left != DAT_005292c4)
            _R->left->parent = _X;
        _R->parent = _X->parent;
        if (_X == head->parent)
            head->parent = _R;
        else if (_X == _X->parent->left)
            _X->parent->left = _R;
        else
            _X->parent->right = _R;
        _R->left = _X;
        _X->parent = _R;
    }
    void Rrotate(Node_004e2620* _X)
    {
        std::_Lockit _Lk;
        Node_004e2620* _L = _X->left;
        _X->left = _L->right;
        if (_L->right != DAT_005292c4)
            _L->right->parent = _X;
        _L->parent = _X->parent;
        if (_X == head->parent) {
            head->parent = _L;
            _L->right = _X;
            _X->parent = _L;
        } else if (_X == _X->parent->right) {
            _X->parent->right = _L;
            _L->right = _X;
            _X->parent = _L;
        } else {
            _X->parent->left = _L;
            _L->right = _X;
            _X->parent = _L;
        }
    }

    Iter_004e2620 FUN_004e2620(Node_004e2620* _X, Node_004e2620* _Y, const Value_004e2620& _V);
};

// FUNCTION: 0x4e2620
Iter_004e2620 Class_004e2620::FUN_004e2620(Node_004e2620* _X, Node_004e2620* _Y, const Value_004e2620& _V)
{
    std::_Lockit _Lk;
    Node_004e2620* _Z = (Node_004e2620*)((Class_004e2b60*)this)->FUN_004e2b60(0x208);
    _Z->parent = _Y;
    _Z->color = 0;
    _Z->left = DAT_005292c4;
    _Z->right = DAT_005292c4;
    _Z->value = _V;
    ++size;
    if (_Y == head || _X != DAT_005292c4 || compare(_V.key, _Y->value.key)) {
        _Y->left = _Z;
        if (_Y == head) {
            head->parent = _Z;
            head->right = _Z;
        } else if (_Y == head->left)
            head->left = _Z;
    }
    for (_X = _Z; _X != head->parent && _X->parent->color == 0; ) {
        if (_X->parent == _X->parent->parent->left) {
            Node_004e2620* _U = _X->parent->parent->right;
            if (_U->color == 0) {
                _X->parent->color = 1;
                _U->color = 1;
                _X->parent->parent->color = 0;
                _X = _X->parent->parent;
            } else {
                if (_X == _X->parent->right)
                    Lrotate(_X->parent);
                _X->parent->color = 1;
                _X->parent->parent->color = 0;
                Rrotate(_X->parent->parent);
            }
        } else {
            Node_004e2620* _U = _X->parent->parent->left;
            if (_U->color == 0) {
                _X->parent->color = 1;
                _U->color = 1;
                _X->parent->parent->color = 0;
                _X = _X->parent->parent;
            } else {
                if (_X == _X->parent->left)
                    Rrotate(_X->parent);
                _X->parent->color = 1;
                _X->parent->parent->color = 0;
                Lrotate(_X->parent->parent);
            }
        }
    }
    head->parent->color = 1;
    return Iter_004e2620(_Z);
}
