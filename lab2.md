## 实验概述

![figure.png](https://imgbed001.oss-cn-hangzhou.aliyuncs.com/img/figure1-20240401192128778.png)

当前实验需要实现一个 TCP 接收器，用于把接收到的 TCP segments 送入 Lab1 中实现的 `StreamReassembler`，之后再写入到 Lab0 中实现的 `ByteStream`。同时，接收器还要给发送方返回**确认号（acknowledgment number, ackno）**和**窗口大小（window size）**。以上两者描述了接收方的*接收窗口*并用于*流量控制*。

## Translating between 64-bit indexes and 32-bit seqnos

首先需要实现的是 64 位与 32 位整型之间的相互转换。

实验说明中的例子如图：

![Lab2-fig1.png](https://imgbed001.oss-cn-hangzhou.aliyuncs.com/img/lab2-fig1.png)



几点说明：

- 头文件 `wrapping_integers.hh` 中已经实现了一些运算符重载函数来方便我们使用。
- `unwrap()` 函数需要计算 `n` 所对应的**最接近** `checkpoint` 的 `absolute seqno`，可以通过计算 **偏移量（offset）** 来实现。
- 形参 `checkpoint` 是 `absolute seqno`，代表最后一个重组字节的序号（*the index of the last reassembled byte*）。
- `unwrap()` 中 `n` 所代表的位置既可以在 `checkpoint` 的左侧，也可以在 `checkpoint` 右侧，同时又是无符号数相减，因此要仔细考虑以上两种情况（*具体可以参考代码中的注释*）。

## Implementing the TCP receiver

接下来就是要实现 TCP 接收器。我们需要实现三个函数，同时在实现中需要考虑以下几种状态的变化。

![evolution.png](https://imgbed001.oss-cn-hangzhou.aliyuncs.com/img/evolution.png)

如上图所示，实验说明中给出了 TCP 接收器在连接过程中的演变。除了 `ERROR` 状态外，其他三种状态为：

- `LISTEN`：等待 `SYN` 到达，在此之前到达的数据都会被丢弃。
- `SYN_RECV`：正常接受数据。
- `FIN_RECV`：获取 `FIN`，结束输入。


一些说明：

- `segment_received()` 中需要留意 `SYN` 标志为 `true` 的起始情况。
- 如之前 `unwrap()` 中说明的那样，`segment_received()` 中的 `checkpoint` 为 **the index of last reassembled byte**。
- `window_size()` 其实就是求 `StreamReassembler` 能接受的空间大小上限（*Lab1 中 `capacity` 图中红色部分的极限大小*）。