<p align="center">
  <img src="docs/logo.png" alt="ttc2sfnt" width="420">
</p>

# ttc2sfnt

[![CI](https://github.com/xiaohunqupo/ttc2sfnt/actions/workflows/ci.yml/badge.svg)](https://github.com/xiaohunqupo/ttc2sfnt/actions/workflows/ci.yml)

[English](README.md) ｜ 简体中文

把 TrueType Collection（`.ttc`）拆成独立的 `.ttf` / `.otf`。C++11，无第三方依赖，默认仅头文件
（与 [shx_tools](https://github.com/xiaohunqupo/shx_tools) 同一套目录与构建约定）。

版本 0.5.0 ｜ [变更日志](CHANGELOG.md) ｜ [格式说明](docs/ttc_format.md)

> **改名说明**：本项目原名 `ttc_tools`，因为和另一个同名的 TTC 字体转换库重名（容易被误装、
> 误引用），已更名为 `ttc2sfnt`。改名同时换掉了全部公开标识：头文件目录 `inc/ttc_tools/` →
> `inc/ttc2sfnt/`、umbrella 头 `ttc_tools.h` → `ttc2sfnt.h`、命名空间 `ttc_tools` →
> `ttc2sfnt`、宏前缀 `TTC_TOOLS_*` → `TTC2SFNT_*`。旧名不做兼容保留。

## 这个库解决什么问题

`.ttc` 是把多个字面（face）打包进一个文件，并让它们共用表数据的容器。Windows 上的中文字体
几乎都是这个格式：`simsun.ttc`、`msyh.ttc`、`msjh.ttc`、`msgothic.ttc`、`mingliub.ttc` …

而多数字体消费方只认单字体。最典型的是 [ttf2mesh](https://github.com/fetisov/ttf2mesh)
（TrueType 轮廓三角化）：把 `simsun.ttc` 直接交给 `ttf_load_from_file()` 会得到
`TTF_ERR_FMT`，因为它按 sfnt 头解析，`.ttc` 的 `ttcf` 头不在它的支持范围里。

`ttc2sfnt` 就是这一步适配：读集合 → 逐字面重建独立 sfnt（重写表目录、重算表校验和、补
`head.checkSumAdjustment`）→ 交回内存或写到磁盘。配合 `shx_tools`（`.shx` → 轮廓/字体）
与 `ttf2mesh`（`.ttf` → 2D/3D mesh），构成"CAD 图纸文字 → 几何/网格"的自有链路：

```
.shx  --shx_tools-->  轮廓 / .ttf / .otf ─┐
                                          ├─ ttf2mesh ─> 2D / 3D mesh
.ttc  --ttc2sfnt-->  .ttf / .otf ────────┘
```

## 目录

| 目录 | 内容 |
|---|---|
| `inc/ttc2sfnt/` | 公开头文件，一个文件一件事；版本号在 `ttc2sfnt.h` 里手写维护，`ttc2sfnt_config.h.in` 只承载 `TTC2SFNT_HEAD_ONLY` 开关（由 CMake 配置成 `<build>/generated/ttc2sfnt/ttc2sfnt_config.h`） |
| `src/ttc2sfnt/` | 与 `inc` 一一对应的实现，加一个内部用的 `common.h`（字节序、上限、校验和数学） |
| `sample/` | `ttcsplit`（拆分/列面）与 `ttcdump`（打印表目录并验证校验和） |
| `test/` | 单文件回归测试，自带合成夹具 |
| `docs/` | `.ttc` 字节布局与拆分算法；项目 logo（`logo.png`） |
| `fonts/` | 只放说明；真实字体不进仓库（见下） |

头文件与实现的对应关系就是调用层次：

```
ttc2sfnt.h  ─┬─ 版本宏 TTC2SFNT_VERSION_*（手写维护）
              ├─ ttc2sfnt_config.h (生成，只有 TTC2SFNT_HEAD_ONLY)
              ├─ ttc_error.h       error_code / error_info / error_name
              ├─ ttc_file.h        整文件读写、目录创建、UTF-8 路径
              ├─ ttc_sfnt.h        Sfnt：一个面的表目录 + 重建成独立字体
              ├─ ttc_name.h        name 表解码（UTF-16BE/Latin-1）、文件名规则
              ├─ ttc_collection.h  Collection：集合头、ListTTC / UnzipTTC / UnzipTTCFace
              └─ ttc_extract.h     UnzipTTCToDir / UnzipTTCFaceToFile
```

## 构建

```bash
cmake -S . -B build            # 默认 header only
cmake --build build
ctest --test-dir build --output-on-failure
```

| CMake 选项 | 默认 | 含义 |
|---|---|---|
| `TTC2SFNT_HEAD_ONLY` | `ON` | 头文件末尾直接 `#include` 同名 `.cpp`，所有函数加 `TTC2SFNT_HEAD_ONLY_INLINE`；库是 INTERFACE 目标，`inc` 与 `src` 都要在 include 路径上 |
| `TTC2SFNT_HEAD_ONLY=OFF` | | `src/*.cpp` 编成静态库，使用者只需要 `inc` |
| `TTC2SFNT_BUILD_TESTS` | `ON` | 构建 `ttc_tests` 并注册到 CTest |
| `TTC2SFNT_BUILD_SAMPLES` | `ON` | 构建 `ttcsplit` 与 `ttcdump` |

**版本号由项目自己维护**：`TTC2SFNT_VERSION_MAJOR` / `MINOR` / `PATCH` / `STRING` 写在
[`inc/ttc2sfnt/ttc2sfnt.h`](inc/ttc2sfnt/ttc2sfnt.h) 开头，改版本就是改这一处（并补一条
`CHANGELOG.md`），构建系统只读取它，不参与生成。

`ttc2sfnt_config.h` 只承载"构建方式"这个开关，它不在源码树里：`CMakeLists.txt` 把
`inc/ttc2sfnt/ttc2sfnt_config.h.in` 的 `#cmakedefine TTC2SFNT_HEAD_ONLY` 配置到
`<build>/generated/ttc2sfnt/ttc2sfnt_config.h`，所以构建目录必须排在 `inc` 之前。不用 CMake
时，把 `ttc2sfnt_config.h.in` 复制成 `inc/ttc2sfnt/ttc2sfnt_config.h`，按注释决定是否
`#define TTC2SFNT_HEAD_ONLY`。文件名和 include guard（`_TTC2SFNT_CONFIG_H_`）都带项目名，
和别的库的 `config.h` 落在同一条 include 路径上也不会认错。

集成方式：

| 方式 | 做法 |
|---|---|
| CMake 子项目 | `add_subdirectory(ttc2sfnt)` 后 `target_link_libraries(app PRIVATE ttc2sfnt::ttc2sfnt)` |
| 手工拷贝（header only） | 拷 `inc/ttc2sfnt/` + `src/ttc2sfnt/*.cpp` + 生成的 `ttc2sfnt_config.h`，两个目录都进 include 路径 |
| 手工拷贝（编译成库） | 只拷 `inc/ttc2sfnt/`，把 `src/ttc2sfnt/*.cpp` 加进工程 |

## 快速开始

```cpp
#include "ttc2sfnt/ttc2sfnt.h"

ttc2sfnt::error_info error;

// 1. 只看有哪些字面
std::vector<ttc2sfnt::Face> faces;
if (ttc2sfnt::ListTTC("C:/Windows/Fonts/simsun.ttc", faces, &error))
{
    for (std::size_t i = 0; i < faces.size(); ++i)
        std::printf("[%zu] %s %s, %u glyphs\n", i,
            faces[i].family.c_str(), faces[i].subfamily.c_str(), faces[i].num_glyphs);
}

// 2. 全部拆到目录（目录不存在会自动创建，文件按字体家族命名）
std::vector<std::string> written;
ttc2sfnt::UnzipTTCToDir("C:/Windows/Fonts/simsun.ttc", "out", "Face", &written, &error);
// -> out/SimSun.ttf, out/NSimSun.ttf

// 3. 只拆一个面进内存，直接喂给 ttf2mesh
ttc2sfnt::TTC::TTF face;
if (ttc2sfnt::UnzipTTCFace("C:/Windows/Fonts/simsun.ttc", 0, face, nullptr, &error))
    ttf_load_from_mem(face.data(), (int)face.size(), &ttf, false);
```

需要更细的控制时用类：

```cpp
ttc2sfnt::Collection collection;
collection.parse(bytes, size, &error);          // 只解析集合头
ttc2sfnt::Sfnt sfnt;
collection.sfnt_of(0, sfnt, &error);            // 只解析第 0 个面的表目录
const ttc2sfnt::Table_Record* head = sfnt.find(ttc2sfnt::Sfnt::tag_head);
for (std::size_t i = 0; i < sfnt.table_count(); ++i)
    std::printf("%s %u bytes\n",
        ttc2sfnt::Sfnt::tag_string(sfnt.table(i).tag).c_str(),
        (unsigned int)sfnt.table(i).length);
```

命令行：

```
ttcsplit <input.ttc> [output_dir]       # 全部拆出
ttcsplit -l <input.ttc>                 # 列字面
ttcsplit -x <input.ttc> <index> <out>   # 拆单个面
ttcdump  <input.ttc> [...]              # 打印表目录 + 校验和验证，退出码是问题数
```

### 下游会写穿 `const`：`ttf2mesh` 的 `ttf_load_from_mem()`

`ttf2mesh` 1.6 的签名是 `ttf_load_from_mem(const uint8_t*, int, ttf_t**, bool)`，但它在解析时把
**表目录和若干头结构原地转成主机字节序**（`conv32(rec->checkSum/offset/length)`，以及 `hhea`、
`cmap`、`glyf` 头等）。小端机器上实测：

| 观察 | 数据 |
|---|---|
| 表记录 0 的 `checkSum` / `offset` / `length` | 三个字段前后**完全颠倒**（`0E 30 4C 68` → `68 4C 30 0E`） |
| 目录之外被改写的字节 | cambria **49 254** 字节、mingliub **197 404** 字节 |
| 结果 | 混合字节序：既不是合法大端，也不是自洽小端 |

**这不可修复**——被转序的字段取决于它的解析路径，无法逆推；重算校验和救不回来。所以只有两条路：

```cpp
// 路线 A（推荐）：交给它一份一次性副本，face 保持原样
const int rc = ttc2sfnt::with_scratch_copy(face, [&](const std::uint8_t* data, std::size_t size)
{
    return ttf_load_from_mem(data, (int)size, &ttf, false);
});
// 这里 face 与原来逐字节相同，可以继续落盘或喂给别的库
```

```cpp
// 路线 B：交出去的那份就当一次性，后面要用就重新取
ttf_load_from_mem(face.data(), (int)face.size(), &ttf, false);   // face 已被破坏
ttc2sfnt::UnzipTTCFace(path, index, face, nullptr, &error);      // 重新取一份新的
```

要长期守住这条边界（例如在 debug 构建或自检里），用 `Sfnt::verify()` 断言——它一次遍历就能
判出"这块内存还是不是一个自洽的 sfnt"：

```cpp
ttc2sfnt::Sfnt_Report report;
if (!ttc2sfnt::Sfnt::verify(face.data(), face.size(), &report))
    std::fprintf(stderr, "face is damaged: %s\n", report.first_problem.c_str());
// 被 ttf2mesh 写坏时：table 'EBDT' at offset 0x5C010000 length 3965059072 runs past the end of the font
```

## API

| 函数 | 作用 |
|---|---|
| `ListTTC(file/data, faces, error)` | 只读表目录与 `name`/`maxp`，不复制数据 |
| `UnzipTTC(file/data, ttc, error)` | 全部字面重建进内存，`ttc.ttfs[i]` 是完整 sfnt，`ttc.faces[i]` 是元数据 |
| `UnzipTTCFace(file/data, index, ttf, face, error)` | 只重建一个字面（大集合时内存友好） |
| `UnzipTTCToDir(file/data, dir, prefix, written, error)` | 全部落盘，返回写出的路径 |
| `UnzipTTCFaceToFile(file, index, path, face, error)` | 单面落到指定路径 |
| `Collection` / `Sfnt` | 分两步解析：只读集合头，只读一个面的表目录 |
| `Sfnt::checksum()` / `Sfnt::tag_string()` | 校验和数学与标签打印，`ttcdump` 就是这么写的 |
| `Sfnt::verify(face, size, report)` | 校验一个独立字体：每表校验和、4 字节对齐、`head` magic、整文件 `0xB1B0AFBA`；`Sfnt_Report` 给出计数与第一处问题 |
| `with_scratch_copy(bytes, consumer)` | 交出一份一次性副本给"会写穿 `const`"的消费方（`ttf2mesh` 就是），调用方的字节保持不变 |
| `file_stem_for()` / `sanitize_file_name()` | 输出文件名的规则，可单独使用 |

失败时返回 `false`，并把原因写进可选的 `error_info`（`code` 见 `error_code`，`message` 是
可直接打印的说明）。`error_name(code)` 给出短名，便于日志。

## 输出文件命名

1. 用 `name` 表的 family（id 1），非法字符替换为 `_`，去掉首尾空格与点，最长 120 字节且不截断
   UTF-8 序列；
2. 字面没有可用 `name` 表时用 `<prefix>_<序号>`；
3. 重名时追加 `_<subfamily>`，仍重名追加 `_2`、`_3` …；比较不区分大小写（Windows 语义）；
4. 扩展名按 sfnt 版本决定：`OTTO`（CFF 轮廓）写 `.otf`，其余写 `.ttf`。

## 校验与容错

输入被视为不可信数据（图纸带字体、客户机拷字体都是常见来源），所以解析路径上每一步都做边界
检查，任何异常都只返回错误码，不抛异常、不做超额分配：

| 检查 | 不通过时 |
|---|---|
| 文件 ≥ 12 字节 | `err_truncated` |
| 签名 == `ttcf` | `err_not_a_collection` |
| 版本 ∈ {1.0, 2.0} | `err_unsupported_version` |
| 字面数 > 0 | `err_empty_collection` |
| 字面数 ≤ 1024 **且** ≤ `(size-12)/4` | `err_bad_face_count` / `err_truncated` |
| 每个字面偏移 + 12 ≤ 文件长度 | `err_truncated` |
| 字面 sfnt 版本 ∈ {`0x00010000`, `true`, `OTTO`} | `err_bad_face` |
| 表数 1…4096 | `err_bad_table_count` |
| 表目录 12 + 16×表数 ≤ 文件长度 | `err_truncated` |
| 每张表 `offset + length` ≤ 文件长度 | `err_truncated` |

输出侧同样自检，测试里逐项验证：每个面 4 字节对齐、每张表自身校验和与目录一致、
`head.magicNumber` 保留、**整文件校验和 == `0xB1B0AFBA`**、表数据与源集合逐字节相同。

### 不是 TTC 的文件

`fonts/` 里就混着这种文件：8 个以随机字节开头、带有统一 28 字节头（含 `PA31` 标记）的"字体"，
整份文件里找不到 `ttcf` / `OTTO` / `wOFF` 任何签名，前 4 KiB 的字节分布接近均匀——典型的**透明
加密/DRM 包装**（被白名单进程打开时才解密）。这类输入的期望行为是"一行代码干净地拒绝"，而不是
崩溃或写出半成品：

- 所有入口（`ListTTC` / `UnzipTTC` / `UnzipTTCFace` / `UnzipTTCToDir` / `UnzipTTCFaceToFile`）
  返回 `false`，`error_info::code == err_not_a_collection`，各自的输出容器保持为空；
- 不会创建目录、不会留下任何文件；
- 测试把这类语料当作**正向用例**：库与测试自带的独立读取器必须**对"是不是集合"得出相同结论**，
  双方都拒绝才算通过，一方接受一方拒绝则报错。

如果你的产品在装了加密客户端的机器上读字体，需要先确认拿到的字节是明文——否则 `.ttc` 在磁盘上
看起来就是这种包装。

## 性能

0.3.0 改为"整文件一次读入 + `memcpy` 区间 + 一次写出"，替代 0.2.0 的逐字节 `getc/putc`：

| 输入 | 0.2.0 | 0.3.0 | 提升 |
|---|---|---|---|
| `simsun.ttc` 18 MB / 2 面 | 1 618 ms | **17 ms** | ≈ 95× |
| `mingliub.ttc` 37 MB / 4 面 | 6 899 ms | **76 ms** | ≈ 90× |
| 15 个系统字体合计 | ≈ 25 s | ≈ 0.3 s | — |

同机 `fread` + `fwrite` 的 I/O 下界是 18.6 ms（读 1 份、写 2 份），也就是说 0.3.0 已经贴着
磁盘带宽跑（≈ 1 GB/s，含页缓存）。

## 已知边界

- **只拆不装**。没有 `.ttf` → `.ttc` 的打包，也没有表去重；每个面独占一份表数据，所以输出
  总体积是输入的 1.7–4.0 倍（`simsun.ttc` 2.00×，`mingliub.ttc` 3.99×）。
- **不做子集化**。不裁剪 glyph、不合并 DSIG；v2 头里的 DSIG 字段不会带到输出面里。
- **表数据原样复制**，只重算校验和与偏移，不解码字形（这是刻意的：解码交给 `ttf2mesh` 或
  `shx_tools` 那类真正的解析器）。
- 只支持 TrueType Collection；`.woff` / `.woff2` / `.dfont` 不在范围内。
- 大集合一次性进内存时峰值约为 `面数 × 文件长度`，需要控制内存请用 `UnzipTTCFace()`。

## 测试

`test/test_ttc.cpp` 是单文件、无框架的回归测试，分两部分：

- **合成夹具**（不依赖任何字体文件）：逐字节构造集合，覆盖双面往返、共享表池、2.0 头、
  `OTTO` 面、奇数长度表的 4 字节填充、重名与无名字面命名、目录自动创建、兼容接口、12 类
  畸形输入，以及 `Sfnt` / `Collection` / 文件名规则等公开接口。
- **真实集合**：CMake 把 `fonts/*.ttc` 通配后作为参数传给测试程序；没有字体时退回
  `TTC2SFNT_TEST_FONT`、`fonts/simsun.ttc`、`C:/Windows/Fonts/*.ttc`，全都没有就打印 `SKIP`。
  每个字体逐表比对源集合与输出、校验每张表校验和、校验整文件校验和 `0xB1B0AFBA`。

断言独立于实现：测试自带一份 sfnt/ttc 读取器，库里的 bug 不会藏在同一个读取器后面。
本机结果：单个 `simsun.ttc` 一轮 327 项断言，15 个系统集合 / 36 个字面一起跑 2 205 项，
全部通过。

> 仓库不附带字体文件。`.ttc` 多为厂商专有字体（例如中易宋体），仓库里只放代码；把任意
> `.ttc` 放进 `fonts/` 就会被测试自动使用，且不会被 git 跟踪。

## 与 shx_tools 的关系

`shx_tools` 做的是反方向：把 AutoCAD `.shx` 的形/字体解析成轮廓，再**写出** `.ttf` / `.otf`。
两者共用同一段 sfnt 知识（表目录、4 字节对齐、`0xB1B0AFBA` 校验和补偿），但一个写、一个拆；
目录布局、版本宏的位置（都写在 umbrella 头里、由项目自己维护）、`ttc2sfnt_config.h.in` 与 CMake 的
`*_HEAD_ONLY` 开关也保持一致。`ttc2sfnt` 不依赖 `shx_tools`，也不与它共享代码——各自作为独立
的三方库可以被单独取用。

## 许可

MIT，见 [LICENSE](LICENSE)。
