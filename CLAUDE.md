# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

QRAT is a C++17 probabilistic reachability analysis tool for quantum programs. It parses a small imperative quantum while
programming language (`.qw` files), simulates program execution symbolically using decision diagrams via one of two
pluggable backends (MQT Core's floating-point package, or exact-dd's exact-arithmetic package), builds a state
space/transition graph, and can either search that graph directly or export it as a DTMC model to be verified by an
external probabilistic model checker (PRISM or Storm).

## Build

Requires a C++17 compiler, CMake >= 3.19/3.20, Flex 2.6.4, and Bison 3.8.2. Dependencies (MQT Core, exact-dd,
googletest) are git submodules under `extern/` — cloned automatically by `cmake/ExternalDependencies.cmake` if
missing (`git submodule update --init --recursive`).

```shell
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j 8
```

The resulting `qrat` executable is built at `build/src/qrat`.

Useful CMake options (see `cmake/ProjectSettings.cmake`): `BUILD_QRAT_TESTS` (default ON), `BUILD_QRAT_DOC`
(default OFF), `FORMAT_SOURCE_CODE` (default OFF, runs clang-format via `cmake/Format.cmake`), `CLANG_TIDY_ANALYSIS`
(default OFF, via `cmake/ClangTidy.cmake`).

## Test

Tests run the built `qrat` binary against example `.qw` programs and diff stdout against `.expected` files
(see `test/testScript.sh` and `test/test_programs.cpp`, driven by GoogleTest).

```shell
ctest --test-dir build                 # run all tests
ctest --test-dir build -R teleportProg # run a single test by name
ctest --test-dir build --output-on-failure
```

Test fixtures live in `test/<name>/<name>.qw` + `test/<name>/<name>.expected` (e.g. `test/teleport`, `test/grover`,
`test/loop`; the `*-exact` variants run the same programs under the exact backend via `set backend dw .` in
their `.qw` scripts). A few fixtures target specific exact-backend behavior rather than mirroring an existing
program: `test/seqmeas-exact` (consecutive measurements on unnormalized states), `test/unsupported-exact` (the
clear-error path for a gate the exact backend can't represent), and `test/mixed-backend` (switching backends
mid-script with `set backend mqt|dw .`, run without any fixed backend). To add a new regression test,
add a `test/<name>/` directory with a `.qw` program and `.expected` output, then add a corresponding
`TEST(ProgTest, ...)` case in `test/test_programs.cpp` calling `bash testScript.sh <name>` — any extra arguments
after the name are passed through to the `qrat` binary. Test directories are copied into
the build tree at configure time, so re-run the CMake configure step after adding or editing fixtures.

## Code style

- **Formatting**: enforced by `.clang-format` (LLVM base style, 4-space indent, no tabs, attached braces, no column
  limit, pointer alignment right e.g. `int *x`). Runs automatically at configure time when `FORMAT_SOURCE_CODE` is
  ON (`cmake/Format.cmake`).
- **Static analysis**: `.clang-tidy` checks run against the `qrat` and `Parser` targets when `CLANG_TIDY_ANALYSIS`
  is ON (`cmake/ClangTidy.cmake`).
- **Naming**: `PascalCase` for classes/types, one class per file with a matching filename (`SearchGraph.hpp`/
  `SearchGraph.cpp`); `camelCase` for methods and member variables (no `m_`/trailing-underscore prefix); `ALL_CAPS`
  for enum-style constants (e.g. `Search::Type::ARROW_EXCLAMATION`).
- **Headers**: classic `#ifndef`/`#define` include guards matching the file name (e.g. `SEARCHGRAPH_HPP`), not
  `#pragma once`.
- **Polymorphism**: base classes declare `virtual ~Base()` (usually `= default`); derived overrides use `override`
  (see `SearchGraph`, `Runner`, `StateTransitionGraph`).

## Install / package

```shell
cmake --install build --config Release
cpack -G "ZIP" --config build/CPackConfig.cmake -B package
```

Docker: `docker build --no-cache -t qrat .` builds and packages the tool inside `ubuntu:22.04`.

