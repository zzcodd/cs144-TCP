## 实验概述

![figure.png](https://imgbed001.oss-cn-hangzhou.aliyuncs.com/img/figure1-20240401192709510.png)

当前实验主要是把前两次实验所完成的 `TCPReceiver` 和 `TCPSender` 封装成一个 `TCPConnection`，从而实现完整的收发数据的功能。

可以先预习/复习一下**三次握手**和**四次挥手**的相关概念。

![three-way handshake](https://imgbed001.oss-cn-hangzhou.aliyuncs.com/img/three-way-handshake.png)

![four-way wavehand](https://imgbed001.oss-cn-hangzhou.aliyuncs.com/img/four-way-wavehand.png)

## `TCPConnection` 的实现

还是重新整理一下几个函数实现的要求：

- ```
    segment_received()
    ```

    - 如果接收到的 segment 设置了 `RST`，则将输入输出字节流都设置为 error 状态，并杀掉连接；
    - 否则将 segment 交由**接收器**，由其处理 `seqno`、`SYN`、payload、`FIN`*（`TCPReceiver` 内实现）*。
    - 如果 segment 设置了 `ACK`，通知**发送器**更新 `ackno` 和 `window size`。
    - 如果 segment 包含任意有效 `seqno`，则**发送器**至少回复一个 segment 用来告知此时的 `ackno` 和 `window size`。
    - 如果 segment 包含无效 `seqno`，则**发送器**也需要回复一个无效的 segment（所谓 `keep-alives`）。

- 发送报文（`send_segments()`）

    - 当**发送器**推送 segment 到待发送队列时需要设置相应的 `seqno`、`SYN`、payload、`FIN`*（`TCPSender` 内实现）*。
    - `TCPConnection` 会将**接收器**的 `ackno` 和 `window size` 更新到 segment 中，并设置 `ACK`。

- ```
    tick()
    ```

    - 告知**发送器**流逝的时间。
    - 如果连续重传次数超过 `TCPConfig::MAX_RETX_ATTEMPTS`，终止连接，发送包含 `RST` 标识的空 segment。
    - 执行 clean shutdown。

- ```
    ~TCPConnection()
    ```

    - 执行析构函数时如果连接仍然活跃，发送包含 `RST` 标识的 segment。

关于关闭连接：

- **unclean shutdown**：当发送和接收包含 `RST` 标识的 segment 时，终止连接，设置输入输出字节流为 error 状态。

- **clean shutdown**：

    记得看四次挥手的内容，注意服务端在 `LAST-ACK` 阶段必须要等到客户端返回的 `ACK` 后才会关闭，因此客户端在 `TIME-WAIT` 阶段等待一会就是为了处理来自服务端的 `FIN` 或者重发的 `FIN`。

    - `_linger_after_streams_finish` 开始为 `true`（*可以通过 `state()` 获得*）。如果**输入流**在**输出流**到达 EOF 之前结束，则 `_linger_after_streams_finish` 会被设置为 `false`。（**服务端**）
- 当**客户端**处于 `TIME-WAIT` 时，如果 `_linger_after_streams_finish` 为 `false`，或者已经等待了 `10 * _cfg.rt_timeout`，那么连接可以关闭。