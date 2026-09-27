// Decompiled by space-bunny-free. Names are provisional.
// The quicksort driver of the old MSVC STL sort, working on an array of item
// pointers ordered by FUN_00485940. The partition step (0x488960 out of line,
// inlined here) splits the range around the median of the first, middle and
// last entries; only ranges of more than 16 entries are handled here, the
// shorter ones are left to the caller's insertion sort pass. The fourth
// parameter is never read, and the recursive calls pass 0 for it.

struct Item_00485940;

typedef int (__stdcall* Pred_00488810)(Item_00485940*, Item_00485940*);

// byte span of a range, rounded down to a whole entry
#define SPAN_00488810(lo, hi) ((((char*)(hi) - (char*)(lo)) & ~3))

static Item_00485940** __stdcall partition_00488810(Item_00485940** first,
                                                    Item_00485940** last,
                                                    Item_00485940* pivot,
                                                    Pred_00488810 pred)
{
    for (;; ++first) {
        for (; pred(*first, pivot); ++first)
            ;
        for (; pred(pivot, *--last);)
            ;
        if (last <= first)
            return first;
        Item_00485940* t = *first;
        *first = *last;
        *last = t;
    }
}

// FUNCTION: 0x488810
void __stdcall FUN_00488810(Item_00485940** first, Item_00485940** last,
                            Pred_00488810 pred, int unused)
{
    while (SPAN_00488810(first, last) > 0x40) {
        Item_00485940* p = first[0];
        Item_00485940* q = first[(last - first) / 2];
        Item_00485940* r = last[-1];
        Item_00485940* pivot = p;

        if (pred(p, q) != 0) {
            if (pred(q, r) != 0)
                pivot = q;
            else if (pred(p, r) != 0)
                pivot = r;
        } else if (pred(p, r) == 0) {
            pivot = pred(q, r) != 0 ? r : q;
        }

        Item_00485940** cut = partition_00488810(first, last, pivot, pred);

        if (SPAN_00488810(cut, last) > SPAN_00488810(first, cut)) {
            FUN_00488810(first, cut, pred, 0);
            first = cut;
        } else {
            FUN_00488810(cut, last, pred, 0);
            last = cut;
        }
    }
}
