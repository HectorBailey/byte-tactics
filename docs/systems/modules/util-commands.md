# Command tokenization, substitution, registration and dispatch

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

## 1. Scope and shared state

The util/commands group owns 21 annotated entries in [commands.cpp](../../../src/util/commands.cpp), from `0x4b73b0` to before `0x4b7e60`. This includes two generated static-lifetime entries, the emitted vector insertion routine and CommandEntry copy/assignment support. It supplies textual command infrastructure used by console commands and AI definition expressions, not the complete behavior of every registered game command.

CommandArgs contains twenty token pointers, a 126-byte token-text array, two padding bytes and a count. The command registry is a static vector of 12-byte entries, each containing a [StringRef](graphics-surface.md) name handle, handler pointer and context mask. Separate globals hold one default handler and its mask. There is no per-player registry or independent process for each command.

## 2. Argument initialization and typed access

InitArgs (`0x4b73b0`) sets count zero and returns this. It does not zero pointers or the token buffer. GetArg (`0x4b73c0`) checks index against zero/count and returns the supplied default when out of range; otherwise its integer-shaped return carries the token pointer.

GetIntArg (`0x4b73e0`) and GetFloatArg (`0x4b7410`) use atoi/atof for an in-range token. Their caller default applies only when the index is missing, not when text is malformed. A present nonnumeric token can therefore convert to zero instead of the supplied fallback. No explicit overflow, full-token-consumption or locale-independent number validation is provided. Token pointers must still refer to valid terminated text.

## 3. Token grammar

Tokenize (`0x4b7440`) accepts a text pointer and optional exclusive endpoint. A null endpoint is replaced by text+strlen(text). It resets count and writes token bytes into its own buffer. Leading/inter-token isspace bytes are skipped. A hash at the next token boundary stops the parse; a hash inside a token ends that token and then stops on the next outer iteration. The hash and following text are not copied.

There is no quoting, backslash unescaping, semicolon statement separator or embedded-whitespace string syntax. A quoted word keeps quote characters as ordinary bytes. Newlines are whitespace for Tokenize when supplied as part of its range; ExecuteCommandText separately limits each parse to one line. An explicitly bounded range uses endpoint equality, not a general text<end check, so callers must supply a consistent forward range.

For the first twenty tokens, a pointer is stored and count incremented. Additional tokens are still consumed/copied but do not get argument slots. Every token adds a NUL terminator to the shared text buffer. The twenty-pointer limit is therefore not an independent guarantee that input fits the text capacity.

The capacity handling has a retained flaw: when the write pointer reaches the end of the 126-byte buffer, the inner copy loop breaks without advancing the source or returning. The outer loop still writes a terminator and increments the output pointer. If source text remains non-whitespace/non-comment, subsequent iterations can repeat without consuming it while writing beyond the buffer. Terminators themselves are not included in the inner character-capacity test. Overlong input is not safely accepted as a truncated command and can corrupt adjacent state or fail to make normal parsing progress.

## 4. Positional substitution and lifetime

SubstituteArgs (`0x4b74f0`) visits tokens whose first byte is percent. It atoi-converts the remaining text and, if that index is within another CommandArgs list, replaces the complete token pointer with that other list's pointer. Indexing is zero-based. An out-of-range reference is left unchanged.

This is whole-token pointer substitution, not string interpolation: a token with a numeric prefix followed by extra text still uses atoi's index, while a nonnumeric percent suffix can select index zero. There is no percent escape or text concatenation. Replacement strings are not copied into this command's buffer. The other argument list and referenced bytes must remain alive during handler use; a null vars pointer is not checked when a percent token needs it.

ShiftArgs (`0x4b7540`) has an inverted-looking retained condition. When count>=n it clears count and returns, including ordinary n values within the list and n zero. Only n>count reaches its copying loop, which starts at args[n] and increments toward an earlier end pointer; that is not a safe implementation of dropping the first n arguments. Negative n also takes the clearing branch under ordinary nonnegative count. The routine's name must not substitute for its literal behavior.

## 5. Registering named handlers

RegisterCommand (`0x4b7620`) copies the supplied name into StringRef, lower-bound searches the sorted vector using case-insensitive comparison and either reuses an exact-case matching element or inserts a new one at that position. It stores the handler pointer and mask in the selected entry. Inserted placeholder values are zero until that assignment. RegisterCommands (`0x4b7760`) repeats the same logic over a record array terminated by a null name.

The sort comparator ignores case, but equality for deciding reuse is plain case-sensitive strcmp. Registering differently cased variants can therefore insert multiple entries within the same case-insensitive equivalence group. Lower-bound lookup selects the first such entry. Even a previously registered spelling can be inserted again if another variant now occupies that first position. Registration does not normalize every key to lowercase or enforce one unique case-insensitive name.