## Architecture

### Pipeline

1. **Parsing** (`src/parser/lexer.l`, `src/parser/parser.y`, built with Flex/Bison into `lexer.cpp`/`parser.cpp` in
   the build tree) — parses `.qw` source and interactive REPL commands into an AST.
2. **AST** (`src/ast/`, `include/ast/`) — statement/expression node hierarchy (`StmNode`, `ExpNode` base classes;
   `UnitaryStmNode`, `CondStmNode`, `WhileStmNode`, `AtomicStmNode`, `SkipStmNode`, `EndStmNode`, `MeasExpNode`,
   `PropExpNode`, `KetExpNode`, `QubitExpNode`, `CondExpNode`, `BoolExpNode`, `OpExpNode`, `NumExpNode`,
   `ConstExpNode`, `InitExpNode`, `CachedNode`, `StmSeq`). Statements form a linked list (`getNext()`), which is how
   the graph search walks the program counter.
3. **Interpreter** (`include/core/Interpreter.hpp`, `src/core/Interpreter.cpp`) — the central coordinator invoked
   from the Bison actions in `parser.y`. Owns the current `SyntaxProg`, the active simulation backend
   (`SimulationBase *ddSim`, constructed in `initDDSimulation()` via `SimulationFactory::create()`), and the active
   `SearchGraph`; dispatches `search`/`psearch`/`pcheck`/`show` commands. `initDDSimulation()` wraps the factory
   call in a `try`/`catch`: a backend that rejects the program at construction (see `DwSimulation` below) prints a
   red error and leaves `ddSim` null rather than crashing, and every command that depends on it
   (`execute()`/`executePCheck()`/`initializeSearch()`/`initializePCheck()`) already guards on that.
