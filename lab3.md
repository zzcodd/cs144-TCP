## 实验概述

![figure.png](https://imgbed001.oss-cn-hangzhou.aliyuncs.com/img/figure1-20240401192309650.png)

当前实验需要实现一个 TCP 发送器，但是一个难点在于它的实验描述比较分散，要把几部分要求合起来才是一个完整的功能，因此容易让人无从下手。

可以先预习/复习一下 **连续 ARQ**、**Go-Back-N** 的相关内容。

## How does the `TCPSender` know if a segment was lost?

第一部分要求实现一个重传定时器（retransmission timer）。

实验说明中提到与定时器相关的几点：

- 定时器运行时间由 RTO (retransmission timeout) 决定。
- 每当一个包含数据的 segment 被发送时，如果定时器没有运行，则启动定时器。
- 当所有的 `outstanding segments` 都被接收并确认时，停止定时器。

其余定时器说明合并到下面的函数实现要求中，其真实设计可以查看 [RFC 6298](https://datatracker.ietf.org/doc/rfc6298/)。

定时器的功能比较简单，具备启动、停止以及判断是否过期即可。

**定时器的具体设计（放在 tcp_sender.hh 中）**


## Implementing the TCP sender

把实验说明重新读了几遍后来梳理一下 `fill_window()`、`ack_received()` 和 `tick()` 需要实现的内容：

- ```
    fill_window()
    ```

    - `TCPSegment` 在此处生成。
    - 每个 `TCPSegment` 的 payload 长度最大不超过 `TCPConfig::MAX_PAYLOAD_SIZE`。
    - 如果 `window size` 大小为 `0`，发送方应按照接收窗口为 `1` 的情况持续发包。
    - 可以参考实验说明中提供的状态变化来设计。![evolution of TCP sender](https://imgbed001.oss-cn-hangzhou.aliyuncs.com/img/evolution-of-sender.jpg)

- ```
    tick()
    ```

    - 如果定时器正在运行且已经过期：

        - 重传最早未被确认的片段。

        - 如果`window size`非空：
            - 记录并增加连续重传次数。
            - RTO 翻倍（*称为 “exponential backoff”，用来减慢糟糕网络上的重传速度，以避免加剧拥堵情况*）。
        
    - 重置并重启计时器（*此时定时器使用的 RTO 已经变化*)。
    
- ```
    ack_received()
    ```

    - 会根据接收方返回的 `ackno` 丢弃已经被确认的 `outstanding segments`。

    - 如果接收到的`ackno`大于任何之前的`ackno`：

      - 重置 RTO 为初始值。
      - 重置连续重传计数为 `0`。
      - 如果发送方此时还有剩余的 `outstanding segments`，重启定时器。
      
    - 如果 `window size` 有空余空间，继续发包。

《自顶向下》这本书中提供了伪代码可以被用作以上三个函数实现的参考：

![retransmission timer](https://imgbed001.oss-cn-hangzhou.aliyuncs.com/img/retransmission-timer.png)