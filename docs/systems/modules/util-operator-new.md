# Global C++ allocation and deallocation adapters

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

The util/operator_new group consists of four entries in [operator_new.cpp](../../../src/util/operator_new.cpp), `0x4b4f10` to before `0x4b4f50`. They replace scalar/array C++ allocation and deallocation with calls into the game's allocator. Their small scope is complete here; the underlying debug heap, allocation counters and out-of-memory handler remain part of the separate diagnostics module.

## 1. Four entry contracts

| Entry | Argument and delegated operation |
| --- | --- |
| operator new, `0x4b4f10` | Byte size forwarded unchanged to GameAllocThunk |
| operator delete, `0x4b4f20` | Pointer forwarded to GameFreeIndirectThunk |
| operator new[], `0x4b4f30` | Byte size forwarded unchanged to the same allocation thunk |
| operator delete[], `0x4b4f40` | Pointer forwarded to the same free thunk |

The scalar and array forms use one underlying allocation family. These adapters do not maintain a separate object registry, remember an element type/count, initialize fields, invoke constructors/destructors or walk an array. Those C++ lifetime operations and any array bookkeeping are generated at the caller's allocation/deletion site. The allocation argument is a byte count, not a number of units or entities.

There is no extra alignment request, placement argument, supplied allocation tag or per-module arena selection in these four interfaces. They do not call calloc or zero-fill an object before its constructor. A successful allocation therefore does not mean all bytes of a partially initialized game object are zero.

## 2. Underlying allocation path

GameAllocThunk (`0x4d8660`) in [debug_lib.cpp](../../../src/debug/debug_lib.cpp) calls GameAlloc (`0x4d83c0`). The operator file declares that thunk pointer-returning, while its definition is void and delegates the allocator call. This recovered cross-file declaration mismatch is part of the current source; there is no explicit checked result conversion or exception wrapper added by operator new itself.

GameAlloc acquires the allocator critical section and chooses either the memfussy debug block allocator or ordinary malloc. The ordinary successful branch updates allocation counters. On null it calls an installed out-of-memory handler and retries while allocation remains null and the handler remains installed. With no handler it can leave the loop with null. On a successful result, an optional debug memory-set mode fills the block with its configured pattern. It then releases the lock and returns the pointer.

Thus retry/counter/debug-fill behavior comes from the underlying allocator, not separate logic in each new form. The adapters do not themselves throw std::bad_alloc, force a minimum size, make a size-overflow check or convert a null result into a fatal error. Callers and the installed handler determine how allocation failure affects a larger operation. Documentation of a caller's null check must not assume the adapter supplies one automatically.

## 3. Underlying free path and ownership

GameFreeIndirectThunk (`0x4d8670`) delegates to GameFreeThunk, which calls GameFree (`0x4d85b0`). GameFree ignores null. For a present pointer it takes the allocator lock and either releases through the memfussy heap or obtains the block size, updates counters and calls free, then unlocks. The global delete adapters add no additional null/double-free/dangling-pointer validation.

Memory returned by these operators belongs to this allocation family. Device COM objects, Windows handles, borrowed asset buffers and externally allocated blocks retain their respective release rules; merely storing one in a C++ object does not make delete the right release for that resource. Higher destructors must release their owned children before the containing allocation is freed.

## 4. Interactions and completion

All four bodies and their direct allocation/free dependencies were inspected. Common consumers include HapiBank wrappers, speech queues, model/container objects and frontend state. The adapters do not serialize allocation addresses, send network messages, create units or decide construction progress. A pointer's numeric identity can be used by other modules, but it has no persistent game ID assigned here.

No allocation probe, build or executable was run. This reference traces existing source without installations or copied source listings, and does not promote the larger allocator/diagnostic module on the strength of these four adapters.
