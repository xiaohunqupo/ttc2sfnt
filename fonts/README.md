# 测试字体目录（不提交字体文件）

`.ttc` 多为厂商专有字体，所以仓库不附带字体。这里可以放任意 `.ttc`，供
`test/test_ttc.cpp` 的"真实集合"一节使用；`.gitignore` 已忽略本目录下的
`*.ttc` / `*.ttf` / `*.otf`。

测试取语料的顺序：

1. `CMakeLists.txt` 把 `fonts/*.ttc` 通配后作为参数传给 `ttc_tests`（`ctest` 会自动带上）；
2. 命令行没有可用路径时，看环境变量 `TTC2SFNT_TEST_FONT`；
3. 再退回 `fonts/simsun.ttc`、`fonts/msyh.ttc`；
4. 最后退回 `C:/Windows/Fonts/simsun.ttc`、`C:/Windows/Fonts/msgothic.ttc`（Windows 上）。

都找不到时该节打印 `SKIP`，其余用例照常执行——合成夹具（`fixture::make_ttc()`）会在没有
任何字体文件的情况下覆盖拆分、命名、落盘、畸形输入与公开类接口的全部分支。
