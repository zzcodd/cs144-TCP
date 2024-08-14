#ifndef SPONGE_LIBSPONGE_TCP_RECEIVER_HH
#define SPONGE_LIBSPONGE_TCP_RECEIVER_HH

#include "byte_stream.hh"
#include "stream_reassembler.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <optional>

//! \brief The "receiver" part of a TCP implementation.

//! Receives and reassembles segments into a ByteStream, and computes
//! the acknowledgment number and window size to advertise back to the
//! remote TCPSender.
class TCPReceiver {
    //! Our data structure for re-assembling bytes.
    // 流重组器 将乱序到达的TCP段重新组装成正确的字节流
    StreamReassembler _reassembler;

    //! The maximum number of bytes we'll store.
    // TCPReceiver的最大容量，即他可存储的字节数上限
    size_t _capacity;

    // 记录是否已经接收到SYN段，TCP三次我握手的第一步 SYN段标识着一个新的TCP连接
    bool _syn;
    // 初始序列号ISN，用于序列号的计算和解包
    WrappingInt32 _isn;

  public:
    //! \brief Construct a TCP receiver
    //!
    //! \param capacity the maximum number of bytes that the receiver will
    //!                 store in its buffers at any give time.
    // 构造函数
    TCPReceiver(const size_t capacity) : _reassembler(capacity), _capacity(capacity), _syn(false), _isn(0) {}

    //! \name Accessors to provide feedback to the remote TCPSender
    //!@{

    //! \brief The ackno that should be sent to the peer
    //! \returns empty if no SYN has been received
    //!
    //! This is the beginning of the receiver's window, or in other words, the sequence number
    //! of the first byte in the stream that the receiver hasn't received.
    // 返回当前接收窗口的起始位置 即下一个期望接受的字节的序列号
    std::optional<WrappingInt32> ackno() const;

    //! \brief The window size that should be sent to the peer
    //!
    //! Operationally: the capacity minus the number of bytes that the
    //! TCPReceiver is holding in its byte stream (those that have been
    //! reassembled, but not consumed).
    //!
    //! Formally: the difference between (a) the sequence number of
    //! the first byte that falls after the window (and will not be
    //! accepted by the receiver) and (b) the sequence number of the
    //! beginning of the window (the ackno).
    // 返回当前接收窗口的大小 capacity减去已经存储但尚未被读取的字节数
    size_t window_size() const;
    //!@}

    //! \brief number of bytes stored but not yet reassembled
    // 返回当前存储在_reassembler中尚未重新组装的字节数
    size_t unassembled_bytes() const { return _reassembler.unassembled_bytes(); }

    //! \brief handle an inbound segment
    // 处理接收到的TCP段，检查TCP中的序列号，数据，交给_reassembler进行处理。
    // 如果是SYN段，初始化_isn 并 标记 _syn为true
    void segment_received(const TCPSegment &seg);

    //! \name "Output" interface for the reader
    //!@{
      // 提供给 _reassembler 内部的 ByteStream的访问接口，可以获取已重新组装的字节流
      // 允许外部获取重新组装后的字节流，供上层应用使用
    ByteStream &stream_out() { return _reassembler.stream_out(); }
    const ByteStream &stream_out() const { return _reassembler.stream_out(); }
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_RECEIVER_HH
