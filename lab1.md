Lab1 中首先说明了当前以及接下去几次实验需要实现的内容。

![figure.png](https://imgbed001.oss-cn-hangzhou.aliyuncs.com/img/figure1.png)

- *Lab0 已经实现 `ByteStream`。*
- **当前实验：Lab1 需要实现 `StreamReassembler`。**
- Lab2 需要实现 `TCPReceiver`。
- Lab3 需要实现 `TCPSender`。
- Lab4 需要实现 `TCPConnection` 用来把以上模块都关联起来。

## Putting substrings in sequence

实验要求的描述其实多少有些让人迷惑，很多地方需要仔细理解一下才行。

同时可以先预/复习有关**滑动窗口**的内容，很多思想其实都相似的。

因为接受到的 segments 可能会出现乱序、丢失、重复、交叉重叠等情况，因此我们要在当前实验中要实现一个流重组器，将收到的字节流中的 segments 拼接还原为其原本正确的顺序。

## Substrings

暂且称为字符串片段好了，代码中用 `segment` 或 `seg` 来表示单个片段。

一些对实验描述的理解如下：

- `push_substring()` 中包含三个参数：字符串片段、起始索引和 `eof` 标记。
- 实验要求索引从**零**开始计数。
- 片段之间会**重叠**（*这里重叠部分数据顺序默认是一致的*），并且可以是**乱序**到达的。
- 起始索引符合 `first unassembled` 的字符串片段会被立即写入 `ByteStream`。
- 当参数 `eof` 为 `true` 时，缓冲区中之前的部分可能还存在一些空缺，需要继续等待新片段。

## What’s the “capacity”

![capacity.png](https://imgbed001.oss-cn-hangzhou.aliyuncs.com/img/capacity.png)

首先要搞清楚实验要求中 `capacity` 究竟是表达什么意思：

1. `ByteStream` 的空间**上限**是 `capacity`。
2. `StreamReassembler` 用于暂存未重组字符串片段的缓冲区空间**上限**也是 `capacity` 。
3. **绿色**部分代表了 `ByteStream` 中已经重组并写入但还未被读取的字节流所占据的空间大小。
4. **红色**部分代表了 `StreamReassembler` 中已经缓存但未经重组的若干字符串片段所占据的空间大小。
5. 同时**绿色**和**红色**两部分加起来的空间总占用大小不会超过 `capacity`（*事实上会一直小于它*）。

此外：

- `first unread` 的索引等于 `ByteStream` 的 `bytes_read()` 函数的返回值。
- `first unassembled` 的索引等于 `ByteStream` 的 `bytes_write()` 函数的返回值。
- `first unacceptable` 的索引等于 `ByteStream` 的 `bytes_read()` 加上 `capacity` 的和。
- `first unread` 和 `first unacceptable` 这两个边界是动态变化的。

## `StreamReassembler` 的实现

**代码详细可见源码文件**

一些注意事项：

1. 需要注意处理 `data` 为空，`eof` 为 `true` 的情况。
2. `size_t` 默认为 `unsigned long` 类型，`0 - 1` 的结果可不是 `-1`。