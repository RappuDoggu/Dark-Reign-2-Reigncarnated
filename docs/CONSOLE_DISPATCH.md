# Original console command dispatch

`prepare_console.py` verifies the external console.cpp hash and stages the
original CmdParse parser, typed argument readers and ProcessCmd dispatcher.
The target links existing original TBuf, StdParse, VNode and VarSys code.
Menu commands therefore reach registered VarSys command handlers; there is no
successful no-op command implementation.

`reign::run_console` requires a display callback. Original variable-display calls
pass their actual VarItem and indentation to that callback. The window/history
console renderer and general Console::Message services remain outside this
slice. Callers must keep TrackSys/VarSys initialized throughout dispatch and use
one UI thread; thread-local parser state does not make global VarSys concurrent.

Changes at native boundaries:

- Scoped binding is required before parsing. Null or over-64KiB commands fail.
- Original nesting state becomes thread-local and is capped at 32 levels.
- Parent command/handler state is restored after nested commands and exceptions.
- Argument scopes are deleted when each parser leaves, including failures.
- Temporary parsed VNodes have exception-safe ownership.
- Parse errors propagate as exceptions instead of being swallowed as integer
  exceptions. Commands before a failure may already have executed; no rollback
  is implied.
- RAWDATA offsets use the tokenizer's actual position. The original code mixed
  `argCount++` and reading argCount across function arguments; this did not
  reliably describe the raw argument's byte offset.
- Float-to-integer assignment rejects nonfinite/out-of-range values. The signed
  argument reader uses a checked signed conversion, avoiding the original U32
  intermediate for negative floats.

Host focused tests exercise registered handler dispatch, assignments, argument
substitution, RAWVAR/RAWDATA, nested command state, negative float conversion,
error cleanup and continued dispatch after failures. An initial test mistakenly
classified `@` as invalid and used scientific notation to force a float; source
inspection showed these were incorrect test assumptions. Tests now use the
original parser's punctuation and decimal-float syntax. The first compile also
caught a staging selector matching a declaration rather than a definition; that
selector now skips declarations. Logs retain the actual compile failure.

These are synthetic commands, not GameGod/Main/Shell handlers. Running the real
menu configuration still needs those handlers, live controls and lifecycle.
