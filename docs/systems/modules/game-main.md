# Process entry, command line, application pump and failure reporting

## Scope and owned entries

The `game/main` group owns eleven entries in `0x49e640..0x49f610`: eight in [main.cpp](../../../src/game/main.cpp), WinMain/ParseCommandLine in [main_49eda0.cpp](../../../src/game/main_49eda0.cpp), and ReportViaException in [main_49e680.cpp](../../../src/game/main_49e680.cpp). It establishes the application, interprets launch arguments, runs the Windows/game dispatch loop and provides logging, language/INI helpers and out-of-memory reporting.

The source was traced without executing launch arguments or modifying Windows settings. Descriptions of registry/device changes below are game behavior, not actions taken to prepare this documentation. [Display/environment](graphics-display.md), [game-state resources](game-game-state.md), [archive registration](game-cd-check.md) and [application frames](../../../src/ingame/screenshots.cpp) remain distinct call boundaries.

## Entry, instance check and startup sequence

WinMain (`0x49eda0`) calls GameMain inside structured exception handling with the reporting label `Main Thread`. An exception is delegated to ExceptionFilter; there is no initialization rollback in this wrapper. Its handler falls through to return zero if exception handling returns control.

GameMain (`0x49e830`) initializes debug support with argument eight and installs OutOfMemoryHandler. It registers AtexitNoOp once using a global bit, then creates the game object. The object allocator is [owned by the endgame group](../../../src/frontend/endgame.cpp): it zeroes a larger allocation and constructs the Game at a tick-dependent offset. This call occurs before single-instance detection, not after all early exits are excluded.

It attempts to open a semaphore named after the application. An existing handle causes return -1. Otherwise it creates a semaphore with initial/maximum count one, seeds CRT rand from calendar time and parses arguments. The semaphore is used as an existence marker; this body does not wait on it or release its count. It does not retain/close the created handle here. Separate open/create calls are not an atomic “only one process can pass” operation.

A parser result zero returns one before ordinary display initialization. Otherwise the function initializes display defaults, selects an initial 640-by-480 environment, supplies instance/show/class/title/menu fields and assigns several display option bits. Video bit 0 receives the low bit of the complemented display seed. The default seed and `-d` parser choice therefore matter through that bit mapping, not through a later string-mode lookup.

InitEnvironment failure returns zero immediately. On success, GameMain initializes timers with argument 30, registers data archives and runs the post-mount hook. It binds the display pointer into Game and stores version bytes 3, 1, 1. It resolves preferred language from the command line or registry/default, loads translations, constructs Sound, invokes a prefrontend hook and calls InitGame.

These early returns do not call the normal shutdown tail in this body. Initialization failures, registration-only exits, an already running instance and an exception consequently have different cleanup paths. Creating the global game object alone does not mean application resources or battle resources were loaded.

## Command-line tokenization and defaults

ParseCommandLine (`0x49ee30`) stack-allocates a mutable copy of the entire command line and tokenizes it with strtok using space and tab delimiters. There is no quotation or escaping grammar, so a quoted multiword name is still split. It initializes lobbyTimeoutSecs to 30, simulated network drop percentage to zero and skip-intro state to zero.

An unprefixed token overwrites the preferred-language string with an unbounded copy. Multiple such tokens therefore leave the last one. Tokens beginning with `-` or `/` are first passed intact to IsDebugKeyword. That [debug-owned helper](../../../src/debug/debug_lib.cpp) compares against nineteen keyword prefixes case-insensitively; recognized debug tokens bypass this parser's switch. The switch otherwise uses the second character, accepting uppercase and lowercase variants. Unknown switches have no action.

Most value-taking cases advance two characters and use a following token if the attached remainder is empty. This is not consistent across every case, and tokens consumed as values are not put back if they resemble another switch. A later command-line argument may therefore disappear into a previous option's validation path.

