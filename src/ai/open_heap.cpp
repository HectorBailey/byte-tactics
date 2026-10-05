// Decompiled by Opus and DeepSeek V4.1 Flash. Names are provisional.

struct HeapNode_0040f000 {
    int index;                           // +0x0
    char unknown_4[8];
    int key;                             // +0xc
};

class OpenHeap {
public:
    char unknown_0[4];
    HeapNode_0040f000** items;           // +0x4
    char unknown_8[0x14 - 0x8];
    int count;                           // +0x14

    void SiftDown(int i);
    void SiftUp(int i);
};

// Sift-up of a binary min-heap of node pointers; each node stores its heap index.
// FUNCTION: 0x40f000
void OpenHeap::SiftUp(int i)
{
    if (i != 0) {
        int parent = (i - 1) >> 1;
        HeapNode_0040f000* node = items[i];
        HeapNode_0040f000* p = items[parent];
        if (node->key < p->key) {
            items[i] = p;
            p->index = i;
            i = parent;
            while (i != 0) {
                parent = (i - 1) >> 1;
                p = items[parent];
                if (node->key >= p->key) break;
                items[i] = p;
                p->index = i;
                i = parent;
            }
            items[i] = node;
            node->index = i;
        }
    }
}

// Sift-down of a binary min-heap of node pointers; each node stores its heap index.
// FUNCTION: 0x40f060
void OpenHeap::SiftDown(int i)
{
    HeapNode_0040f000* node = items[i];
    while (true) {
        int left = i * 2 + 1;
        int right = i * 2 + 2;
        if (right < count) {
            HeapNode_0040f000* l = items[left];
            HeapNode_0040f000* r = items[right];
            if (r->key < l->key) {
                if (r->key >= node->key)
                    break;
                items[i] = r;
                r->index = i;
                i = right;
            } else {
                if (l->key >= node->key)
                    break;
                items[i] = l;
                l->index = i;
                i = left;
            }
        } else {
            if (left >= count)
                break;
            HeapNode_0040f000* l = items[left];
            if (l->key >= node->key)
                break;
            items[i] = l;
            items[left]->index = i;
            i = left;
        }
    }
    items[i] = node;
    node->index = i;
}
