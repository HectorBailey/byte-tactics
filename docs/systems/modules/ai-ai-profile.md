# AI profile tokenization and definition protection wrappers

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `ai/ai_profile`, nine functions from `0x428c60` through `0x428fe0` in [ai_profile.cpp](../../../src/ai/ai_profile.cpp). This module provides the tokenizer used by AI configuration parsing and two unit-definition memory-protection adapters. It does not by itself choose construction priorities or run squads.

## 1. Parser state and buffer ownership

AIProfileParser stores a 128-byte token buffer, input begin/end pointers, cursor, sticky error flag, pending-token flag and cached token type. Construction (`0x428c60`) sets input pointers null, clears error/pending and first token byte, and initializes type to 0x102.

SetBuffer (`0x428c90`) retains a caller-owned buffer and size, sets end to begin plus size and resets cursor/token/error state. It neither copies nor frees the input. The parser has no owned-buffer teardown here. A source buffer must remain readable while tokenization runs; several scanner expressions dereference before checking end, so a merely mathematical end pointer is not sufficient defensive protection against an unreadable boundary.

NextToken (`0x428cd0`) returns 0x102 whenever the sticky error flag is set. A pending token is returned once from the cached type and clears pending without scanning. Otherwise it scans and caches the returned type. There is no separate pushback method here; the pending flag is parser state exposed to the higher-level grammar's use.

## 2. Token kinds and priority

| Return | Meaning in these methods |
| --- | --- |
| 0 | End pointer or NUL reached |
| Character value | Punctuation, consumed one character at a time |
| 0x100 | Alphanumeric identifier |
| 0x101 | Number-like token beginning with a digit |
| 0x102 | Error/no-valid-token sentinel |

ScanToken (`0x428d10`) first skips whitespace, then checks end/NUL, then returns punctuation **before** its numeric/identifier branch. Although later source conditions mention minus and period as possible number starters, ordinary punctuation classification consumes them earlier. A negative number therefore produces a minus-character token followed by a number token; a leading decimal point likewise does not begin one 0x101 token through that later condition.

Identifiers consume alphanumeric characters only. Underscores, quotes, slashes, braces and other punctuation are separate tokens, not embedded identifier characters or a comment/string-literal language implemented here. Identifier length must be nonzero and **less than 127** to return 0x100. Reaching 127 writes the terminator and returns 0x102, leaving later input unconsumed. That scanner return alone does not set the sticky error flag; the higher-level parser can report/latch it.

Number scanning copies the initial digit and then consumes digits or periods. Thus a token such as 12.5 is accepted as number-like, and even repeated internal periods are not rejected by a numeric grammar in this function. The loop has no token-buffer length bound and no explicit end-pointer test for each continuation. Long numeric strings can overflow its 128-byte token storage; valid bounded input is an original implementation precondition.

The scanner uses C character-classification functions directly on input chars. It does not implement Unicode normalization, escape processing or a general syntax validator. EOF and an error sentinel are different return values, even though callers may stop on either.

## 3. Integer reads and comparisons

ReadInt (`0x428e90`) uses the same sticky-error/pending-token/scan priority. It requires type 0x101. Any other type formats an error locally once, sets the sticky flag and returns zero. A legitimate numeric zero and an invalid read therefore have the same integer return; error state distinguishes them.

A valid number-like token is converted with atoi. A decimal token is truncated at its first noninteger character according to atoi behavior; this is not a float parser. A minus token fails the immediate integer read unless a higher-level grammar handles sign separately. The method does not independently report range overflow through a structured error value.

TokenEquals (`0x428f40`) compares the current token text case-insensitively to the supplied string. It does not fetch a new token or verify the current type first. Because punctuation returns its character without necessarily replacing token text, callers must use the token-type contract rather than comparing stale token text indiscriminately.

## 4. Error reporting behavior

ReportParseError (`0x428f60`) formats a local message containing the mission's AI-profile name slot 7 and supplied explanation when not already failed, then sets errorReported. The output call is absent: no console, dialog or log write occurs here. ReadInt's error path uses the same local-only pattern with expecting-int text.

The format ends with the literal last-string label but has no further conversion placeholder for the extra parser pointer supplied as an argument. It does not actually append the token text through that unused argument. Both paths use an unbounded sprintf into a 256-byte local buffer, so very long profile names/explanations are not made safe by the lack of visible output.

Once errorReported is set, NextToken and ScanToken keep returning 0x102 and ReadInt returns zero. SetBuffer resets that failure state. Repeated ReportParseError calls do not reformat the local message.

## 5. Definition protection adapters

ProtectUnitDefsReadOnly (`0x428fc0`) passes the game's unitDefs allocation to ProtectBlockReadOnly. ProtectUnitDefsReadWrite (`0x428fe0`) passes the same allocation to ProtectBlockReadWrite. These wrappers do not copy definitions, change a specific AI weight or implement parser rollback. Actual operating-system page protection and block metadata belong to the called memory services.

They are documented as source behavior only; the documentation process does not invoke them or mutate page protections. There is no save or network format in this module. AI profile commands, category resolution, weights, limits and plan/squad semantics are owned by the [AI player implementation](../../../src/ai/ai_player.cpp), which consumes this tokenizer.

All nine functions, lexical edge cases, sticky-error handling, caller-owned storage and both protection wrappers are covered through static source inspection. No builds, dependency installation or game execution were used.