| Option | Observed behavior |
| --- | --- |
| B/b | Tests a list of battle words, but does not advance past the option prefix. Its test for an empty current token cannot succeed for an ordinary `-b`/`/b` token. See the broken-consumption detail below. |
| C/c | Loads attached/following online configuration through Onl_LoadConfigFile, swallowing C++ exceptions. Sets skip-intro even without a valid value. |
| D/d | Inspects the third character only. F/f selects seed 2; any other character, including none, selects seed 3. It does not consume a separate mode token. |
| E/e | Parses attached/following simulated packet-drop percentage with atoi unless it starts with `-`; values outside 0..100 become zero. Missing value leaves the initialized setting. |
| F/f | Calls SetBypassDriveScan with one. The CD helper then probes its hardcoded drive rather than automatically accepting all content. |
| H/h | Sets host-game name from the attached remainder, or consumes one following token if nonempty and not beginning with `-`. A rejected following token has still been consumed. |
| L/l | Calls SetCommandLineUnusedFlagL with zero. No further policy is established by this parser. |
| N/n | Parses attached/following provider number when the value does not begin with `-`. For provider one and a colon, sets direct-connect address to the text after the colon. Always sets skip-intro. |
| P/p | Parses attached/following packet rate, passing -1 when a supplied value begins with `-`. Clamping/meaning belongs to SetPacketRate. |
| R/r | Registers the executable as a lobbyable DirectPlay application using the untouched remaining original command-line text, then returns zero regardless of registration success. |
| S/s | Selects no DirectSound through SetNoDirectSound. |
| T/t | Parses attached/following lobby timeout; values outside 30..300 become 30. |
| W/w | Selects Windows sound fallback through SetUseWindowsSound. |

The B case compares its current token to `lock`, `deathends`, `deathplays`, `deathmatch`, `fixedloc`, `mapping`, `circlos`, `truelos`, `permlos`, `cheating` and `watching`. Only lock has a mutation, setting lockFlags bit 0; the others are empty branches. Because the current token still includes its switch prefix, normal `-b lock` does not reach that mutation. The next outer token `lock` instead becomes a language string. Attached `-block` also is not equal to the unprefixed word. The source's literal control flow must not be turned into a promised working battle-option grammar.

atoi accepts numeric prefixes and returns zero for many nonnumeric strings; these cases do not require a complete valid decimal token. Rejection of leading `-` is also not a universal prefixed-switch test: several branches can accept a slash-prefixed token as a value. There are no general command-line-length or preferred-language-capacity checks before the copy.

## DirectPlay registration-only path

The R case locates the original text after the option using the mutable copy's token offset, skips whitespace and passes the remaining original text as the registered command line. It queries the current executable, splits drive/directory/filename/extension, combines the filename and prepares a DirectXRegisterApp structure with application name, fixed application GUID, executable name, path and current directory.

It loads `dsetup.dll`, resolves DirectXRegisterApplicationA and calls it when available. It then releases the module. A failure to obtain the executable path, load/resolve the helper or receive a successful registration result triggers a beep and translated failure message. The parser still returns zero, causing GameMain's registration-only early return. It does not proceed into normal gameplay after a failed registration.

This branch dynamically calls the installed legacy registration API; it does not launch a game, join a lobby or validate a multiplayer map. No installation or registration was performed for the documentation.

## Language and INI helpers

If g_preferredLanguage is empty during startup, GameMain reads game registry value `language` with a 64-byte size argument. If the first byte remains zero it copies the default `english`. It loads `gamedata/translate.tdf` for that language. Literal values are established by the named entries in [globals.csv](../../../data/globals.csv), not inferred solely from symbol names.

GetPreferredLanguage (`0x49f580`) returns zero for an empty string or its pointer represented as an integer otherwise. Callers cast that result back to text. It does not validate a language name or test that translations exist. [TDF translation caching](util-tdf.md) and [data path fallback](game-data-files.md) explain the consequences for selected strings and assets.

ChdirToExeDirectory (`0x49f540`) gets the executable path into 256 bytes, removes its filename and sets the process current directory. It discards API results and does not restore the prior directory. GetPreferenceInt (`0x49f5a0`) similarly obtains/strips the executable name, forms a path ending in `totala.ini` and reads the Preferences section with GetPrivateProfileIntA using the caller's key/default. It returns an unsigned integer and imposes no per-setting range clamp. Callers, such as the UnitLimit initialization, supply their own validation.

Registry string reads here do not independently guarantee termination after a full-size/truncated value. The [registry helper](util-registry.md) also accepts more-data results as success, so a completed read call is not proof that every requested string fits its destination.

## AudioCD shell preference handling

After InitGame, GameMain opens the machine-wide `SOFTWARE\Classes\AudioCD\shell` key. If its default value can be queried, it saves those bytes and writes a one-byte empty value of the same registry type. If querying fails it makes the saved text empty. It then reads the game's `cdshell` preference into a second buffer: a nonempty saved preference replaces the current backup, otherwise it persists the backup as a fixed 50-byte game value. It flushes/closes the machine key.

