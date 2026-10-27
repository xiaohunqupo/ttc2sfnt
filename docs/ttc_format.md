# `.ttc` 字节布局与拆分算法

本文记录 `ttc2sfnt` 依赖的格式事实，以及拆分时每一步为什么这么做。文中所有偏移都相对文件
起点，所有整数都是大端（big endian）。

## 1. 集合头

```
offset  size  field
0       4     ttcTag            = 'ttcf' (0x74746366)
4       4     version           = 0x00010000 (1.0) 或 0x00020000 (2.0)
8       4     numFonts          字面数
12      4×n   offsetTable[n]    每个字面的 sfnt 头在文件中的绝对偏移
12+4n   4     dsigTag           仅 2.0：'DSIG' 或 0
16+4n   4     dsigLength        仅 2.0
20+4n   4     dsigOffset        仅 2.0
```

要点：

- **偏移是绝对偏移**，不是相对头部的偏移，也不是相对上一个字面的偏移。
- 2.0 只是在 1.0 后面多了 12 字节的 DSIG 描述；没有签名时三个字段通常都是 0。拆分时不把
  DSIG 带到输出里（它对单字体没有意义）。
- `numFonts` 是拆分代码里第一个需要设上限的字段：它决定要分配多少个偏移、多少个输出缓冲。

## 2. 一个字面 = 一个普通 sfnt

`offsetTable[i]` 指向的位置开始就是一个标准 sfnt（TrueType/OpenType 字体）：

```
offset  size  field
0       4     sfntVersion    0x00010000 (TrueType) / 'true' (Apple) / 'OTTO' (CFF)
4       2     numTables
6       2     searchRange    = 16 × 2^floor(log2(numTables))
8       2     entrySelector  = floor(log2(numTables))
10      2     rangeShift     = 16 × numTables - searchRange
12      16×n  tableRecords[n]
```

每条表记录 16 字节：

```
offset  size  field
0       4     tag           'head' 'name' 'glyf' …
4       4     checkSum      该表数据的校验和
8       4     offset        表数据在文件中的绝对偏移
12      4     length        表数据长度（未对齐前的真实长度）
```

## 3. 共享：集合的意义所在

在 `.ttc` 里，多个字面**指向同一份表数据**。`simsun.ttc` 的 SimSun 与 NSimSun 共用
`glyf`、`cmap` 等大表，只在 `name`、`head` 等少数表上不同。这就是"18 MB 装下两个 18 MB
字体"的原因。

因此拆分必然要**复制**：单字体消费方要求表数据在同一个连续缓冲区里。代价是输出总体积放大到
输入的 1.7–4.0 倍（本机实测：`simsun.ttc` 2.00×、`msgothic.ttc` 2.92×、`mingliub.ttc` 3.99×），
且倍数与"共享程度"直接相关。

另一种做法是不拆分、直接生成新的 `.ttc`（保留共享），但那要求消费方支持集合——正是本库要
绕开的前提。

## 4. 校验和

两条规则，缺一条字体就会被认为是损坏的：

1. **每张表**：把表数据按 4 字节对齐（尾部补 0）后按 32 位大端求和，结果写进表记录的
   `checkSum` 字段。
2. **整个文件**：把文件按 4 字节对齐后同样求和，要求等于 `0xB1B0AFBA`。为了让这个等式成立，
   `head` 表里偏移 8 处的 `checkSumAdjustment` 要写成

   ```
   checkSumAdjustment = 0xB1B0AFBA - checksum(整个文件，且该字段先置 0)
   ```

   注意顺序：先把该字段清零再算总和，否则算出来的值会把自己包含进去，永远收敛不了。

`head` 表内偏移（相对 `head` 起点）：`version`@0、`fontRevision`@4、`checkSumAdjustment`@8、
`magicNumber`@12（必须为 `0x5F0F3CF5`）。

在 `.ttc` 里，`head.checkSumAdjustment` 是整个集合算出来的值；拆出来的单字体必须重算，否则
`ttf2mesh` 这类会校验的文件加载器会直接拒收。

## 5. 拆分步骤

1. 校验集合同头部：签名、版本、`numFonts`、偏移数组是否越界。
2. 逐字面解析表目录，校验 `numTables` 与每条表记录的 `offset + length` 都在文件内。
3. 计算输出长度：`12 + 16n + Σ align4(length)`，一次性 `assign` 成 0 字节缓冲（顺带把表间
   的填充字节清零，保证 4 字节对齐且填充为 0）。
4. 写字面头：`sfntVersion` 原样，`numTables` 原样，`searchRange` / `entrySelector` /
   `rangeShift` **重算**，不照抄源文件（源文件不靠谱时会一起错）。
5. 写表记录：`tag`、`length` 原样，`offset` 换成新位置，`checkSum` 重算。
6. `memcpy` 每张表的数据。
7. 若存在 `head`：清零 `checkSumAdjustment` → 重算全部表记录校验和 → 算整文件校验和 → 写回
   `0xB1B0AFBA - sum`。

第 4、5、7 步是本库真正"有知识"的部分；其余都是搬运。

## 6. 命名与 `name` 表

`name` 表结构：

```
0  2  format          0 或 1
2  2  count           记录数
4  2  stringOffset    字符串存储区相对该表起点的偏移
6  12×count  NameRecord
```

每条 NameRecord：`platformID`(2)、`encodingID`(2)、`languageID`(2)、`nameID`(2)、
`length`(2)、`offset`(2)。

- `nameID` 1 = 家族名（Family），2 = 子家族名（Subfamily）。拆分时用家族名做文件名。
- `platformID` 3（Microsoft）+ `encodingID` 1 或 10 时字符串是 **UTF-16BE**；`platformID` 1
  （Mac）时是单字节编码。选择顺序：Windows/en-US 优先，其次其他 Windows 语言，再其次
  Unicode 平台，最后 Mac。
- 字符串可能含 Windows 文件名非法字符（`< > : " / \ | ? *`），写入前要替换并去重。

## 7. 实测范围

本机验证过的字体（Windows 10/11 自带）：`simsun`、`msyh`、`msyhbd`、`msyhl`、`msjh`、
`msjhbd`、`msjhl`、`msgothic`、`mingliub`、`cambria`、`Nirmala`、`YuGothR/M/L/B`，共 15 个
集合、36 个字面，全部通过结构校验（表校验和、整文件校验和、偏移边界、4 字节对齐）。
