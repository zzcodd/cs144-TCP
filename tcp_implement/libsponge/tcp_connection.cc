#include "tcp_connection.hh"

#include <iostream>
#include <numeric>
// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

// 当前发送缓冲区剩余的容量
size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity();}

// 已发送但未收到确认的字节数
size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

// 接收器中未重组的字节数
size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

// 自从上次收到TCP段以来的时间（ms）
size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received;}

// 处理超时或判断连接是否需要保持活跃
bool TCPConnection::active() const { return _isactive; }

/*
  处理接收到的数据段
  首先判断是否活跃，时间设置为0 表示刚接受一个段
  rst 则强制关闭
  将值传递给接收器_receiver进行处理
*/
void TCPConnection::segment_received(const TCPSegment &seg) {
    if(!_isactive) return;

    _time_since_last_segment_received = 0;

    if(seg.header().rst){
        unclean_shutdown();
        return;
    }

    _receiver.segment_received(seg);
    // 表明接收方已经收到对方的SYN段并发送SYN-ACK 当前正处于SYN_RCVD状态
     // 发送方还没有发送任何数据 处于CLOSED状态
    if(TCPState::state_summary(_receiver) == TCPReceiverStateSummary::SYN_RECV && 
        TCPState::state_summary(_sender) == TCPSenderStateSummary::CLOSED){
            connect();
            return;
    }

// 如果收到ack标志，表明对方确认一些数据接收，更新发送器的状态，移除已经确认的段
    if(seg.header().ack)
        _sender.ack_received(seg.header().ackno, seg.header().win);
    
    // 如果接收实际的数据 但 发送器没有待发送数据 发送一个空段为了保持连接活跃或者是对接受的段作出响应
    if(seg.length_in_sequence_space() > 0 && _sender.segments_out().empty())
        _sender.send_empty_segment();
    
    // 如果接收的段没有数据（长度为0） 但它的序列号正好是接收器的期望值，则发送一个空段。
    // 这是TCP处理窗口探测的常见逻辑 发送一个空段作出响应 回复窗口大小等。。。

    //如果远端发送一个包含无效seqno的segment，则必须回复一个包含无效seqno的包，来确认双方是否有效。
    //同时可以查看彼此的ackno和window size，这称为“keep-alive”机制。实现代码如下
    if(_receiver.ackno().has_value() && seg.length_in_sequence_space() == 0 &&
        seg.header().seqno == _receiver.ackno().value() - 1){
            _sender.send_empty_segment();
        }
    
    // 发送所有待发送的段
    send_segment();

    // 正常关闭
    clean_shutdown();

    
}

// 将数据写入到TCP连接中 并发送数据
size_t TCPConnection::write(const string &data) {
    if(!_isactive || data.empty())
        return 0;
    // 数据写入字节流中
    size_t len = _sender.stream_in().write(data);
    _sender.fill_window();
    send_segment();
    return len;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
// 定时器每次滴答时调用 用于处理重传和检查连接状态
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if(!active())
        return;
    
    _sender.tick(ms_since_last_tick);
    _time_since_last_segment_received += ms_since_last_tick;
    
    // 重传次数过多 发送RST 并强制关闭
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS){
        send_rst_segment();
        unclean_shutdown();
        return;
    }

    send_segment();
    clean_shutdown();
}

// 结束输入流 首先表示输入流结束FIN 将剩余数据填充窗口发送出去包括FIN段，发送所有准备好的数据段
void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    send_segment();
}
// 启动TCP连接 发送SYN段
void TCPConnection::connect() {
    _sender.fill_window();
    send_segment();
}

// 将_sender设置好的数据段发送出去 并设置ACK 窗口大小等头部字段
void TCPConnection::send_segment() {
    while(!_sender.segments_out().empty()){
        TCPSegment seg = _sender.segments_out().front();
        _sender.segments_out().pop();

        if(_receiver.ackno().has_value()){
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = (_receiver.window_size() <= numeric_limits<uint16_t>::max()) ? 
                                                            _receiver.window_size() : numeric_limits<uint16_t>::max();
        }

        _segments_out.push(seg);
    }
}

// 当连接需要终止或遇到错误 发送RST段
// 如果我们fill_window后依然没数据，说明发送器没准备好数据段，这时我们需要发送一个空的RST段
void TCPConnection::send_rst_segment()
{
    _sender.fill_window();
    if(_sender.segments_out().empty()){
        _sender.send_empty_segment();
        return;
    }

    TCPSegment seg = _sender.segments_out().front();
    _sender.segments_out().pop();
    if(_receiver.ackno().has_value()){
        seg.header().ack = true;
        seg.header().ackno = _receiver.ackno().value();
        seg.header().win = (_receiver.window_size() <= numeric_limits<uint16_t>::max()) ? 
                                                _receiver.window_size() : numeric_limits<uint16_t>::max();
    }
    seg.header().rst = true;

    _segments_out.push(seg);
}

// 正常关闭
void TCPConnection::clean_shutdown()
{
    // 第一步检查：接收器的字节流是否已经结束，且发送器的字节流还没有到达EOF
    if (_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) {
        // 如果接收器已经接收到对方的FIN（即数据流结束），但发送器的数据还未发送完，
        // 那么关闭linger_after_streams_finish，表示不再等待
        _linger_after_streams_finish = false;
    }
    // 第二步检查：如果发送方的状态是FIN_ACKED，且接收方的状态是FIN_RECV
    // 这意味着两边的FIN都已经被发送并且被对方接收到和确认。
    else if (TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_ACKED && 
             TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV) {
        
        // 第三个检查条件：_linger_after_streams_finish为false，或者已经等待了足够长的时间
        // 如果linger_after_streams_finish为false，或者已经等待了超过10倍的重传超时时间
        // 那么就将_isactive设置为false，表示连接已经完全关闭
        if (!_linger_after_streams_finish || _time_since_last_segment_received >= 10 * _cfg.rt_timeout) {
            _isactive = false;
        }
    }
}

// 处理非正常关闭 将发送器和接收器的字节流设置为错误 并将连接标记为不活跃
void TCPConnection::unclean_shutdown()
{
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _isactive = false;
}

// 析构函数 ： 在连接依然活跃时发送警告并发送rst段进行非正常关闭
TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            send_rst_segment();
            unclean_shutdown();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
