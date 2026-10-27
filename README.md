<p align="center">
  <img src="docs/logo.png" alt="ttc2sfnt" width="420">
</p>

# ttc2sfnt

[![CI](https://github.com/xiaohunqupo/ttc2sfnt/actions/workflows/ci.yml/badge.svg)](https://github.com/xiaohunqupo/ttc2sfnt/actions/workflows/ci.yml)

English ｜ [简体中文](README.zh-CN.md)

Split a TrueType Collection (`.ttc`) into standalone `.ttf` / `.otf` fonts. C++11, no third-party
dependencies, header-only by default (same directory and build conventions as
[shx_tools](https://github.com/xiaohunqupo/shx_tools)).

Version 0.4.1 · [Changelog](CHANGELOG.md) · [`.ttc` format notes](docs/ttc_format.md)

> **Renamed from `ttc_tools`.** The project used to share its name with an unrelated TTC conversion
> library, which made it easy to install or link the wrong one, so every public identifier moved to
> `ttc2sfnt`: header directory `inc/ttc_tools/` → `inc/ttc2sfnt/`, umbrella header `ttc_tools.h` →
> `ttc2sfnt.h`, namespace `ttc_tools` → `ttc2sfnt`, macro prefix `TTC_TOOLS_*` → `TTC2SFNT_*`. The
> old names are not kept as aliases.

## Contents

- [Why this library exists](#why-this-library-exists)
- [Repository layout](#repository-layout)
- [Requirements](#requirements)
- [Building](#building)
- [Integration](#integration)
- [Quick start](#quick-start)
- [Command line](#command-line)
- [Consumers that write through const](#consumers-that-write-through-const)
- [API](#api)
- [Error codes](#error-codes)
- [Output file naming](#output-file-naming)
- [Validation and robustness](#validation-and-robustness)
- [Performance](#performance)
- [Known limitations](#known-limitations)
- [Tests](#tests)
- [Relationship to shx_tools](#relationship-to-shx_tools)
- [License](#license)

## Why this library exists

A `.ttc` packs several faces into one file and lets them share table data. Nearly every Chinese font
shipped with Windows is one: `simsun.ttc`, `msyh.ttc`, `msjh.ttc`, `msgothic.ttc`, `mingliub.ttc`…

Most font consumers accept a single face only. The classic case is
[ttf2mesh](https://github.com/fetisov/ttf2mesh) (TrueType outline triangulation): hand
`simsun.ttc` straight to `ttf_load_from_file()` and it returns `TTF_ERR_FMT`, because it parses an
sfnt header and knows nothing about the `ttcf` header of a collection.

`ttc2sfnt` is that one adapter step: read the collection → rebuild every face as a standalone sfnt
(rewrite the table directory, recompute the table checksums, fix `head.checkSumAdjustment`) → hand
it back in memory or write it to disk. Together with `shx_tools` (`.shx` → outlines / fonts) and
`ttf2mesh` (`.ttf` → 2D/3D mesh) it forms a self-contained "CAD drawing text → geometry / mesh"
chain:

```
.shx  --shx_tools-->  outlines / .ttf / .otf ─┐
                                              ├─ ttf2mesh ─> 2D / 3D mesh
.ttc  --ttc2sfnt-->   .ttf / .otf ────────────┘
```

## Repository layout

| Path | Contents |
|---|---|
| `inc/ttc2sfnt/` | Public headers, one file per topic. The version macros are maintained by hand in `ttc2sfnt.h`; `config.h.in` carries only the `TTC2SFNT_HEAD_ONLY` switch (CMake configures it into `<build>/generated/ttc2sfnt/config.h`). |
| `src/ttc2sfnt/` | Implementation, one `.cpp` per header, plus the internal `common.h` (byte order, limits, checksum math). |
| `sample/` | `ttcsplit` (split / list faces) and `ttcdump` (print table directories, verify checksums). |
| `test/` | Single-file regression test with synthetic fixtures. |
| `docs/` | `.ttc` byte layout and the splitting algorithm (currently in Chinese), plus `logo.png`. |
| `fonts/` | Notes only; real fonts stay out of the repository (see below). |

The headers mirror the call hierarchy:

```
ttc2sfnt.h  ─┬─ version macros TTC2SFNT_VERSION_* (maintained by hand)
              ├─ config.h        (generated; only TTC2SFNT_HEAD_ONLY)
              ├─ ttc_error.h     error_code / error_info / error_name
              ├─ ttc_file.h      whole-file I/O, directory creation, UTF-8 paths
              ├─ ttc_sfnt.h      Sfnt: one face's table directory + standalone rebuild
              ├─ ttc_name.h      name table decoding (UTF-16BE / Latin-1), file name rules
              ├─ ttc_collection.h Collection: collection header, ListTTC / UnzipTTC / UnzipTTCFace
              └─ ttc_extract.h   UnzipTTCToDir / UnzipTTCFaceToFile
```

## Requirements

- A C++11 compiler. Everything builds warning-free with MSVC at `/W4` (x64 and x86) and with
  GCC/Clang at `-Wall -Wextra`, in both header-only and static-library mode.
- CMake 3.12 or newer when you use the CMake build (optional — the headers can be used directly).

## Building

```bash
cmake -S . -B build            # header only by default
cmake --build build
ctest --test-dir build --output-on-failure
```

| CMake option | Default | Meaning |
|---|---|---|
| `TTC2SFNT_HEAD_ONLY` | `ON` | The headers `#include` the matching `.cpp` at the end and every function is `TTC2SFNT_HEAD_ONLY_INLINE`. The library is an INTERFACE target, so both `inc` and `src` must be on the include path. |
| `TTC2SFNT_HEAD_ONLY=OFF` | | `src/*.cpp` is compiled into a static library; consumers only need `inc`. |
| `TTC2SFNT_BUILD_TESTS` | `ON` | Build `ttc_tests` and register it with CTest. |
| `TTC2SFNT_BUILD_SAMPLES` | `ON` | Build `ttcsplit` and `ttcdump`. |

**The version number is maintained by the project itself**: `TTC2SFNT_VERSION_MAJOR` / `MINOR` /
`PATCH` / `STRING` sit at the top of [`inc/ttc2sfnt/ttc2sfnt.h`](inc/ttc2sfnt/ttc2sfnt.h). Changing
the version means editing that one place (and adding a `CHANGELOG.md` entry); the build system only
reads the macros and never configures them into a generated header.

`config.h` carries the build mode only and is not part of the source tree: `CMakeLists.txt`
configures `#cmakedefine TTC2SFNT_HEAD_ONLY` from `inc/ttc2sfnt/config.h.in` into
`<build>/generated/ttc2sfnt/config.h`, which is why the build directory has to come before `inc` on
the include path. Without CMake, copy `config.h.in` to `inc/ttc2sfnt/config.h` and follow the
comments there to decide whether to `#define TTC2SFNT_HEAD_ONLY`.

## Integration

| Approach | How |
|---|---|
| CMake subproject | `add_subdirectory(ttc2sfnt)`, then `target_link_libraries(app PRIVATE ttc2sfnt::ttc2sfnt)` |
| Manual copy (header only) | Copy `inc/ttc2sfnt/` + `src/ttc2sfnt/*.cpp` + the generated `config.h`; put both directories on the include path |
| Manual copy (compile to a library) | Copy `inc/ttc2sfnt/` only and add `src/ttc2sfnt/*.cpp` to your project |

## Quick start

```cpp
#include "ttc2sfnt/ttc2sfnt.h"

ttc2sfnt::error_info error;

// 1. Look at the faces first
std::vector<ttc2sfnt::Face> faces;
if (ttc2sfnt::ListTTC("C:/Windows/Fonts/simsun.ttc", faces, &error))
{
    for (std::size_t i = 0; i < faces.size(); ++i)
        std::printf("[%zu] %s %s, %u glyphs\n", i,
            faces[i].family.c_str(), faces[i].subfamily.c_str(), faces[i].num_glyphs);
}

// 2. Split every face into a directory (created when missing, files named after the family)
std::vector<std::string> written;
ttc2sfnt::UnzipTTCToDir("C:/Windows/Fonts/simsun.ttc", "out", "Face", &written, &error);
// -> out/SimSun.ttf, out/NSimSun.ttf

// 3. Extract a single face into memory and hand it straight to ttf2mesh
ttc2sfnt::TTC::TTF face;
if (ttc2sfnt::UnzipTTCFace("C:/Windows/Fonts/simsun.ttc", 0, face, nullptr, &error))
    ttf_load_from_mem(face.data(), (int)face.size(), &ttf, false);
```

When you need finer control, use the classes:

```cpp
ttc2sfnt::Collection collection;
collection.parse(bytes, size, &error);          // collection header only
ttc2sfnt::Sfnt sfnt;
collection.sfnt_of(0, sfnt, &error);            // table directory of face 0 only
const ttc2sfnt::Table_Record* head = sfnt.find(ttc2sfnt::Sfnt::tag_head);
for (std::size_t i = 0; i < sfnt.table_count(); ++i)
    std::printf("%s %u bytes\n",
        ttc2sfnt::Sfnt::tag_string(sfnt.table(i).tag).c_str(),
        (unsigned int)sfnt.table(i).length);
```

## Command line

```
ttcsplit <input.ttc> [output_dir]       # split every face into <family>.ttf / .otf
ttcsplit -l <input.ttc>                 # list the faces
ttcsplit -x <input.ttc> <index> <out>   # split a single face
ttcsplit -v                             # print the version
ttcsplit -h                             # print the help
ttcdump  <input.ttc> [...]              # print table directories and verify checksums;
                                        # the exit code is the number of problems found
```

The output directory defaults to the directory of the input file and is created when missing.
Results go to stdout, problems to stderr.

## Consumers that write through const

`ttf2mesh` 1.6 declares `ttf_load_from_mem(const uint8_t*, int, ttf_t**, bool)`, but while parsing
it converts the table directory and several header structures to host byte order **in place**
(`conv32(rec->checkSum/offset/length)`, plus the `hhea`, `cmap`, `glyf` headers, …). Measured on a
little-endian machine:

| Observation | Data |
|---|---|
| Table record 0's `checkSum` / `offset` / `length` | all three fields **byte-reversed** (`0E 30 4C 68` → `68 4C 30 0E`) |
| Bytes rewritten outside the directory | cambria **49,254**, mingliub **197,404** |
| Result | mixed endianness: neither valid big-endian nor self-consistent little-endian |

**This cannot be repaired.** Which fields got swapped depends on the consumer's parse path, so it
cannot be inverted, and recomputing checksums will not bring the font back. That leaves two
options:

```cpp
// Option A (recommended): hand it a throwaway copy and keep the face intact
const int rc = ttc2sfnt::with_scratch_copy(face, [&](const std::uint8_t* data, std::size_t size)
{
    return ttf_load_from_mem(data, (int)size, &ttf, false);
});
// `face` is still byte-for-byte what it was, so it can still be written to disk or used elsewhere
```

```cpp
// Option B: treat the buffer you handed over as consumed, and re-extract when you need it again
ttf_load_from_mem(face.data(), (int)face.size(), &ttf, false);   // face is damaged now
ttc2sfnt::UnzipTTCFace(path, index, face, nullptr, &error);      // take a fresh copy
```

To keep this boundary honest over time (in a debug build or a self test, say), assert with
`Sfnt::verify()` — one pass tells you whether the buffer is still a self-consistent sfnt:

```cpp
ttc2sfnt::Sfnt_Report report;
if (!ttc2sfnt::Sfnt::verify(face.data(), face.size(), &report))
    std::fprintf(stderr, "face is damaged: %s\n", report.first_problem.c_str());
// after ttf2mesh had its way with it:
// table 'EBDT' at offset 0x5C010000 length 3965059072 runs past the end of the font
```

## API

| Function | Purpose |
|---|---|
| `ListTTC(file/data, faces, error)` | Reads the table directory and `name`/`maxp` only; copies no data |
| `UnzipTTC(file/data, ttc, error)` | Rebuilds every face into memory: `ttc.ttfs[i]` is a complete sfnt, `ttc.faces[i]` its metadata |
| `UnzipTTCFace(file/data, index, ttf, face, error)` | Rebuilds a single face (memory-friendly for large collections) |
| `UnzipTTCToDir(file/data, dir, prefix, written, error)` | Writes every face to disk and returns the paths written |
| `UnzipTTCFaceToFile(file, index, path, face, error)` | Writes one face to an explicit path |
| `Collection` / `Sfnt` | Two-step parsing: the collection header only, then one face's table directory only |
| `Sfnt::checksum()` / `Sfnt::tag_string()` | Checksum math and printable tags — this is how `ttcdump` is written |
| `Sfnt::verify(face, size, report)` | Validates a standalone font: every table checksum, 4-byte alignment, `head` magic, whole-file `0xB1B0AFBA`; `Sfnt_Report` carries the counters and the first problem |
| `with_scratch_copy(bytes, consumer)` | Hands a throwaway copy to a consumer that writes through `const` (`ttf2mesh` does); the caller's bytes stay untouched |
| `file_stem_for()` / `sanitize_file_name()` | The output file naming rules, usable on their own |

Failures return `false` and write the reason into an optional `error_info` (`code` is one of
`error_code`, `message` is a printable explanation). `error_name(code)` returns a short name for
logs.

## Error codes

| Code | Value | Meaning |
|---|---|---|
| `err_none` | 0 | success |
| `err_open_failed` | 1 | the input file could not be opened |
| `err_read_failed` | 2 | the input file could not be read to the end |
| `err_write_failed` | 3 | a face could not be written |
| `err_mkdir_failed` | 4 | the output directory could not be created |
| `err_not_a_collection` | 5 | the signature is not `ttcf` |
| `err_unsupported_version` | 6 | the collection version is neither 1.0 nor 2.0 |
| `err_empty_collection` | 7 | the header declares zero faces |
| `err_bad_face_count` | 8 | the face count exceeds the supported limit |
| `err_truncated` | 9 | header, directory or table data runs past the end of the file |
| `err_bad_face` | 10 | a face is not a known sfnt (`0x00010000` / `true` / `OTTO`) |
| `err_bad_table_count` | 11 | a face declares zero or too many tables |
| `err_out_of_range` | 12 | the face index is outside the collection |

## Output file naming

1. The family name from the `name` table (id 1); characters that are illegal in Windows file names
   become `_`, leading and trailing spaces and dots are trimmed, and the result is capped at 120
   bytes without cutting a UTF-8 sequence in half.
2. A face with no usable `name` table falls back to `<prefix>_<index>`.
3. On a collision `_<subfamily>` is appended; if that still collides, `_2`, `_3`, … Names are
   compared case-insensitively (Windows semantics).
4. The extension follows the sfnt version: `OTTO` (CFF outlines) is written as `.otf`, everything
   else as `.ttf`.

## Validation and robustness

Input is treated as untrusted data — a font that travels with a drawing, or was copied off a
customer's machine, is an ordinary source — so every step of the parse path is bounds-checked.
Anything unexpected returns an error code: no exceptions, no over-sized allocations.

| Check | On failure |
|---|---|
| File is at least 12 bytes | `err_truncated` |
| Signature == `ttcf` | `err_not_a_collection` |
| Version ∈ {1.0, 2.0} | `err_unsupported_version` |
| Face count > 0 | `err_empty_collection` |
| Face count ≤ 1024 **and** ≤ `(size-12)/4` | `err_bad_face_count` / `err_truncated` |
| Every face offset + 12 ≤ file size | `err_truncated` |
| Face sfnt version ∈ {`0x00010000`, `true`, `OTTO`} | `err_bad_face` |
| Table count 1…4096 | `err_bad_table_count` |
| Table directory 12 + 16×count ≤ file size | `err_truncated` |
| Every table `offset + length` ≤ file size | `err_truncated` |

The output side checks itself too, and the tests assert each item: every face is 4-byte aligned,
every table's checksum matches its directory entry, `head.magicNumber` is preserved, **the whole
file sums to `0xB1B0AFBA`**, and every table payload is byte-identical to the source collection.

### Files that are not collections

`fonts/` is where such files show up: eight "fonts" that start with random bytes and share a
uniform 28-byte header (carrying a `PA31` marker), with no `ttcf` / `OTTO` / `wOFF` signature
anywhere in the file and a first 4 KiB whose byte distribution is close to uniform — the
fingerprint of **transparent encryption / DRM wrapping**, which only decrypts for whitelisted
processes. The expected behaviour for that input is a clean one-line rejection, not a crash or a
half-written file:

- every entry point (`ListTTC` / `UnzipTTC` / `UnzipTTCFace` / `UnzipTTCToDir` /
  `UnzipTTCFaceToFile`) returns `false` with `error_info::code == err_not_a_collection`, and each
  output container stays empty;
- no directory is created and no file is left behind;
- the tests treat this corpus as a **positive** case: the library and the test's own independent
  reader must **agree on whether the bytes are a collection**. Both rejecting passes; one accepting
  while the other rejects fails.

If your product reads fonts on machines that run an encryption client, confirm first that the bytes
you receive are plaintext — otherwise a `.ttc` on disk looks exactly like this wrapper.

## Performance

0.3.0 replaced the byte-at-a-time `getc`/`putc` of 0.2.0 with "read the whole file once + `memcpy`
the ranges + write once":

| Input | 0.2.0 | 0.3.0 | Speed-up |
|---|---|---|---|
| `simsun.ttc`, 18 MB / 2 faces | 1,618 ms | **17 ms** | ≈ 95× |
| `mingliub.ttc`, 37 MB / 4 faces | 6,899 ms | **76 ms** | ≈ 90× |
| 15 system fonts, total | ≈ 25 s | ≈ 0.3 s | — |

Plain `fread` + `fwrite` with the same I/O pattern on the same machine (read once, write twice) has
a lower bound of 18.6 ms, so 0.3.0 already runs at disk bandwidth (≈ 1 GB/s, page cache included).

## Known limitations

- **Split only, never pack.** There is no `.ttf` → `.ttc` packing and no table deduplication: every
  face gets its own copy of the table data, so the output is 1.7–4.0× the input size
  (`simsun.ttc` 2.00×, `mingliub.ttc` 3.99×).
- **No subsetting.** No glyph pruning and no `DSIG` merging; the `DSIG` fields of a v2 header are
  not carried into the output faces.
- **Table data is copied verbatim** — only checksums and offsets are recomputed, no glyph decoding
  (deliberate: decoding belongs to a real parser such as `ttf2mesh` or `shx_tools`).
- TrueType Collections only. `.woff` / `.woff2` / `.dfont` are out of scope.
- Unpacking a large collection into memory peaks at about `face count × file size`. Use
  `UnzipTTCFace()` when memory matters.

## Tests

`test/test_ttc.cpp` is a single-file, framework-free regression test in two parts:

- **Synthetic fixtures** (no font file required): collections are built byte by byte to cover a
  two-face round trip, shared table pools, the 2.0 header, an `OTTO` face, 4-byte padding of
  odd-length tables, duplicate and nameless faces, automatic directory creation, the compatibility
  interfaces, twelve classes of malformed input, and the public interfaces of `Sfnt` /
  `Collection` / the file name rules.
- **Real collections**: CMake globs `fonts/*.ttc` and passes them as arguments; with no font it
  falls back to `TTC2SFNT_TEST_FONT`, `fonts/simsun.ttc`, `C:/Windows/Fonts/*.ttc`, and prints
  `SKIP` when none of those exists. For each font it compares the source collection and the output
  table by table and checks every table checksum and the whole-file checksum `0xB1B0AFBA`.

The assertions are independent of the implementation: the test carries its own sfnt/ttc reader, so a
bug in the library cannot hide behind the same reader. On this machine: 327 assertions for a single
`simsun.ttc`, 2,205 for all 15 system collections / 36 faces, all passing.

> The repository ships no fonts. `.ttc` files are usually vendor fonts (SimSun, for one), so only
> code is committed. Drop any `.ttc` into `fonts/` and the tests pick it up automatically; it stays
> untracked by git.

## Relationship to shx_tools

`shx_tools` does the opposite direction: it parses AutoCAD `.shx` shapes/fonts into outlines and
**writes** `.ttf` / `.otf`. The two share the same slice of sfnt knowledge (table directories, 4-byte
alignment, the `0xB1B0AFBA` checksum compensation), but one writes while the other splits. The
directory layout, the location of the version macros (both in the umbrella header, maintained by the
project) and `config.h.in` with the CMake `*_HEAD_ONLY` switch are kept identical as well.
`ttc2sfnt` does not depend on `shx_tools` and shares no code with it — each can be taken on its own
as a standalone third-party library.

## License

MIT, see [LICENSE](LICENSE).