This temporarily changes shell behavior associated with audio CDs. It is separate from MCI CD track playback and from HPI game-data mounts. The game does not guarantee the key exists or access is granted; it simply skips this block when opening fails.

At normal exit it reopens that machine key, writes the backed-up text with strlen plus one, flushes/closes and clears the game's cdshell preference using another fixed 50-byte write. There is no independent initialization of every backup/type field outside the first successful block. If startup could not populate them but exit can open the key, the restoration path can use uninitialized state. Abnormal process termination also bypasses the ordinary restore tail. These are limits of the traced preservation scheme, not registry changes made during this work.

## Message pump, focus and sound maintenance

GameMain runs an inner idle-frame loop inside an outer Windows message loop. Before checking messages, it handles CD focus state. When inactive and the Sound object's first device field is nonzero, it saves CD categories, remembers the current category, closes CD audio and marks it for reopening. When active again with that field zero and the reopen flag set, it opens CD audio, restores callback/enabled/order/category choices, calls ReopenCdAudio and clears the flag.

If PeekMessage reports pending Windows traffic, the inner loop stops. It also stops when the display is inactive and network-game flag bit 0 is clear. Otherwise it invokes MainFrameTick and periodically reaps finished sound buffers when at least 100 GetTickCount milliseconds have elapsed since the last reap. There is no generic sleep in this active idle-frame loop.

The outer loop calls GetMessage. Zero exits; otherwise it translates and dispatches the message. A GetMessage error value of -1 is not separately handled before translation/dispatch in this source. When inactive and nonnetworked, blocking in GetMessage avoids ordinary idle simulation. A network-form game can continue invoking frames while inactive, subject to the application callback's own gates.

MainFrameTick handles input/timers and invokes whichever state callback is installed, so the pump is not unconditionally a battle simulation loop. [Battle frames](game-main-loop.md) can render without advancing ticks, and [match loading](game-game-load.md) uses the same dispatch mechanism for its main-thread synchronization/progress work.

When the message loop ends, display flag bit 11 gates ShutdownMouse and ShutdownGame. The function attempts AudioCD shell restoration, shuts down the environment and returns the final message's wParam. It does not itself unconditionally call battle teardown before ShutdownGame; exit callbacks and lifecycle state must establish that precondition.

## Logging, allocation failure and diagnostic exception

AppendToDebugFile (`0x49e640`) opens DEBUG.FIL for append, falling back to creating it, writes strlen(text) plus one bytes and closes it. Each record therefore includes a NUL terminator; it is not simply newline-separated text. It does not check that fallback creation succeeded before writing or that all requested bytes were written. The [HPI append forwarding declaration](util-hpi.md) is locally pointer-returning here despite its void definition elsewhere.

InstallOutOfMemoryHandler (`0x49e6f0`) registers the same OutOfMemoryHandler that GameMain installs directly. OutOfMemoryHandler (`0x49e700`) forms ErrorLog.txt beside the executable, or under `C:\` if it cannot find a path separator. It opens with OPEN_ALWAYS, seeks to the end, writes the fixed out-of-memory/disk-full message and closes.

Its handle test is nonzero rather than comparison with INVALID_HANDLE_VALUE, so an ordinary Windows invalid-file result can still enter the write block. Individual seek/write errors are ignored. It then reports through ReportViaException, shows the fatal message box, calls NopAbortHook, raises SIGABRT and calls `_exit(3)`. This is termination behavior, not a memory-recovery callback that returns reclaimed bytes. Ordinary application shutdown and registry restoration are not guaranteed on this path.

ReportViaException (`0x49e680`) deliberately causes an integer divide-by-zero inside structured exception handling so ReportException receives exception/register context plus the supplied message. Its local handler then handles that diagnostic exception. This is a reporting device; it does not mean every caller suffered an actual arithmetic bug. The out-of-memory caller proceeds to its explicit abort after reporting.

AtexitNoOp (`0x49ed90`) has an empty body. Its once-only registration does not free the Game object, restore shell settings or provide general crash cleanup. Allocation details and diagnostics belong to [allocation adapters](util-operator-new.md) and the separate debug modules.

## Completion and source-dependent boundaries

All eleven owned entries are covered, including complete option cases and their consumption differences. Actual command-line content, installed language/data, Windows access rights, messages, CD devices and failures determine which paths execute. No build, process launch, registration, Windows setting mutation or game run was used to prepare this source-based reference.
