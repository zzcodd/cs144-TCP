#include "wrapping_integers.hh"

#include <iostream>
// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    uint32_t number = isn.raw_value() + static_cast<uint32_t>(n);
    // 64位截断程 32 位
    return WrappingInt32{number};
}
// 将64位绝对序列号n转换为32位整数，与isn的原始值相加，得到相对序列号。


//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
// 32位转换为 64位
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint32_t offset = n - wrap(checkpoint, isn);
    // 计算n与checkpoint的偏移量
    uint64_t pos = checkpoint + offset;

  // 如果偏移量的绝对值非常大 超过了32位的中间值 且 结果超过了32位，则将pos-2^32 
  // 确保最终结果尽量靠近checkpoint 
    if (offset > (1u << 31) && pos >= (1ul << 32))
        pos -= (1ul << 32);

    return pos;
}
