# 变更日志 / Changelog

每条先中文、后英文（英文以斜体标出）；表格与代码块只出现一次，表头做成双语。

*Every item is Chinese first, then English in italics; tables and code blocks appear once, with
bilingual column headers.*

## 0.4.1

文档发布：仓库改为中英双语并加入项目 logo。除版本宏的取值外没有代码变化，0.4.0 的调用方无需改动。

*Documentation release: the repository is bilingual now (Chinese/English) and carries a project
logo. Nothing changed in the code beyond the values of the version macros, so 0.4.0 callers need no
action.*

### 文档 / Documentation

- 本变更日志改为中英文对照：每条先中文、后英文（斜体）；表格与代码块只出现一次。

  *This changelog is bilingual now: every item is Chinese first, then English in italics; tables
  and code blocks appear once.*

- `README.md` 改为英文主页（GitHub 默认语言），原中文内容整体移到
  [`README.zh-CN.md`](README.zh-CN.md)，两篇顶部互加语言链接。

  *`README.md` is now the English landing page (GitHub's default language). The original Chinese
  text moved over to [`README.zh-CN.md`](README.zh-CN.md), and both files link to each other at
  the top.*

- 英文版按常规库 README 重排：新增 `Requirements`（C++11、MSVC `/W4` 与 GCC/Clang
  `-Wall -Wextra`、CMake ≥ 3.12）、`Integration`、`Error codes` 三节；命令行补上源码里已有但
  此前未列出的 `-v` / `-h`；其余章节与中文版信息量对齐。

  *The English version is reorganised the way a library README usually is: three new sections —
  `Requirements` (C++11, MSVC `/W4` and GCC/Clang `-Wall -Wextra`, CMake ≥ 3.12), `Integration`
  and `Error codes`; the command line section gained `-v` / `-h`, which the source had but the
  document never listed; the remaining sections carry the same information as the Chinese
  version.*

- 新增项目 logo `docs/logo.png`（1536×611，白底原图），在 `README.md` 与 `README.zh-CN.md`
  顶部居中展示（420 px 宽，不随主题反色）。

  *New project logo `docs/logo.png` (1536×611, the original white-background image), shown centred
  at the top of both `README.md` and `README.zh-CN.md` at 420 px wide; it does not invert with the
  theme.*

- 两篇 README 的目录表都补上 `docs/logo.png`。

  *Both READMEs' repository-layout tables now mention `docs/logo.png`.*

- 两篇 README 顶部加 CI 状态徽章（指向 `build-and-test` 工作流）。

  *Both READMEs got a CI status badge that points at the `build-and-test` workflow.*

### 内部 / Internal

- 恢复改名时丢失的 CI：重建 `.github/workflows/ci.yml`。Windows（`-A x64` / `-A Win32`）与
  Linux 各自跑 `TTC2SFNT_HEAD_ONLY=ON` / `OFF` 两档，configure → build → ctest；另加
  `workflow_dispatch`、同分支并发取消与 `contents: read` 最小权限。

  *Restored the CI that was lost in the rename: `.github/workflows/ci.yml` is rebuilt. Windows
  (`-A x64` / `-A Win32`) and Linux each run both `TTC2SFNT_HEAD_ONLY=ON` and `OFF` — configure →
  build → ctest — plus `workflow_dispatch`, per-ref concurrency cancellation and the minimal
  `contents: read` permission.*

- 版本号升到 0.4.1：`TTC2SFNT_VERSION_PATCH` / `TTC2SFNT_VERSION_STRING` 与
  `test_version()` 里的断言同步更新（沿用"版本号在 `ttc2sfnt.h` 手写维护、测试断言跟着改"
  的做法）。

  *Version bumped to 0.4.1: `TTC2SFNT_VERSION_PATCH` / `TTC2SFNT_VERSION_STRING` and the assertions
  in `test_version()` moved together, following the existing rule — the version is maintained by
  hand in `ttc2sfnt.h` and the test assertion follows it.*

### 兼容性 / Compatibility

- 除版本宏的取值外，公开头文件、函数签名、错误码、输出文件命名与构建开关都和 0.4.0 完全
  一致。

  *Apart from the values of the version macros, the public headers, function signatures, error
  codes, output file naming and build switches are exactly what 0.4.0 had.*

## 0.4.0

### 改名（破坏） / Renamed (breaking)

- 项目更名为 **`ttc2sfnt`**：原名 `ttc_tools` 与另一个同名的 TTC 字体转换库重名，容易被误装、
  误引用，因此换名。

  *The project is now **`ttc2sfnt`**. The old name `ttc_tools` collided with an unrelated TTC font
  conversion library, which made it easy to install or link the wrong one, hence the rename.*

- 公开标识一并更换，旧名不做兼容保留：

  *Every public identifier moved with it, and the old names are not kept as aliases:*

  | 旧 / Old | 新 / New |
  |---|---|
  | `#include "ttc_tools/ttc_tools.h"` | `#include "ttc2sfnt/ttc2sfnt.h"` |
  | 命名空间 / namespace `ttc_tools` | `ttc2sfnt` |
  | `TTC_TOOLS_VERSION_*` | `TTC2SFNT_VERSION_*` |
  | `TTC_TOOLS_HEAD_ONLY` / `_INLINE` | `TTC2SFNT_HEAD_ONLY` / `_INLINE` |
  | `TTC_TOOLS_BUILD_TESTS` / `_SAMPLES` | `TTC2SFNT_BUILD_TESTS` / `_SAMPLES` |
  | `<build>/generated/ttc_tools/config.h` | `<build>/generated/ttc2sfnt/config.h` |
  | 环境变量 / env var `TTC_TOOLS_TEST_FONT` | `TTC2SFNT_TEST_FONT` |

- 目录结构不变（`inc/<name>/` + `src/<name>/`），只是 `<name>` 换成 `ttc2sfnt`；模块级头文件名
  （`ttc_sfnt.h`、`ttc_collection.h` 等）、可执行文件 `ttcsplit` / `ttcdump` 与测试目标
  `ttc_tests` 保持原名 —— 它们描述的是 TTC 格式，不是项目名。

  *The directory layout is unchanged (`inc/<name>/` + `src/<name>/`); only `<name>` became
  `ttc2sfnt`. Module headers (`ttc_sfnt.h`, `ttc_collection.h`, …), the executables `ttcsplit` /
  `ttcdump` and the test target `ttc_tests` keep their names — they describe the TTC format, not
  the project.*

- 改名是纯文本替换，行为无变化：x64 / x86、header-only / 静态库四种组合构建零警告，完整语料
  1527 项断言全过。

  *The rename is a pure text substitution with no behaviour change: all four combinations of
  x64/x86 and header-only/static library build without warnings, and the full corpus passes all
  1,527 assertions.*

## 0.3.1

拿真实字体把公开 API 跑了一遍：7 个真集合（21 个面）全项通过，另外 8 个非 TTC 文件（透明加密
包装）被一致地拒绝；期间发现并修掉两个库缺陷，以及测试对语料的一个错误假设。

*The public API was exercised against real fonts: 7 real collections (21 faces) passed every check,
while 8 non-TTC files (transparent encryption wrappers) were rejected consistently. Two library
defects and one wrong assumption the tests made about the corpus were found and fixed along the
way.*

### 修复 / Fixed

- **`ensure_directory()` 把"路径被一个文件占用"报成成功。** `mkdir` 对"已存在的目录"和"同名
  的文件"都返回 `EEXIST`，旧实现直接当作成功。现在补一次类型查询：路径被文件占用时返回
  `false` + `err_mkdir_failed`，消息写明 `a file with that name is in the way`。落盘本身此前
  也会因此失败，所以这不是数据错误，而是**诊断错误**——调用方拿到的是一次假成功。

  *`ensure_directory()` reported "the path is taken by a file" as success. `mkdir` returns
  `EEXIST` both for a directory that already exists and for a file of that name, and the old
  implementation treated either case as success. It now looks the type up: when a file owns the
  path it returns `false` + `err_mkdir_failed`, with `a file with that name is in the way` in the
  message. Writing to disk had always failed in that case anyway, so this is a **diagnostic** bug,
  not a data bug — the caller was handed a false success.*

- **`file_stem_for()` 的重名循环有魔法上限 10000**，超过后会静默返回一个重复名，让后一个面覆盖
  前一个。改为按 `used.size() + 1` 个互不相同的候选去试：候选数严格多于已用名数，必然存在空闲
  名，也不再依赖任意常数。

  *The collision loop in `file_stem_for()` had a magic limit of 10000 and, past it, silently
  returned a duplicate name, so the next face overwrote the previous one. It now tries
  `used.size() + 1` distinct candidates: strictly more candidates than names in use, so a free one
  must exist — and no arbitrary constant is involved.*

### 测试 / Tests

- **测试不再假设语料都是 TTC。** 原先"真实集合"一节对每个语料直接断言 `ref::parse_ttc()` 成功，
  于是 `fonts/` 里一旦出现非 TTC 文件（加密包装、残缺文件）测试就红。现在先让库与测试自带的
  独立读取器各自判断，**两者结论必须一致**：都接受 → 全套逐表校验；都拒绝 → 逐个入口断言拒绝
  行为（返回值、错误码、输出容器为空）；只有结论不一致才算失败。语料目录因此可以放心放"坏文件"。

  *The tests no longer assume the corpus is TTC. The "real collections" section used to assert
  `ref::parse_ttc()` success for every file, so a single non-TTC file in `fonts/` (an encryption
  wrapper, a truncated file) turned the suite red. Now the library and the test's own independent
  reader each decide first, and **the two must agree**: both accepting → the full table-by-table
  verification; both rejecting → every entry point is checked for the documented rejection (return
  value, error code, empty output containers); only a disagreement fails. A corpus directory can
  therefore hold "bad files" safely.*

- 新增用例：路径被同名文件占用时的 `ensure_directory()`；64 个同名字面连续取文件名必须两两
  不同；父目录不存在时 `write_file()` 的错误码；`parent_directory()` / `join_path()` 的边界。

  *New cases: `ensure_directory()` when a file of the same name occupies the path; 64 same-named
  faces must get 64 distinct file names in a row; the error code of `write_file()` when the parent
  directory is missing; the edges of `parent_directory()` / `join_path()`.*

- 测试用例名与清理逻辑对残留状态更健壮（上一轮中断留下的同名目录不会让下一轮误判）。

  *Case names and cleanup are more robust against leftovers — a same-named directory left behind
  by an interrupted run no longer confuses the next one.*

### 验证 / Verification

- 7 个真实集合（cambria、mingliub、msgothic、msjh、msjhbd、msjhl、Nirmala，共 21 个面）：
  每个面逐表与源集合比对、每张表校验和、整文件 `0xB1B0AFBA`、4 字节对齐、`head.magicNumber`
  全部通过；产出的 21 个 `.ttf` 都能被 ttf2mesh 1.6 加载。

  *7 real collections (cambria, mingliub, msgothic, msjh, msjhbd, msjhl, Nirmala — 21 faces in
  total): every face compared table by table against the source collection, every table checksum,
  the whole-file `0xB1B0AFBA`, the 4-byte alignment and `head.magicNumber` all passed; all 21
  output `.ttf` files load in ttf2mesh 1.6.*

- 8 个非 TTC 文件（随机字节 + `PA31` 标记的加密包装）：五个入口全部 `err_not_a_collection`，
  无输出、无残留文件。

  *8 non-TTC files (random bytes behind a `PA31` marker — encryption wrappers): all five entry
  points returned `err_not_a_collection`, with no output and no leftover files.*

- 独立 API 检查程序（不在仓库内）5948 项断言：全部公开接口、Unicode 文件名（CJK family 落盘后
  可读回）、磁盘与内存结果逐字节一致、覆盖写；`Sfnt` / `Collection` 与自由函数结果互相印证。

  *A separate API checker (not part of the repository), 5,948 assertions: every public interface,
  Unicode file names (a CJK family name still reads back after being written), disk and memory
  results byte-identical, overwriting; `Sfnt` / `Collection` and the free functions cross-checked
  against each other.*

- 模糊测试：对有效集合 400 轮变异（截断/翻转字节/改头），对每个非 TTC 文件各 40 轮，合计
  1000+ 轮无崩溃；拒绝时错误码与消息齐备，被接受的变异产物仍然通过结构校验。

  *Fuzzing: 400 mutation rounds on a valid collection (truncation, byte flips, header edits) plus
  40 per non-TTC file — 1,000+ rounds in total with no crash; rejections carried both code and
  message, and accepted mutants still passed the structural checks.*

### 新增 / Added

- **`Sfnt::verify()` + `Sfnt_Report`**：校验一个独立字体（`build()` 的产物，或任意 `.ttf`/`.otf`）
  —— 逐表重算校验和、4 字节对齐、`head.magicNumber`、整文件 `0xB1B0AFBA`。只额外拷 `head`
  （54 字节），可以在 debug 构建或自检里直接断言。`Sfnt_Report::first_problem` 给出可直接打印
  的第一处问题。

  *`Sfnt::verify()` + `Sfnt_Report`: validates a standalone font (what `build()` produces, or any
  `.ttf`/`.otf`) — every table checksum recomputed, 4-byte alignment, `head.magicNumber`, the
  whole-file `0xB1B0AFBA`. It copies only `head` (54 bytes), so it can be asserted in a debug build
  or a self test; `Sfnt_Report::first_problem` carries the first problem in printable form.*

- **`with_scratch_copy(bytes, consumer)`**：把一份**一次性副本**交给"声明 `const` 却会写进去"的
  消费方，调用方的字节原样保留。这正是 `ttf2mesh` 需要的那层隔离：

  *`with_scratch_copy(bytes, consumer)`: hands a **throwaway copy** to a consumer that declares
  `const` and writes through it anyway, leaving the caller's bytes intact. This is exactly the
  isolation `ttf2mesh` needs:*

  ```cpp
  const int rc = ttc2sfnt::with_scratch_copy(face, [&](const std::uint8_t* data, std::size_t size) {
      return ttf_load_from_mem(data, (int)size, &ttf, false);
  });
  // `face` is byte-for-byte what it was before the call
  ```

### 文档 / Documentation

- README 增加"不是 TTC 的文件"一节（加密包装的识别特征与期望行为）。

  *README gained a "Files that are not collections" section: the fingerprint of an encryption
  wrapper and the behaviour to expect from it.*

- README 增加"下游会写穿 `const`"一节，把 `ttf2mesh` 的行为讲清楚：它**不是重算校验和**，而是把
  表目录与若干头结构**原地 big→host 转序**（实测 cambria 49 254 字节、mingliub 197 404 字节被改，
  表记录的 `checkSum`/`offset`/`length` 全部前后颠倒），结果是混合字节序、**不可修复**。给出两条
  正确用法（`with_scratch_copy()` 或重新 `UnzipTTCFace()`）与一条自检（`Sfnt::verify()`）。

  *README gained a "Consumers that write through const" section that spells out what `ttf2mesh`
  does: it does **not** recompute checksums, it converts the table directory and several header
  structures **in place from big endian to host order** (measured: cambria 49,254 bytes and
  mingliub 197,404 bytes rewritten, with `checkSum`/`offset`/`length` of every table record
  byte-reversed), which leaves a mixed-endian, **unrepairable** font. Two correct usages
  (`with_scratch_copy()` or a fresh `UnzipTTCFace()`) and one self-check (`Sfnt::verify()`) are
  given.*

### 精简（1.0 之前不留包袱） / Simplified (no baggage before 1.0)

- **删掉没有人用的公开接口**：`Collection::data()` / `size()` / `face_offset()`、
  `Sfnt::start()` / `tables()` / `table_data()`，以及 `Sfnt` 里只为它们存在的 `m_start` 成员。
  留下的访问器（`version()` / `face_count()` / `table_count()` / `table()` / `index_of()` /
  `find()`）已经覆盖全部需要。

  *Removed public interfaces nobody used: `Collection::data()` / `size()` / `face_offset()`,
  `Sfnt::start()` / `tables()` / `table_data()`, and the `m_start` member of `Sfnt` that existed
  only for them. The accessors left (`version()` / `face_count()` / `table_count()` / `table()` /
  `index_of()` / `find()`) cover everything needed.*

- **`name` 表的三个解码器降为内部实现**：`utf16be_to_utf8()` / `latin1_to_utf8()` /
  `name_string()` 移进 `ttc2sfnt::details`，公开接口只保留调用方真正会用的
  `sanitize_file_name()` 与 `same_file_name()`。

  *The three `name` table decoders became internal: `utf16be_to_utf8()` / `latin1_to_utf8()` /
  `name_string()` moved into `ttc2sfnt::details`, leaving only `sanitize_file_name()` and
  `same_file_name()` public — the two a caller actually uses.*

- **空输入只有一种判定**：`nullptr` 与"0 字节"现在都走 `err_truncated`，删掉了为区分二者而存在
  的 `details::empty_pointer()`（库内 4 处调用）与测试里的 `bytes_pointer()`。

  *Empty input has a single verdict: `nullptr` and "zero bytes" both go to `err_truncated` now;
  `details::empty_pointer()` (four call sites in the library) and `bytes_pointer()` in the test,
  which existed only to tell the two apart, are gone.*

- **sfnt 结构的校验只留一处**：`Collection::parse()` 不再逐字面预检偏移（那是 `Sfnt::parse()`
  的职责，而且每个面本来都会调到它），只校验集合头本身。

  *sfnt structure checking lives in one place: `Collection::parse()` no longer pre-checks every
  face offset (that is `Sfnt::parse()`'s job, and every face reaches it anyway); it validates the
  collection header only.*

- **`Collection::extract()` 不再二次解析表目录**：Face 元数据由已经解析好的 `Sfnt` 直接填出，
  每个面少一次目录解析。

  *`Collection::extract()` no longer parses the table directory twice: the `Face` metadata is
  filled from the already parsed `Sfnt`, saving one directory parse per face.*

- **去掉与 `#pragma once` 重复的 include guard**：7 个头文件各少 3 行，也顺带消除了
  `_CONFIG_H_` 这类通用宏名与别的库撞车的可能。

  *Removed include guards that duplicated `#pragma once`: three lines less in each of the seven
  headers, and generic macro names such as `_CONFIG_H_` can no longer collide with another
  library.*

- **`ttcdump` 不再自己重算校验和**：改为打印表目录后直接调 `Sfnt::verify()`，样例少 30 行，
  验证逻辑也只剩库内一份。

  *`ttcdump` no longer recomputes checksums itself: it prints the table directory and calls
  `Sfnt::verify()`, which is 30 lines less in the sample and one copy of the verification logic
  (the library's).*

### 兼容性（破坏） / Compatibility (breaking)

- **移除了对 0.2.0 的全部兼容别名**：`UnzipTTc()`、`unzipTTC_Memery()`、`unzipTTC_Memory()`。
  0.2.0 的老代码要改用 `UnzipTTCToDir()`（落盘）或 `UnzipTTC()`（进内存）。保留它们除了让
  0.3.0 的接口表多三行、让测试多一段兼容用例之外没有任何收益：0.2.0 的实现本身跑不通
  （`unzipTTC_Memery()` 从不写回输出参数），而 0.3.0 已经把调用方全部迁到新名字上。

  *All 0.2.0 compatibility aliases are gone: `UnzipTTc()`, `unzipTTC_Memery()`,
  `unzipTTC_Memory()`. Old 0.2.0 code uses `UnzipTTCToDir()` (to disk) or `UnzipTTC()` (into
  memory) instead. Keeping them bought nothing beyond three extra rows in the 0.3.0 API table and
  one more compatibility case in the test: the 0.2.0 implementation never worked anyway
  (`unzipTTC_Memery()` never wrote its output parameter), and 0.3.0 had already moved every caller
  to the new names.*

- 0.2.0 → 0.3.x 本来就整体换了 API（`ListTTC` / `UnzipTTC` / `UnzipTTCFace` / `Collection` /
  `Sfnt`），这次只是把最后三行旧名字清掉。

  *0.2.0 → 0.3.x had already replaced the whole API (`ListTTC` / `UnzipTTC` / `UnzipTTCFace` /
  `Collection` / `Sfnt`); this just cleared the last three old names.*

- 项目还没到 1.0，**不承诺任何旧版本兼容**：删接口、改语义都按"让当前这份最简洁"来做。

  *The project is not at 1.0 and promises no backward compatibility: interfaces get deleted and
  semantics get changed whenever that keeps the current code simplest.*

## 0.3.0

一次以"能托付"为目标的收尾：修掉 0.2.0 里跑不通的接口、把逐字节 I/O 换成内存搬运、给外部
输入加上边界校验，按 `shx_tools` 的约定重排目录与构建，并补齐测试与文档。

*A release meant to make the library something you can hand over: fix the 0.2.0 entry points that
never worked, replace the byte-at-a-time I/O with memory moves, add boundary checks for external
input, rearrange the directories and the build to match `shx_tools`, and fill in the tests and the
documentation.*

### 架构与构建 / Architecture and build

- **移除 Visual Studio 工程**（`Project/*.vcxproj`），构建只走 CMake：`inc/` 公开头文件、
  `src/` 实现、`sample/` 示例、`test/` 测试，与 `shx_tools` 完全一致。

  *Removed the Visual Studio project (`Project/*.vcxproj`); the build is CMake only — `inc/` public
  headers, `src/` implementation, `sample/` examples, `test/` tests — exactly like `shx_tools`.*

- **版本号由项目自己维护**：`TTC2SFNT_VERSION_MAJOR` / `MINOR` / `PATCH` / `STRING` 写在
  `inc/ttc2sfnt/ttc2sfnt.h` 开头（与 `shx_tools` 的 `SHX_TOOLS_VER_*` 同一位置），升级就是
  改这一处加一条变更日志。CMake 不再配置版本，`project()` 也不声明 `VERSION`，避免两个数字
  打架；`config.h.in` 只保留构建方式开关 `TTC2SFNT_HEAD_ONLY`。

  *The version is maintained by the project itself: `TTC2SFNT_VERSION_MAJOR` / `MINOR` / `PATCH` /
  `STRING` sit at the top of `inc/ttc2sfnt/ttc2sfnt.h` (the same place as `SHX_TOOLS_VER_*` in
  `shx_tools`), so a bump is one edit plus a changelog entry. CMake configures no version any more
  and `project()` declares no `VERSION`, so two numbers cannot disagree; `config.h.in` keeps only
  the build-mode switch `TTC2SFNT_HEAD_ONLY`.*

- **`TTC2SFNT_HEAD_ONLY` 开关**（默认 `ON`）：开启时头文件末尾直接 include 同名 `.cpp`，
  函数用 `TTC2SFNT_HEAD_ONLY_INLINE` 保持 `inline`，库是 INTERFACE 目标；关闭时 `src/*.cpp`
  编成静态库。两种模式都在本机验证过（`ctest` 1/1 通过）。

  *The `TTC2SFNT_HEAD_ONLY` switch (default `ON`): when on, each header includes its matching
  `.cpp` at the end and the functions use `TTC2SFNT_HEAD_ONLY_INLINE` to stay `inline`; the library
  is an INTERFACE target. When off, `src/*.cpp` is compiled into a static library. Both modes were
  verified locally (`ctest` 1/1 passing).*

- 单文件 `include/ttc2sfnt.ipp` 拆成六个模块：`ttc_error`（错误码）、`ttc_file`（整文件读写
  与目录）、`ttc_sfnt`（`Sfnt` 类：一个面的表目录 + 重建）、`ttc_name`（`name` 表与文件名
  规则）、`ttc_collection`（`Collection` 类与 `ListTTC`/`UnzipTTC*`）、`ttc_extract`（落盘）。
  内部辅助集中在 `src/ttc2sfnt/common.h`。

  *The single-file `include/ttc2sfnt.ipp` became six modules: `ttc_error` (error codes), `ttc_file`
  (whole-file I/O and directories), `ttc_sfnt` (the `Sfnt` class: one face's table directory and
  its rebuild), `ttc_name` (the `name` table and the file name rules), `ttc_collection` (the
  `Collection` class and `ListTTC`/`UnzipTTC*`), `ttc_extract` (writing to disk). Internal helpers
  live in `src/ttc2sfnt/common.h`.*

- **新增类接口** `Collection` / `Sfnt`：可以只解析集合头、只解析一个面的表目录，或单独调用
  `Sfnt::build()` / `Sfnt::checksum()` / `Sfnt::tag_string()`；0.3.0 的自由函数全部保留。

  *New class interfaces `Collection` / `Sfnt`: parse only the collection header, parse only one
  face's table directory, or call `Sfnt::build()` / `Sfnt::checksum()` / `Sfnt::tag_string()` on
  their own; every 0.3.0 free function is kept.*

- 示例程序改成两个：`ttcsplit`（拆分/列面/单面导出）与 `ttcdump`（打印表目录并验证每张表
  校验和与整文件校验和，退出码是问题数）。

  *The example became two programs: `ttcsplit` (split / list faces / export one face) and
  `ttcdump` (print the table directories, verify every table checksum and the whole-file checksum;
  the exit code is the number of problems).*

- 补 `.github/workflows/ci.yml`：Windows 与 Linux，`TTC2SFNT_HEAD_ONLY` 取 `ON` / `OFF`
  各跑一遍 configure + build + ctest。

  *Added `.github/workflows/ci.yml`: Windows and Linux, `TTC2SFNT_HEAD_ONLY` both `ON` and `OFF`,
  each running configure + build + ctest.*

### 修复 / Fixed

- **内存接口原先什么都不返回。** `unzipTTC_Memery()` 把结果写进局部变量 `tempData`，从未写回
  输出参数，函数末尾也没有 `return`（MSVC `/W4` 报 C4715、C4100）。实测返回 `true` 而
  `ttfs.size() == 0`。0.3.0 把它改成 `UnzipTTC()` 的别名并补了拼写正确的 `unzipTTC_Memory()`
  ——两个别名在 0.3.1 里已删除，直接用 `UnzipTTC(file, ttc, error)`。

  *The in-memory entry point returned nothing. `unzipTTC_Memery()` wrote its result to a local
  `tempData`, never wrote the output parameter, and had no `return` at the end (MSVC `/W4` reports
  C4715 and C4100). Measured: it returned `true` with `ttfs.size() == 0`. 0.3.0 made it an alias of
  `UnzipTTC()` and added the correctly spelled `unzipTTC_Memory()` — both aliases were removed in
  0.3.1; use `UnzipTTC(file, ttc, error)`.*

- **失败被当成功。** 输出目录不存在时旧实现什么都不写却返回 `true`；单个字面 `fopen` 失败也
  只是 `continue`。现在目录会自动创建（可多级），任一字面写入失败即整体失败并给出
  `err_write_failed`。

  *Failures were reported as success. With a missing output directory the old implementation wrote
  nothing and returned `true`; a per-face `fopen` failure was only a `continue`. The directory is
  now created (including missing parents), and any failed write fails the whole call with
  `err_write_failed`.*

- **不校验输入。** 16 字节的伪造集合头（`numFonts = 0x7FFFFFFF`）会让 0.2.0 分配 8.6 GB 并
  空转；字面偏移越界会抛未捕获的 `std::length_error`；表数读到垃圾值时返回 `true` 并写出坏
  文件。现在有 10 项边界校验（见 README 表格），全部走错误码，不抛异常、不做超额分配。

  *Input was not validated. A 16-byte forged collection header (`numFonts = 0x7FFFFFFF`) made
  0.2.0 allocate 8.6 GB and spin; an out-of-range face offset threw an uncaught
  `std::length_error`; a garbage table count returned `true` and wrote a broken file. There are now
  10 boundary checks (see the README table), all reported through error codes — no exceptions, no
  over-sized allocation.*

- **表头 `searchRange` / `entrySelector` / `rangeShift` 改为重算**，不再照抄源文件。

  *The sfnt header's `searchRange` / `entrySelector` / `rangeShift` are recomputed instead of
  copied from the source file.*

### 性能 / Performance

- 读路径从"逐字节 `getc` + 每表 `fseek`"改为"整文件一次 `fread` + `memcpy` 区间 + 一次
  `fwrite`"，输出缓冲一次分配。

  *The read path went from "byte-at-a-time `getc` plus a `fseek` per table" to "one `fread` of the
  whole file + `memcpy` of the ranges + one `fwrite`", with the output buffer allocated once.*

| 输入 / Input | 0.3.0 之前 / before 0.3.0 | 0.3.0 | 提升 / Speed-up |
|---|---|---|---|
| `simsun.ttc`（18 MB / 2 面） | 1,618 ms | **17 ms** | ≈ 95× |
| `mingliub.ttc`（37 MB / 4 面） | 6,899 ms | **76 ms** | ≈ 90× |
| 15 个系统字体合计 / 15 system fonts | ≈ 25 s | ≈ 0.3 s | — |

### 新增 / Added

- `ListTTC()`：只读集合头与 `name` / `maxp`，列出字面（家族名、子家族名、glyph 数、表数、
  sfnt 类型）而不复制数据。

  *`ListTTC()`: reads only the collection header plus `name` / `maxp` and lists the faces (family,
  subfamily, glyph count, table count, sfnt type) without copying any data.*

- `UnzipTTC()` / `UnzipTTCFace()` 的内存版与单面版；单面版避免"面数 × 文件长度"的内存峰值。

  *In-memory and single-face versions of `UnzipTTC()` / `UnzipTTCFace()`; the single-face version
  avoids the "face count × file size" memory peak.*

- `UnzipTTCToDir()` 与 `UnzipTTCFaceToFile()`：落盘接口，返回写出的路径列表。

  *`UnzipTTCToDir()` and `UnzipTTCFaceToFile()`: the on-disk entry points, returning the list of
  paths written.*

- `error_info` / `error_code` / `error_name()`：可打印的失败原因，`nullptr` 时可忽略。

  *`error_info` / `error_code` / `error_name()`: printable failure reasons, ignorable by passing
  `nullptr`.*

- 以 `name` 表命名输出文件（`SimSun.ttf` 而不是 `face_00.ttf`），处理非法字符、UTF-8 截断、
  重名（`_<subfamily>`、`_2`）、无 `name` 表回退，并按 sfnt 版本选择 `.ttf` / `.otf`。

  *Output files are named after the `name` table (`SimSun.ttf` rather than `face_00.ttf`),
  handling illegal characters, UTF-8 truncation, collisions (`_<subfamily>`, `_2`) and faces
  without a `name` table, and picking `.ttf` / `.otf` by sfnt version.*

- 跨平台：去掉 `fopen_s` / `sprintf_s` / `_MAX_PATH`；Windows 上用 `_wfopen` + UTF-8→UTF-16
  转换，非 ASCII 字体名也能落盘；目录创建用 `_wmkdir` / `mkdir`。

  *Cross-platform: dropped `fopen_s` / `sprintf_s` / `_MAX_PATH`; on Windows the code uses
  `_wfopen` with a UTF-8→UTF-16 conversion, so non-ASCII font names reach the disk; directories are
  created with `_wmkdir` / `mkdir`.*

- 示例程序改为真正的命令行（`-l` / `-x` / `-v` / `-h`），不再是硬编码路径的空壳。

  *The example is a real command line now (`-l` / `-x` / `-v` / `-h`) instead of a shell with a
  hard-coded path.*

- `test/test_ttc.cpp`（自包含回归测试）、`docs/ttc_format.md`（格式与算法说明）、
  `.gitignore`、`.github/workflows/ci.yml`。

  *`test/test_ttc.cpp` (self-contained regression test), `docs/ttc_format.md` (the format and the
  algorithm), `.gitignore`, `.github/workflows/ci.yml`.*

### 测试 / Tests

- 合成夹具覆盖：双字面往返、共享表池、2.0 头部、`OTTO` 面、奇数长度表的 4 字节填充、
  重名与无名字面命名、目录自动创建、兼容接口、12 类畸形输入，以及 `Sfnt` / `Collection` /
  文件名规则的公开接口。

  *The synthetic fixtures cover: a two-face round trip, shared table pools, the 2.0 header, an
  `OTTO` face, 4-byte padding of odd-length tables, duplicate and nameless faces, automatic
  directory creation, the compatibility interfaces, 12 classes of malformed input, and the public
  interfaces of `Sfnt` / `Collection` / the file name rules.*

- 断言独立于实现：测试自带一份 sfnt/ttc 读取器，逐表比对源集合与输出、校验每张表校验和、
  校验整文件校验和 `0xB1B0AFBA`、校验 4 字节对齐与 `head.magicNumber`。

  *The assertions are independent of the implementation: the test carries its own sfnt/ttc reader
  and compares the source collection with the output table by table, checks every table checksum,
  the whole-file checksum `0xB1B0AFBA`, the 4-byte alignment and `head.magicNumber`.*

- 真实集合由 CMake 把 `fonts/*.ttc` 通配后作为参数传入，没有字体时退回
  `TTC2SFNT_TEST_FONT` 与系统字体路径。本机 15 个系统集合、36 个字面全部通过。

  *CMake globs `fonts/*.ttc` and passes them as arguments; without fonts it falls back to
  `TTC2SFNT_TEST_FONT` and the system font paths. On this machine 15 system collections (36 faces)
  all passed.*

### 仓库 / Repository

- 移除了随仓库提交的 18 MB `fonts/simsun.ttc`（中易宋体，厂商专有字体，MIT 只覆盖代码）。
  本地保留的文件不受影响，测试会自动使用它；仓库内改为 `fonts/README.md` 说明。

  *Removed the 18 MB `fonts/simsun.ttc` that used to be committed (SimSun, a vendor-proprietary
  font; the MIT license covers the code only). Locally kept copies are unaffected and the tests
  pick them up automatically; the repository now explains this in `fonts/README.md`.*

### 兼容性 / Compatibility

- 0.2.0 的两个函数名与签名保持不变：`UnzipTTc(file, dir, prefix)`、`unzipTTC_Memery(file, ttc)`。
  两者的行为都变好了（前者会建目录并按字体名命名，后者真的返回数据），但不再"静默成功"。

  *The two 0.2.0 function names and signatures are unchanged: `UnzipTTc(file, dir, prefix)` and
  `unzipTTC_Memery(file, ttc)`. Both behave better (the first creates the directory and names files
  after the font, the second actually returns data), but neither "silently succeeds" any more.*

- `ttc2sfnt::TTC` 增加 `version` 与 `faces` 两个成员，`ttfs` 语义不变。

  *`ttc2sfnt::TTC` gained `version` and `faces`; `ttfs` means what it did.*

## 0.2.0

- 初版：读取 `.ttc` 头，逐字面复制表数据、重写表偏移、补 `head.checkSumAdjustment`，
  支持落盘与"进内存"两个入口（后者未写回输出参数）。

  *Initial version: reads the `.ttc` header, copies the table data per face, rewrites the table
  offsets, fixes `head.checkSumAdjustment`, and supports two entry points — to disk and "into
  memory" (the latter never wrote its output parameter).*