4. **Quantum simulation** — pluggable backend behind the abstract `SimulationBase`
   (`include/dd/SimulationBase.hpp`; concrete backends split into `include/dd/mqt/`/`src/dd/mqt/` and
   `include/dd/exact/`/`src/dd/exact/` — distinct from, but sharing the `dd/exact` path fragment with,
   `extern/exact-dd`'s own `dd::exact` namespace headers): gate application, measurement with probability, refcounting/GC, property
   projectors/tests, and display, all in terms of the backend-neutral state handle `QState` (named for
   "quantum state" to leave room for a future classical-state counterpart, `CState`, once the language grows
   persistent classical variables) (`include/dd/QState.hpp`, a `std::variant<qc::VectorDD, dd::exact::DwVEdge>`)
   and the probability type `Prob`
   (`include/utility/Prob.hpp`, a double plus optional exact `Dw`; `raw()` returns the plain double and throws if an
   exact value is attached, `approx()` always succeeds via `Dw::toComplexFloat()`, `exactValue()` promotes literal
   0/1 doubles to `Dw::zero()`/`Dw::one()` for the exact solver). `SimulationFactory::create()`
   (`include/dd/SimulationFactory.hpp`, `src/dd/SimulationFactory.cpp`) picks the concrete subclass from
   `Configuration::backend`, set via the in-language `set backend mqt|dw .` command (default `mqt`; re-read by
   `Interpreter::initDDSimulation()` on every `search`/`psearch`/`pcheck`, so a script can switch backends between
   commands; distinct from pcheck's `--backend=PRISM|Storm` runner argument). Two implementations:
   - `DDSimulation` (`include/dd/mqt/DDSimulation.hpp`) — the default, wraps MQT Core's floating-point
     `dd::Package<DDSimulationPackageConfig>`; full gate set incl. parametrized rotations. Gate names/macro
     (`DEFINE_SINGLE_TARGET_OPERATION`) live in this header.
   - `DwSimulation` (`include/dd/exact/DwSimulation.hpp`) — exact backend on exact-dd's `dd::exact::DwPackage`;
     amplitudes are exact elements of Q[ω]. Clifford+T only — `validateProgram()` rejects parametrized/unsupported
     gates at construction by throwing, which `initDDSimulation()` catches. Post-measurement states stay
     **unnormalized** (1/√p is not exact); `measureWithProb` returns exact conditional probabilities ⟨Pv|Pv⟩/⟨v|v⟩,
     dividing by the input state's squared norm at every measurement so consecutive measurements stay correct.
     `init[q]` projectors relocate the stored 1-qubit init state to the property's target qubit by rebuilding
     |v⟩⟨v| from its two exact amplitudes.
5. **Graph search** (`include/core/SearchGraph.hpp` and its two subclasses):
   - `StateTransitionGraph` (`search`/`psearch` commands) — explores reachable `(program counter, quantum state)`
     pairs up to a search type (`Search::Type`: `=>1`, `=>*`, `=>+`, `=>!`) and depth/solution bounds; in
     probabilistic mode (`psearch`) it solves for reachability probabilities using Gauss-Seidel or Jacobi iterative
     methods over the induced Markov chain (MQT backend), exact Gaussian elimination over the real subfield of
     Q[√2] (`exactGaussianElimination()`, exact backend — iterative methods cannot terminate exactly on irrational
     fixed points of cyclic chains), or backward-reachability for qualitative checks.
   - `StateSpaceGraph` (`pcheck` command) — builds the full reachable state space, tagging states by which named
     property they satisfy, for export to an external model checker.
   Both share `SearchGraph`'s state deduplication (`stateTab`, hashed by program counter + canonical DD node
   pointer via `QState::node()`; both backends hash-cons, so dedup-up-to-global-scalar carries over) and dispatch
   on statement type via `procSkipStm`/`procUnitaryStm`/`procCondStm`/`procWhileStm`/`procAtomicStm`.
6. **Model export & checking** (`src/model/`, `include/model/`) — `DTMC` serializes a `StateSpaceGraph` into a PRISM
   `.pm` model file with labeled properties; `Runner` is the abstract interface for invoking an external model
   checker binary as a subprocess and parsing its result, implemented by `PrismRunner` and `StormRunner`;
   `RunnerFactory` picks the backend from `pcheck` command-line-style arguments (`--backend=PRISM|Storm`,
   `--save-model=true|false`). Under the exact backend, transition probabilities are exported as 30-digit decimal
   approximations (PRISM syntax cannot express √2) with the last branch of each row written as the textual
   complement so rows sum to exactly 1; the `.pm` file carries a comment noting the approximation.

### Quantum while language (`.qw`)

QRAT's input language is a quantum extension of Dijkstra's guarded-command language (in the spirit of Ying's
"quantum while" language), where branching/looping guards are measurement outcomes rather than classical booleans.
Grammar in `src/parser/parser.y`, keywords/tokens in `src/parser/lexer.l`.

**Program structure**

```
prog <Name> is
var <id>, ... : qubit;      // startDecl
prop <name> := <property>;  // startProp, see below
init
    <var> := <expression>;  // initial value per declared var
begin
    <statements>
end
```

**Statements** (`stm` rule; one AST node class per kind, see `src/ast/`):
- `skip;` — no-op (`SkipStmNode`).
- `<GATE>[<qubits>];` or `<GATE>[<qubits>](<params>);` — unitary gate application (`UnitaryStmNode`, built by
  `DDOperation::makeOperation`). Gate names come from `DEFINE_SINGLE_TARGET_OPERATION` in
  `include/dd/mqt/DDSimulation.hpp` plus two-target/controlled (`C…`)/multi-controlled (`MC…`) variants;
  parametrized gates take 0–3 angle params (`pi`, `pi/2`, `pi/4`, `tau`, `e`, or a numeric literal). The exact
  backend only accepts the Clifford+T subset of this gate set (see `DwSimulation` above); anything else is
  rejected with a clear error rather than being silently approximated.
- `if M[q] = 0|1 then <stmts> else <stmts> fi;` — conditional on a measurement outcome (`CondStmNode`); the guard
  must be exactly `M[q] = 0` or `M[q] = 1` (`CondExpNode` + `MeasExpNode`), not an arbitrary boolean.
- `while M[q] = 0|1 do <stmts> od;` — loop on a measurement outcome (`WhileStmNode`); this measurement-driven guard
  is what makes the language "quantum while" rather than plain classical control flow, and is what the probabilistic
  search (`psearch`) solves reachability probabilities over.
- `atomic { <stmts> };` — groups statements into a single transition/state-graph step (`AtomicStmNode`).
  Conditional and loop statements are rejected inside `atomic` blocks (`AtomicStmNode::isValid()`, enforced at
  parse time) since atomicity only makes sense for a straight-line sequence of unitaries.

**Expressions:**
- Qubit bases: `|0>`, `|1>`, `|+>`, `|->`, `random` (`KetExpNode`), optionally scaled by a coefficient expression:
  `<expr> . <basis>` (`QubitExpNode`) — used in `init` assignments, e.g. `q0 := random;`, `q1 := |0>;`.
  Bell states `|phi+>`, `|phi->`, `|psi+>`, `|psi->` are available for use in properties (see below).
  `M[q]` (`MeasExpNode`) is a measurement expression, valid only as one side of `M[q] = 0|1` in a `condStm`/`loopStm` guard.
- Arithmetic over declared `const`s and numeric literals: `+ - * /`, unary `-`, parentheses (`OpExpNode`,
  `ConstExpNode`, `NumExpNode`).

**Properties** (named via `prop`, or written inline for `search`/`psearch`/reused by `pcheck`):
- `true` / `false` (`BoolExpNode`).
- `proj(q, <basis>)` — projector of qubit `q` onto a basis state, or `init[q]` for its declared initial state
  (`PropExpNode` / `InitExpNode`).
- `proj(q1, q2, <bellState>)` — two-qubit projector onto a Bell state.
- Boolean combinators `and`, `or`, `not` compose properties.

**Commands** (REPL / script-level, drive the `Interpreter`): `load <file> .`, `search [bound{, depth}] in <Prog>
with <arrow> such that <property> .` and `psearch ...` (qualitative vs. probabilistic reachability; `<arrow>` is
one of `=>1 =>* =>+ =>!`, see `Search::Type`), `pcheck in <Prog> with "<formula>" <args> .` (export to
PRISM/Storm), `show path/state <id> .`, `show prob|amp of basis '<bits>' in state <id> .`, `set show timing on|off
.`, `set random seed <n> .`, `set backend mqt|dw .` (selects the simulation backend for subsequent
commands; `backend` is a reserved keyword), `quit .`.

Example programs for all supported protocols (teleport, grover, loop, biteleport, entangleswap, networkcoding,
relay, secretsharing, wmgrover) live under `examples/`, each with a `.qw` source; the ones used in CTest also have
a matching `.expected` file under `test/`.

### External dependencies

- **MQT Core** (`extern/mqt-core`, fork at `canhminhdo/mqt-core`) — the default (`mqt`) backend's decision-diagram
  quantum computation backbone; version pinned via `MQT_CORE_VERSION` in `cmake/ExternalDependencies.cmake`.
- **exact-dd** (`extern/exact-dd`, `canhminhdo/exact-dd`) — the exact (`dw`) backend's decision-diagram package;
  linked as `ExactDD` into the `Parser` static library.
- **googletest** (`extern/googletest`) — only built when `BUILD_QRAT_TESTS` is on.

All three are git submodules; if a submodule directory is empty, run `git submodule update --init --recursive`
before configuring (or let `ExternalDependencies.cmake` attempt it automatically when Git is available).

### Build layout note

Almost all real logic (AST, core, dd, model, utility, and the AST-heavy parts of the parser) is compiled into a
static library target named `Parser` (see `src/parser/CMakeLists.txt`), not the `qrat` executable target itself.
`src/CMakeLists.txt` builds the `qrat` executable from just `main.cpp` + `utility/printUtils.cpp` +
`model/{DTMC,PrismRunner,Runner,StormRunner}.cpp` and links against `Parser`. When adding a new `.cpp` file to
`src/ast`, `src/core`, `src/dd`, or `src/utility` (other than `printUtils.cpp`), add it to the `Parser` library
sources list in `src/parser/CMakeLists.txt`, not to `src/CMakeLists.txt`.
