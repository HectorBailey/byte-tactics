# Optional online.dll service buttons and configuration

Status: finished. The six entries of `network/online` start at `0x45b250`, before the following module boundary. They are in [online.cpp](../../../src/network/online.cpp), [button commands](../../../src/network/online_45b250.cpp), [link enumeration](../../../src/network/online_45b490.cpp) and [configuration loading](../../../src/network/online_45b670.cpp). This adapter is separate from reporter.dll statistics, HAPINET/DirectPlay transport and actual game-packet dispatch.

## DLL discovery, version gate and lifetime

Each substantial wrapper contains equivalent local LoadOnline and CheckVersion helpers. The first request fills the cached g_onlineDllPath using GetModuleFileNameA for the executable, scans backward for slash/backslash and replaces the filename with online.dll. It then loads that exact cached path when the handle is zero. Normal discovery therefore targets the executable's directory, not a data-archive resource. It does not consult HAPI, a mod directory or a configurable search list.

The buffer has MAX_PATH plus one bytes and forces its last byte to NUL after retrieving the executable path, but replacing the final component uses unbounded strcpy with no remaining-capacity test. A no-separator path can exhaust the backward scan and leave its write pointer before the buffer; normal absolute Windows executable paths contain a separator. A failed GetModuleFileName starts replacement at the buffer beginning. No path trust, signature or architecture validation is added by these helpers.

After loading, CheckVersion resolves ONLGetVersion and accepts unsigned responses at least three. Missing export, version below three or failed library load makes the wrapper take its default-result path. A library rejected by the version check remains loaded; only OnlineUnload explicitly frees it. Other exports are resolved on each wrapper call, not retained in independent global function pointers.

OnlineUnload (`0x45b640`) frees a nonzero handle and clears it. It leaves cached path, configuration bytes and configuration-loaded status intact, so a later request can reload the same path and IsOnlineConfigLoaded can still report success after unloading. It does not call a custom DLL shutdown export or free caller-owned service arrays. Existing game-loading/provider-selection consumers invoke it when handing off or leaving those flows.

## Service-button invocation and result codes

OnlineProcessButtonCommand (`0x45b250`) starts with result two. Button ID minus one skips loading/execution and returns that default. Other IDs require load/version success and an ONLProcessButtonCommand export; the export receives button ID, caller message buffer, caller size and OnlineTranslate callback. Its returned integer passes through, except that result one rewrites the message as an installation-CD prompt.

The result-one path translates a format asking for Installation CD (Disc 1), formats the existing message as the named action into a 120-byte local string, forces termination if _snprintf reports truncation, empties caller text and concatenates at most size minus one. Unlike the catch path, this branch does not guard message nonnull or size positive. Size is unsigned, so zero underflows that bound. The wrapper does not itself insert/check a disc, relaunch the game or wait for a service connection.

The source has try/catch around load/version/command execution. Its catch clears and fills the caller message with a translated fault description when message exists and size is positive, and returns two. The result-one rewrite occurs after that try/catch. These files' existing build notes specify compilation without /GX; source catch blocks are not evidence that every native access violation or DLL failure will be recovered. No external DLL code was inspected or executed for this reference.

OnlineTranslate (`0x45b480`) forwards its text pointer to Translate through a void-declared interface and discards any returned pointer. The ordinary translation implementation returns a possibly different string pointer. This adapter does not explicitly copy that returned translation into the original buffer, so it should not be described as guaranteed in-place localization for the DLL.

In the direct [provider-button consumer](../../../src/frontend/multi_4441a0.cpp), SERVICE plus a numeric suffix indexes a saved LinkInfo ID, supplies a 320-byte message and restores cursor mode around the call. Result zero unloads online.dll and exits the process. Other results show the message or a fallback service-failed/took-too-long message and preserve the provider dialog. That fallback text does not establish a timeout implemented by this wrapper; no timer or cancellation loop exists here.

## Link enumeration and translation

OnlineGetLinkInfo (`0x45b490`) starts with count zero, requires load/version success, resolves ONLGetLinkInfo and lets it fill the caller array. A [LinkInfo](../../../src/network/link_info.h) is 36 bytes: integer ID plus 32-byte name; ID minus one represents unused. There is no capacity parameter passed to the export and no returned-count bound check in this wrapper.

It walks indices zero through count minus one. For IDs other than minus one it translates the name, and if the returned pointer differs from the inline name buffer, copies up to 32 bytes and explicitly terminates byte 31. If Translate returns the same pointer it does not independently force termination. It does not compact unused records or change count to the number of valid IDs. A caught exception returns zero without rolling back any records already written/translated.

The [provider-list builder](../../../src/frontend/multi_444580.cpp) uses that result to clone service buttons. Its own loop advances across indices while counting only non-minus-one IDs until it reaches the returned count. That treats count like a valid-entry count, whereas this wrapper's translation loop treats it like the array span. The adapter does not reconcile that difference; correct DLL output must satisfy its consumers' assumptions. Neither wrapper owns the caller's service array or validates duplicate IDs, supported button operations or user-facing name contents.

## External configuration loading

Onl_LoadConfigFile (`0x45b670`) first sets g_onlineConfigLoaded to zero and clears the entire 84-integer, 336-byte configuration block at DAT_00512c80. It then requires load/version success and resolves ONLLoadConfigFile. That export receives the supplied filename, the raw configuration-block address and its byte size. Its integer result is stored without converting it to a Boolean and returned by the wrapper. Missing export/load/version returns zero. The catch also sets status zero but does not clear configuration bytes a second time after a partially writing export.

IsOnlineConfigLoaded (`0x45b660`) returns the retained integer as-is, without testing the module handle, filename, successful session connection or configuration-field validity. Loading another file discards the previous block even if the new load fails. No parser or config-file syntax is implemented in these sources: that contract is delegated to online.dll. The command-line handler in [main_49eda0.cpp](../../../src/game/main_49eda0.cpp) supplies the filename, while [settings](game-settings.md), frontend and network connection code consume individual configured fields.

Configuration state and the library lifetime are therefore independent. Unload does not undo configuration; failed reload resets it; exceptions can leave changed bytes behind a zero status. The adapter does not serialize this raw block into a battle save or send it as a game packet.

## Coverage and external boundary

All three main-file entries and three companion entries, shared LinkInfo representation and direct provider consumers have been traced. Service launch behavior, external file syntax and actual DLL internals remain explicitly delegated. Existing source explains the call contracts and failure defaults without a build, installation, browser/service launch or DLL execution, and no source listing is embedded here.
