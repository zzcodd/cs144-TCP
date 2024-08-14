#ifndef SPONGE_LIBSPONGE_TCP_FACTORED_HH
#define SPONGE_LIBSPONGE_TCP_FACTORED_HH

#include "tcp_config.hh"
#include "tcp_receiver.hh"
#include "tcp_sender.hh"
#include "tcp_state.hh"

//! \brief A complete endpoint of a TCP connection
// 封装TCP协议的接收和发送逻辑，管理连接的状态并处理网络通信

class TCPConnection {
  private:
    TCPConfig _cfg;         // 保存TCP配置的对象，包含接收窗口大小、发送窗口大小、重传超时和其他配置选项
    TCPReceiver _receiver{_cfg.recv_capacity};  // TCP接收器 处理接收到的TCP段 管理接收窗口和字节流的重组
    TCPSender _sender{_cfg.send_capacity, _cfg.rt_timeout, _cfg.fixed_isn};
    //  TCP发送器 负责发送TCP段并管理未确认的TCP段，处理重传逻辑

    //! outbound queue of segments that the TCPConnection wants sent
    // 保存待发送的TCP段的队列
    // 在连接中 所有要发送的TCP段都会被压入这个队列，然后由操作系统或网络栈发送到网络中
    std::queue<TCPSegment> _segments_out{};

    //! Should the TCPConnection stay active (and keep ACKing)
    //! for 10 * _cfg.rt_timeout milliseconds after both streams have ended,
    //! in case the remote TCPConnection doesn't know we've received its whole stream?
    // 控制在所有字节流结束后，是否保持连接一段时间，以确保对方知道我们已收到其发送的数据
    bool _linger_after_streams_finish{true};

    // 表示连接是否依然活跃
    bool _isactive{true};

    // 距离上次接收到TCP段的时间
    size_t _time_since_last_segment_received{0};

  public:
    //! \name "Input" interface for the writer
    //!@{


    //! \brief Initiate a connection by sending a SYN segment
    
    // 输入接口：用于写入器
    // 发送SYN段开启三次握手连接
    void connect();

    //! \brief Write data to the outbound byte stream, and send it over TCP if possible
    //! \returns the number of bytes from `data` that were actually written.
    // 将数据写入发送字节流 
    size_t write(const std::string &data);

    //! \returns the number of `bytes` that can be written right now.
    // 返回发送缓冲区剩余的容量 （可写入的数据量）
    size_t remaining_outbound_capacity() const;

    //! \brief Shut down the outbound byte stream (still allows reading incoming data)
    // 关闭发送字节流，但可以接收
    void end_input_stream();
    //!@}

    //! \name "Output" interface for the reader
    //!@{

    //! \brief The inbound byte stream received from the peer
    
    // 输出接口：用于读取器
    // 返回接收到的字节流，可以从中读取已经接收到并且重组的数据
    ByteStream &inbound_stream() { return _receiver.stream_out(); }
    //!@}

    //! \name Accessors used for testing

    //!@{
    //! \brief number of bytes sent and not yet acknowledged, counting SYN/FIN each as one byte
    // 已发送但未被确认的字节数
    size_t bytes_in_flight() const;
    //! \brief number of bytes not yet reassembled
    // 未被重组的字节数
    size_t unassembled_bytes() const;
    //! \brief Number of milliseconds since the last segment was received
    // 距离上次接收到TCP段的时间（毫秒）
    size_t time_since_last_segment_received() const;
    //!< \brief summarize the state of the sender, receiver, and the connection
    // 返回TCP连接的状态，包括发送器、接收器的状态，以及连接是否依然活跃
    TCPState state() const { return {_sender, _receiver, active(), _linger_after_streams_finish}; };
    //!@}

    //! \name Methods for the owner or operating system to call
    //!@{

    //! Called when a new segment has been received from the network
    // 当接收到新的TCP段时调用，处理这个段并更新连接状态
    void segment_received(const TCPSegment &seg);

    //! Called periodically when time elapses
    // 定期调用该方法模拟时间流逝
    void tick(const size_t ms_since_last_tick);

    //! \brief TCPSegments that the TCPConnection has enqueued for transmission.
    //! \note The owner or operating system will dequeue these and
    //! put each one into the payload of a lower-layer datagram (usually Internet datagrams (IP),
    //! but could also be user datagrams (UDP) or any other kind).
    // 返回待发送的TCP段队列，操作系统或网络栈会从这个队列中获取段并发送到网络上
    std::queue<TCPSegment> &segments_out() { return _segments_out; }

    //! \brief Is the connection still alive in any way?
    //! \returns `true` if either stream is still running or if the TCPConnection is lingering
    //! after both streams have finished (e.g. to ACK retransmissions from the peer)
    // 检查连接是否活跃
    bool active() const;
    //!@}

    //! Construct a new connection from a configuration
    explicit TCPConnection(const TCPConfig &cfg) : _cfg{cfg} {}

    //! \name construction and destruction
    //! moving is allowed; copying is disallowed; default construction not possible

    //!@{
      // 析构函数 如果连接依然打开则发送一个RST强制关闭连接
    ~TCPConnection();  //!< destructor sends a RST if the connection is still open
    TCPConnection() = delete;
    TCPConnection(TCPConnection &&other) = default;
    TCPConnection &operator=(TCPConnection &&other) = default;
    TCPConnection(const TCPConnection &other) = delete;
    TCPConnection &operator=(const TCPConnection &other) = delete;
    //!@}
    // 发送一个TCP段加入待发送的队列中
    void send_segment();
    // 发送一个RST段
    void send_rst_segment();
    // 清理关闭连接的逻辑 确保连接正常关闭
    void clean_shutdown();
    // 不正常关闭连接逻辑，可能由于错误或者异常导致
    void unclean_shutdown();
};

#endif  // SPONGE_LIBSPONGE_TCP_FACTORED_HH
