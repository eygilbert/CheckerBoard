# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

CheckerBoard, a Windows GUI for playing and analyzing checkers. It plays English and Italian rules, and it loads engines as DLLs through its own protocol. Kingsrow is its main engine. The code is C style with some C++ features, and it uses the Win32 API directly. The `.c` files are compiled as C++. The version is `VERSION` in `CheckerBoard.h`.

The user is Ed Gilbert, the author of Kingsrow and a checkers programmer for decades. Martin Fierz wrote the original CheckerBoard code. Ed maintains it, and he started this repo.

## Building

Use MSBuild from the **PowerShell** tool. Running MSBuild from Git Bash can fail with "Item has already been added. Key in dictionary: 'HOME'", because the environment has both `HOME` and `home`.

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" checkerboard.vcxproj -p:Configuration=Release -p:Platform=x64 -m -v:minimal -clp:ErrorsOnly -nologo
```

- The solution is `checkerboard.sln`, with Visual Studio 2026 and platform toolset v145. Build both `x64` and `Win32` before a merge.
- The programs are written to the repo root: `checkerboard64.exe` (Release x64), `checkerboard.exe` (Release Win32), and `checkerboard64_dbg.exe` and `checkerboard_dbg.exe` for Debug.
- Other projects in the solution: `source\simplech.vcxproj` and `source\dama.vcxproj`, sample engines, and `Perft` and `Perft-Italian`.
- `setup\CheckerBoard.iss` and `setup\CheckerBoard64.iss` are the Inno Setup installer scripts.

## Running it

CheckerBoard loads engines from its `engines\` folder (`CheckerBoard.c`, `engines\%s`). That folder is git-ignored, and so are `egdb64.dll` and the executables. A fresh clone therefore has no engines. To test with Kingsrow, copy these from a Kingsrow build:

- the engine DLL and its weights files into `engines\`;
- `egdb64.dll` (x64) or `egdb.dll` (Win32) into the repo root, next to the executable.

An endgame database is also needed to test anything that uses one.

## Testing

There is no unit-test framework. Plan testing in three parts:

- **Non-GUI logic** (move generation, referee, PDN and FEN, the engine interface): Claude tests it with small console programs that link the CheckerBoard sources.
- **Menu-driven actions:** these can be scripted by sending `WM_COMMAND` with the menu IDs to the CheckerBoard window.
- **Mouse, dialog and visual behavior:** Ed tests these by hand, from a short checklist that Claude writes. Use desktop control only if Ed asks for it, because it works on his desktop.

Warn Ed just before opening a window on his desktop. Test with Release builds unless a Debug build is the point of the test: a failing assert pops up a dialog on his desktop.

## Git workflow

The repo's main branch is `master`, on GitHub at https://github.com/eygilbert/CheckerBoard. Ed uses TortoiseGit, not the git command line. He has created several repos on GitHub and knows the basic commands casually, but he isn't proficient, and after years of Subversion the differences between the two systems can confuse him. Explain git steps in plain terms, especially where git differs from Subversion.

- Work on one branch per fix, never directly on `master`.
- Don't commit, merge, or push without Ed's go-ahead.
- After Ed has tested, commit only the files that belong to the fix. Then fast-forward merge into `master`, push to `origin`, and delete the branch, here and on GitHub.
- Older branches are on GitHub. Don't assume they are merged, and don't delete them without asking.
- Line endings: the repo stores LF. `core.autocrlf=true` makes git write CRLF on checkout. When editing with scripts, keep each file's existing line endings, and check bytes with Python if it matters.

## Code style

Match the existing C style. Don't do C++ rewrites unless the existing code is actually broken.

## Architecture

- `CheckerBoard.c`, `CheckerBoard.h`: the main window, menu commands, engine loading, the game and engine-match logic, and the time control.
- `dialogs.c`: the dialogs. `graphics.c`, `bmp.c`: board drawing and animation. `checkerboard.rc`, `resource.h`: resources and menu IDs.
- `CB_movegen.c`, `bitboard.c`, `lsb.h`: move generation.
- `utility.c`: moves, draw detection, sending the game history to engines, and helpers.
- `PDNparser.c`, `PDNfind.c`, `fen.c`, `coordinates.c`: PDN and FEN, and square numbering.
- `registry.c`: saved settings. `saveashtml.c`: export to HTML. `app_instance.cpp`: numbering of running instances.
- `CBstructs.h`, `CBconsts.h`: shared types and constants.
- Documentation: `cb_api_reference.htm` is the engine interface reference, and `help.htm`, `helpspanish.htm`, `nutshell.htm` are the user help. `source\cb_interface.h` has the interface declarations for engines.

## Keep these in step with Kingsrow

The Kingsrow repo (https://github.com/eygilbert/Kingsrow, normally `D:\Projects\git\kingsrow`) has `kr_cli`, a command-line tool that plays engine matches and tests engines. It copies CheckerBoard's move generation and referee code, so that its match results mean the same as CheckerBoard's. Its logic must stay identical to CheckerBoard's. Each copied file starts with a comment naming its CheckerBoard source.

| CheckerBoard source | Copied into `kr_cli` for |
|---|---|
| `CB_movegen.c`, `CB_movegen.h` | move generation and legality |
| `utility.c`: `domove`, `undomove`, `is_conversion_move`, `detect_nonconversion_draws`, `send_game_history` | playing moves, draws, the game history |
| `fen.c`, `fen.h` | FEN to board and back |
| `PDNparser.c`, `PDNparser.h`, parts of `PDNmain.c` | reading ballot files, writing games |
| `coordinates.c`, `coordinates.h` | square numbering |
| `CheckerBoard.c`: `maxtime_for_non_incremental_tc`, `maxtime_for_incremental_tc`, `format_time_args` | time control |
| `CBconsts.h`, parts of `CBstructs.h`, `cb_interface.h` | protocol types and constants |

`doc/kr_cli.md` in the Kingsrow repo has the details. When a change here touches any of these, or changes the protocol or how engines are called, tell Ed and raise the matching `kr_cli` change. Protocol changes must keep older engines working.

## Planned work: bug review

Ed plans a review of the CheckerBoard code for bugs. It is code reading, followed by the testing split above. Present the findings for Ed to decide on, and fix them one branch at a time.

## Working with Ed

- Conversations scroll off, so anything he needs later should be written to a file.
- He prefers one thing at a time to several in parallel.
- Kingsrow engine matches run on another machine, ed-wkstn, and CheckerBoard work shouldn't disturb them.
