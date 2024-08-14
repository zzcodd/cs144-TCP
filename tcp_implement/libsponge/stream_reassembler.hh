/*
 * @Description: 
 * @Author: zy
 * @Date: 2024-08-04 17:52:13
 * @LastEditTime: 2024-08-12 17:33:59
 * @LastEditors: zy
 */
#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <set>
#include <string>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    struct Segment {
        size_t _idx;
        std::string _data;

        Segment() : _idx(0), _data() {}
        Segment(size_t index, const std::string &data) : _idx(index), _data(data) {}

        size_t length() const { return _data.length(); }
        size_t tail_idx() const { return _idx + length() - 1; }
        // 片段的结束位置

        // 定义了Segment的排序方式，按idx排序。
        bool operator<(const Segment &seg) const { return this->_idx < seg._idx; }
    };

  private:
    // Your code here -- add private members as necessary.

    //重新组装后的字节流会写入到这个ByteStream对象中，供上层代码读取。
    ByteStream _output;  //!< The reassembled in-order byte stream
    // 表示字节流重组器的最大容量
    size_t _capacity;    //!< The maximum number of bytes
    // 标记数据流的结束
    size_t _eof_index;
    // 当前缓冲区未重新组装的字节数
    size_t _unassembled_bytes;
    bool _eof;
    // 用于存储乱序到达但还未被重新组装的字节片段
    std::set<Segment> _buf;

    // 从缓冲区中移除已经被写入到字节流的片段
    void _buf_erase(const std::set<Segment>::iterator &iter);

    // 处理新接收到的字节片段，检查并处理可能得重叠、乱序的情况
    void _handle_substring(Segment &seg);
    // 处理新片段与已经存在的片段的重叠情况，确保数据的唯一性和顺序性
    void _handle_overlap(Segment &seg);

    // 将新的片段插入到缓冲区set中
    void _buf_insert(const Segment &seg);

    // 合并字符串 将重叠的片段整合在一个
    void _merge_seg(Segment &seg1, const Segment &seg2);

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    // 初始化最大容量
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    // 接收一个子串并将新字节按顺序写入到字节流中 超出容量限制的字节将被丢弃
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    // 返回重新组装后的ByteStream对象
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    // 返回当前未被重新组装的字节数
    size_t unassembled_bytes() const;
    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    // 检查stream_reassembler是否已经处理完所有片段
    bool empty() const;

    // 第一个未读取的字节的索引
    size_t first_unread() const { return _output.bytes_read(); }
    // 第一个未被重新组装的字节的索引
    size_t first_unassembled() const { return _output.bytes_written(); }
    // 返回不接受的新字节片段的起始索引，用于流量控制和防止超出容量限制
    size_t first_unacceptable() const { return first_unread() + _capacity; }
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
