# lrg —— 简化版 grep

用 C 写的简化版 `grep`，在 Linux 上运行。支持 `-n` / `-i` / `-c`，可处理多个文件。

## 构建

需要 Linux、gcc、make。

```bash
make         # 生成 ./lrg
make test    # 构建并运行测试
make clean   # 删除 lrg 和所有 .o
```

Makefile 采用 `.c → .o → lrg` 两阶段构建，`-MMD -MP` 自动生成头文件依赖。

## 使用

```
lrg [-nic] PATTERN [FILE...]
```

| 选项 | 含义                                        |
| ---- | ------------------------------------------- |
| `-n` | 每行前输出行号，从 1 开始，每个文件独立计数 |
| `-i` | 匹配时忽略 ASCII 字母大小写                 |
| `-c` | 只输出匹配行数，与 `-n` 同时出现时忽略 `-n` |

- 选项可合并：`-ni` 等价于 `-n -i`
- 遇到第一个不以 `-` 开头的参数即 `PATTERN`，其后都是 `FILE`
- 省略 `FILE` 时读标准输入；`PATTERN` 是字面量，空串匹配所有行

退出码：`0` 有匹配且无错，`1` 无匹配且无错，`2` 发生了错误。

## 示例

```
$ ./lrg boot tests/sample.log
boot ok
boot ok again
```

```
$ ./lrg -n -i error tests/sample.log
3:ERROR: retry later
5:error: give up
```

```
$ ./lrg -ci error tests/sample.log tests/notes.txt
tests/sample.log:2
tests/notes.txt:1
```

```
$ ./lrg error tests/sample.log no-such-file; echo "exit=$?"
tests/sample.log:error: give up
lrg: no-such-file: No such file or directory
exit=2
```

## 设计

### 模块划分

| 文件           | 职责                                   |
| -------------- | -------------------------------------- |
| `src/main.c`   | 参数解析、匹配、输出、遍历文件、退出码 |
| `src/reader.c` | 流式读取器：块缓冲 + 行缓冲            |
| `src/reader.h` | 读取器接口（三个函数）                 |

### 行长不限 + 内存不随文件增长

两层缓冲：

| 缓冲   | 作用           | 实现           |
| ------ | -------------- | -------------- |
| 块缓冲 | 装文件的一"块" | 64 KiB 的数组 |
| 行缓冲 | 装当前这一行   | 动态数组       |

- 块缓冲固定，读满就消费掉再读下一块
- 一行跨多块时把各块内容续到行缓冲，装不下就扩容

### 参数解析

外层遍历 `argv`，内层遍历合并的选项字符；扫到第一个非选项参数即停止

### 匹配

朴素的字面量匹配，逐位置逐字节比较。`-i` 只折叠 ASCII 的 A-Z。

## 错误处理

| 错误情况                | 行为                                                                            | 退出码   |
| ----------------------- | ------------------------------------------------------------------------------- | -------- |
| 非法选项 / 缺少 PATTERN | 输出 `usage: lrg [-nic] PATTERN [FILE...]` 到标准错误，立即退出，不处理任何文件 | 2        |
| 文件打不开 / 读取失败   | 输出 `lrg: 文件名: strerror(errno)` 到标准错误，继续处理后续文件                | 2        |
| `read` 被信号打断       | 重试，不计为错误                                                                | 不受影响 |
| 内存不足                | 同"文件打不开"，继续处理后续文件                                                | 2        |
| 没有匹配行              | —                                                                               | 1        |
| 有匹配且全程无错        | —                                                                               | 0        |

## 测试

```bash
make test
```

`tests/run_tests.sh` 共 18 条用例，覆盖输出格式、三个选项、多文件前缀、标准输入、退出码、读行边界（空文件 / 空行）、1 MiB 超长行。

```
$ make test
  ok   基本匹配
  ok   带行号 -n
  ok   忽略大小写 -i
  ok   多文件带前缀
  ok   空 pattern 匹配全部
  ok   -c 单个输入
  ok   -c 多个文件
  ok   stdin 不加前缀
  ok   有匹配 0
  ok   无匹配 1
  ok   缺 pattern 2
  ok   非法选项 2
  ok   打不开 2
  ok   空文件 0 行
  ok   两个空行 2 字节
  ok   混合空行 3 行
  ok   1 MiB 不截断
  ok   1 MiB 只有 1 行
全部通过
```

测试数据在 `tests/` 下。

## 已知限制

匹配为朴素算法，最坏复杂度 O(行长 × pattern 长度)。

## 参考资料

- man 页：`grep(1)`、`open(2)`、`read(2)`、`close(2)`、`errno(3)`、`make(1)`
- CSAPP 第 10 章（系统级 I/O）
