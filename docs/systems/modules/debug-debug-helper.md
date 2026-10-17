# Optional DebugHelper.dll value adapter

Status: finished. `debug/debug_helper` is the single entry `0x4cbb70`, ending at the next module boundary `0x4cbbe0`. Its complete implementation is [debug_helper.cpp](../../../src/debug/debug_helper.cpp). The module contains no hidden diagnostic UI, worker, data structure or gameplay rule beyond this adapter.

## Call contract and normal execution

PassThroughDebugHelper takes and returns one 32-bit integer using a stdcall interface. On each invocation it asks Windows LoadLibraryA to load the literal DebugHelper.dll, resolves the literal export DebugFunc1, and if found calls it as a stdcall function taking/returning an integer. The returned value replaces the input value. The wrapper frees its acquired module reference before returning on the ordinary loaded-library paths.

The function does not cache the library handle or export pointer between calls. Its filename is not constructed relative to the executable or data directory here, so discovery follows the process's Windows library-loading context. It does not use HAPI archives, a game-resource parser or a static import of DebugFunc1. A loaded library's implementation and any side effects of its export are external to the existing source.

## Failure behavior and ownership

If loading fails, the original integer is returned and a fixed could-not-find-library diagnostic is sent to OutputDebugStringA. If loading succeeds but the export is absent, it sends a separate could-not-find-function diagnostic, frees the library reference and returns the original integer. These diagnostics are debugger output, not an in-game message box or a written log file. The wrapper does not expose the Windows error number to its caller.

There is no version/signature check, output validation or exception guard around the external function call. It assumes the found export matches the declared calling convention. An abnormal failure during that call has no guaranteed local cleanup path in this body; ordinary FreeLibrary after return is not a general crash-recovery mechanism. A result equal to the input does not distinguish missing DLL, missing export or a successful identity transformation.

The only owned runtime resource is the module reference obtained for that invocation. The integer is passed by value, with no buffer/pointer contract implied by this source. There is no retained game state, packet schema, save account or dependency on the optional helper for a documented game-system branch. A repository-wide symbol search finds the definition and no direct named C++ caller; that does not prove absence of every possible external or indirect invocation.

## Coverage boundary

The one owned body and its source symbol references have been checked. DebugHelper.dll was not loaded or executed during documentation, and no build, installation or source listing was used. The broader debug runtime is a separate module and is not counted complete by this small adapter reference.