There is no local check that the handler is nonnull, the mask is meaningful or the input array is correctly terminated. A mask-eligible null registered handler can reach an indirect null call. Registration can reallocate/reorder the vector, so pointers into its entries are not a stable external handle API.

[AI registration](ai-ai-player.md) adds plan, weight and limit with mask eight. [console_commands.cpp](../../../src/game/console_commands.cpp) provides the larger command table and script-file callers. Context masks choose applicable handler families; concrete command effects and any multiplayer authorization checks belong to those handlers.

## 6. Default handler and dispatch result

SetDefaultCommandHandler (`0x4b78e0`) stores the single callback/mask pair. ClearDefaultCommandHandler (`0x4b7590`) clears both. Neither owns a callback object, invokes its destructor or keeps an observer list.

ExecuteCommand (`0x4b7900`) returns zero for an empty argument list. Otherwise it copies token zero to a StringRef, makes that lookup key lowercase, and performs a case-insensitive lower-bound search. It does not rewrite the original token or remove it before calling the handler. The selected handler receives the original CommandArgs object with the command name still at index zero.

A found entry runs when storedMask & suppliedMask is nonzero. If no applicable registered entry exists, an installed default runs when its mask intersects instead. Thus a known but context-ineligible name can fall through to the default. The default does not run after a selected registered handler simply because that handler performed no visible effect.

Return value is the full mask of the handler chosen, not the mask intersection, callback return value, number of commands or a detailed success/failure report. Handlers have a void interface. A return can prove dispatch selection while the handler itself rejects its arguments or fails its underlying action.

## 7. Multi-line execution

FindLineEnd (`0x4b7a10`) scans up to the supplied signed length for newline and returns its zero-based position or length when none occurs. A negative length simply returns that negative value after no loop; the block executor calls it only while remaining length is positive.

ExecuteCommandText (`0x4b7a30`) initializes one stack CommandArgs, repeatedly finds the next line endpoint, tokenizes that slice, substitutes variables and ORs ExecuteCommand's result into an accumulated mask. It advances by line length plus one. A final line without newline still executes; pointer/remaining-length arithmetic then steps one past that final slice and ends the loop. Blank/hash-comment lines produce no dispatched handler.

Each next Tokenize reuses/overwrites the same local buffer. Handlers must not retain pointers into it after the call without copying. Supplied variable pointers have their own borrowed lifetime. There is no syntax-error status, per-line error collection, cancellation/rollback or automatic stop when a command returned no matching mask. Successful dispatch on one line does not mean every line ran.

## 8. Vector lifetime and reference-counted entries

The generated initializer (`0x4b75a0`) constructs the static vector; its atexit destructor (`0x4b75d0`) destroys entries and storage. ClearCommandTable (`0x4b7ad0`) destroys all active entries through vector::clear while retaining allocated vector capacity. It does not also clear the separate default callback/mask.

CommandEntry's copy constructor (`0x4b7e30`) copies the StringRef handle and two integers. AssignEntry (`0x4b7e00`) delegates name assignment to StringRef::Assign and copies the handler/mask. Destruction releases the handle through StringRef's destructor; adding a separate manual release would be a second release, not required cleanup in this module.

The emitted vector insert at `0x4b7b00` was traced in the existing MSVC 5 SP3 VECTOR header. When capacity is insufficient it allocates oldSize+max(oldSize,insertCount), copies the prefix, constructs inserted entries, copies the suffix, destroys old entries and frees old storage. With room, it constructs the tail and shifts/assigns the affected range. These copies/assignments retain/release the name's shared storage through the methods above. The registry saves an element index across insertion and recomputes its pointer after possible reallocation.

These generated support bodies do not make command execution transactional or thread-safe across registry mutation. No outer synchronization protects a handler retaining a pointer into a table that another callback changes. The existing library header was read only; no compiler was run to verify emitted code.

## 9. Completion and interactions

NopRet (`0x4b7ac0`) is an empty function with no hidden dispatch work. All 21 entries, generated storage support and AI/console consumers were inspected. Name references and argument buffers have different ownership: entries own shared name handles, while CommandArgs tokens are local or borrowed substitutions. No argument-list save record or network command packet is implemented here.

Unit attacks, building, targeting and cheats are effects of registered handlers and their gameplay consumers, not the tokenizer itself. This reference accounts for parser limits, case/mask rules and retained ShiftArgs/overflow behavior without copying source listings or running builds, installing dependencies or executing commands in the game.
