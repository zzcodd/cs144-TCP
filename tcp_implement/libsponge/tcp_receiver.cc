/*
 * @Description: 
 * @Author: zy
 * @Date: 2024-08-04 17:52:13
 * @LastEditTime: 2024-08-13 08:48:38
 * @LastEditors: zy
 */
/*
 * @Description: 
 * @Author: zy
 * @Date: 2024-08-04 17:52:13
 * @LastEditTime: 2024-08-12 17:47:25
 * @LastEditors: zy
 */
#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;


// 如果syn为false 表示可能是初始阶段 为第一个TCP段 如果此段不是syn 直接返回
// 如果第一个段是syn 记录下段的序列号作为初始序列号_isn 将_syn 设置为true
// 将SYN段的数据传递给StreamReassembler 索引为0
void TCPReceiver::segment_received(const TCPSegment &seg) {
    if (!_syn) {
        if (!seg.header().syn)
            return;
        _isn = seg.header().seqno;
        _syn = seg.header().syn;
        // .palyroad() 是获取TCP段中实际传输的数据载荷
        _reassembler.push_substring(seg.payload().copy(), 0, seg.header().fin);
        return;
    }

// unwrap 函数将接收到的段的序列号seqno解包成绝对序列号abs_seqno 64位无符号整数
// 将段中的数据及绝对序列号传给StreamReassembler进行重组
    uint64_t abs_seqno = unwrap(seg.header().seqno, _isn, _reassembler.first_unassembled());

    _reassembler.push_substring(seg.payload().copy(), abs_seqno - 1, seg.header().fin);
}

// 计算当前需要发送给远程确认方的确认号
// 确认号 = 第一个未重组的字节的索引+1 ； 表明期望接收到的下一个字节的序列号
optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_syn)
        return std::nullopt;

    uint64_t _ackno = _reassembler.first_unassembled() + 1;

  // 返回reassembler中的stream_out接口输出ByteStream 看是否输入结束
  // 如果结束 确认号再加1 以包括 FIN标识
    if (_reassembler.stream_out().input_ended())
        _ackno += 1;

  // 使用wrap函数将64位的确认号_ackno 转换为 32位的 WrappingInt32
    return wrap(_ackno, _isn);
}

size_t TCPReceiver::window_size() const { return _reassembler.first_unacceptable() - _reassembler.first_unassembled(); }
