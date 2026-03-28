# Proof Verification for CryptoMiniSat XOR/FRAT Proofs

## Overview

CryptoMiniSat can emit a proof in FRAT format. The proof is then elaborated into
XLRUP format and checked by `cake_xlrup`. Both tools come from the same repository.

The pipeline is:

```
CryptoMiniSat  →  .frat file  →  frat-xor (elab)  →  .xlrup file  →  cake_xlrup
```


## Step 0: Build the tools

Both `frat-xor` and `cake_xlrup` live in the **`main` branch** of:

    https://github.com/meelgroup/frat-xor

Clone and build both:

```bash
git clone https://github.com/meelgroup/frat-xor
cd frat-xor
# build frat-xor
cargo build --release
cp target/release/frat-xor .

# build cake_xlrup
cd cake_xlrup
make
```

Then symlink both binaries into the directory where your CNF file lives (or wherever
you run verification from):

```bash
ln -s /path/to/frat-xor/frat-xor        ./frat-xor
ln -s /path/to/frat-xor/cake_xlrup/cake_xlrup  ./cake_xlrup
```


## Step 1: Run CryptoMiniSat

CryptoMiniSat takes **two file arguments**: the input CNF and the output proof file.

```bash
cryptominisat5 [options] input.cnf proof.frat
```

The proof file will be written in FRAT format and will contain comment lines
starting with `c` interspersed with the proof steps.

## Step 2: Filter the FRAT file (in case of DEBUG_FRAT)

`frat-xor` cannot handle the `c ...` comment lines that CryptoMiniSat writes in
case DEBUG_FRAT is enabled. Strip them before passing the file to `frat-xor`:

```bash
grep -v "^c" proof.frat > proof_clean.frat
```

## Step 3: Elaborate with frat-xor

`frat-xor` takes the cleaned FRAT file, the original CNF, and an output path for
the elaborated XLRUP proof:

```bash
./frat-xor elab proof_clean.frat input.cnf proof.xlrup
```

## Step 4: Check with cake_xlrup

```bash
./cake_xlrup input.cnf proof.xlrup
```

`cake_xlrup` will print `s VERIFIED` on success, or report the line number and
reason for failure.

## Full example

```bash
CNF=out/fuzzTest_40.cnf
FRAT=/tmp/proof.frat
CLEAN=/tmp/proof_clean.frat
XLRUP=/tmp/proof.xlrup

cryptominisat5 --zero-exit-status [options] "$CNF" "$FRAT"
grep -v "^c" "$FRAT" > "$CLEAN"
./frat-xor elab "$CLEAN" "$CNF" "$XLRUP"
./cake_xlrup "$CNF" "$XLRUP"
```

## Debugging a verification failure

### Enable DEBUG_FRAT

When `cake_xlrup` or `frat-xor` fails on a specific line, the first step is to
find out which function in CryptoMiniSat wrote the FRAT entry responsible for
that line.

Enable `DEBUG_FRAT` in `src/constants.h`:

```c
/* before: */
/* #define DEBUG_FRAT */

/* after: */
#define DEBUG_FRAT
```

Rebuild CryptoMiniSat. With `DEBUG_FRAT` enabled, every write to the FRAT file
is bracketed by comment lines of the form:

```
c void ClassName::functionName(...) start
<frat proof steps>
c void ClassName::functionName(...) end
```

These comments are filtered out by the `grep -v "^c"` step, so they do not
affect `frat-xor` or `cake_xlrup` — they are purely for human inspection of the
raw FRAT file.

### Find the offending function

Say `cake_xlrup` fails at line N of the XLRUP file. To trace this back:

1. Run the pipeline keeping the raw (unfiltered) FRAT at a fixed path:

```bash
CNF=myinput.cnf
FRAT=/tmp/frat_debug.frat
CLEAN=/tmp/clean_debug.cnf
XLRUP=/tmp/xlrup_debug.xlrup

../../build/cryptominisat5 --zero-exit-status [options] "$CNF" "$FRAT"
grep -v "^c" "$FRAT" > "$CLEAN"
./frat-xor elab "$CLEAN" "$CNF" "$XLRUP"
./cake_xlrup "$CNF" "$XLRUP"
```

The raw FRAT at `$FRAT` is kept unfiltered and contains all the `c ... start/end`
comments from DEBUG_FRAT for inspection.

2. Find the line in the XLRUP that fails, e.g. line 2557:

```bash
sed -n '2555,2560p' /tmp/xlrup_debug.xlrup
```

3. Identify the corresponding entry in the clean FRAT. The XLRUP is generated
   by `frat-xor elab` from the clean FRAT, so look for the clause ID or
   literals that appear in the failing XLRUP line:

```bash
grep -n "a x 47\|i 12918\|d x 47" /tmp/clean_debug.cnf
```

4. Search the **raw** FRAT for the same content and look at the surrounding
   `c ... start` / `c ... end` comment lines to identify the function:

```bash
grep -n "a x 47\|i 12918\|d x 47\|^c " /tmp/frat_debug.frat | grep -A2 -B2 "a x 47\|i 12918"
```

The `c void SomeClass::someFunction() start` line immediately before the
offending FRAT entry is the function responsible.
