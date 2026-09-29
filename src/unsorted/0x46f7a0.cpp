// Decompiled by space-bunny-free. Names are provisional.
//
// 80.1% (799 bytes against 798). Both non-reallocating branches, the whole
// prologue and every call target are byte identical; the single byte and every
// remaining diff are the register allocator's choice inside the reallocating
// branch, so both branch targets downstream of it are off by one.
//
// std::vector<Class_0046ded0>::insert(iterator, size_type, const value_type&)
// from MSVC 5's <vector>, emitted out of line as its own COMDAT: the callee at
// 0x4b4f10 is allocator.allocate's operator new and the one at 0x4b4f20 is
// deallocate's operator delete, both reached through the header.
//
// The element class is 0x5c bytes and its three out-of-line members are the
// copy constructor 0x470390, the copy assignment 0x470040 and the destructor
// 0x46ded0. data/symbols.csv gives the copy assignment to Class_0046eaa0 and
// the copy constructor to Class_0046ded0, so the two are modelled here as a
// base and an empty derived class, which is the only arrangement in which both
// mangled names come out right for one 0x5c-byte element. The operator= below
// is the crucial line: written as an implicit member it is emitted as its own
// out-of-line COMDAT (40.2%, and fill()/copy_backward() then call
// ??4Class_0046ded0@@ instead of 0x470040); written by hand it is one call to
// the base's and /Ob2 inlines it, which takes the score to 80.1% and puts all
// six copy-construction and three assignment loops on the right callees.
//
// What still differs, all in the reallocating branch: the original keeps _P in
// edi across the allocate call (`mov edi,[esp+0x24]` after it), and MSVC here
// spills _P to [esp+0x20] instead and reuses edi for the counter of the inlined
// _Ufill loop, which the original spills to [esp+0x20]. The two values are
// swapped between a callee-saved register and the frame, so one priority
// decision in the allocator is responsible for every diff here, including the
// *23 sign-fixup register (ecx against edx) and the order of the two stores to
// _End and the size() reload that follow. Tried and did not change it:
// the copy assignment as an implicit member, insert reached by a plain call
// (which inlines it into the caller and leaves no COMDAT at all), and a cdecl
// function-pointer typedef, which VC5 rejects (C4234, then C2563).
// What would need hand-rolling: the vector's own insert body, so that _Ufill's
// counter is prevented from coalescing with the caller's _M, which is what
// costs _P its callee-saved register here.
#include <vector>

class Class_0046eaa0 {                 // 0x5c bytes, the base subobject
public:
    char unknown_0[0x5c];

    Class_0046eaa0& operator=(const Class_0046eaa0& rhs);
};

class Class_0046ded0 : public Class_0046eaa0 {   // 0x5c bytes, no members
public:
    Class_0046ded0(const Class_0046ded0& other);
    ~Class_0046ded0();

    Class_0046ded0& operator=(const Class_0046ded0& rhs)
    {
        return (Class_0046ded0&)Class_0046eaa0::operator=(rhs);
    }
};

// One indirect call, so insert is emitted as its own out-of-line copy.
typedef void (std::vector<Class_0046ded0>::*InsertFn_0046f7a0)(Class_0046ded0*,
                                        std::vector<Class_0046ded0>::size_type,
                                        const Class_0046ded0&);

void FUN_0046f7b0(std::vector<Class_0046ded0>* v, Class_0046ded0* p,
                  std::vector<Class_0046ded0>::size_type n, const Class_0046ded0& x)
{
    InsertFn_0046f7a0 f = &std::vector<Class_0046ded0>::insert;
    (v->*f)(p, n, x);
}

// FUNCTION: 0x46f7a0 ?insert@?$vector@VClass_0046ded0@@V?$allocator@VClass_0046ded0@@@std@@@std@@QAEXPAVClass_0046ded0@@IABV3@@Z
