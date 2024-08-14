#include "stream_reassembler.hh"

#include <iostream>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), _eof_index(0), _unassembled_bytes(0), _eof(false), _buf() {}

void StreamReassembler::_buf_erase(const set<Segment>::iterator &iter) {
    _unassembled_bytes -= iter->length();
    _buf.erase(iter);
}

void StreamReassembler::_buf_insert(const Segment &seg) {
    _unassembled_bytes += seg.length();
    _buf.insert(seg);
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (eof) {  // 若eof==true 表明到达标志位，计算出结束下标并将标志符置为true
        _eof_index = index + data.size();
        _eof = true;
    }

    if (!data.empty()) {
        Segment seg{index, data};
        _handle_substring(seg);
    }

    while (!_buf.empty() && _buf.begin()->_idx == first_unassembled()) {
        const auto &iter = _buf.begin();
        _output.write(iter->_data);
        _buf_erase(iter);
    }

    if (_eof && first_unassembled() == _eof_index) {
        _output.end_input();
        _buf.clear();
    }
}

void StreamReassembler::_handle_substring(Segment &seg) {
    // 共分为3种情况：
    // 1.index越界，直接丢弃
    // 2.部分数据越界， 裁掉一部分
    // 3.都在范围内
    const size_t index = seg._idx;

    // 丢弃
    if (index >= first_unacceptable() || seg.tail_idx() < first_unassembled())
        return;

    // 截断

    if (index < first_unassembled() && seg.tail_idx() >= first_unassembled()) {
        seg._data = seg._data.substr(first_unassembled() - index);
        seg._idx = first_unassembled();
    }

    if (index < first_unacceptable() && seg.tail_idx() >= first_unacceptable())
        seg._data = seg._data.substr(0, first_unacceptable() - index);

    if (_buf.empty()) {
        _buf_insert(seg);
        return;
    }

    _handle_overlap(seg);
}

void StreamReassembler::_handle_overlap(Segment &seg) {
    const size_t seg_index = seg._idx;
    const size_t seg_tail = seg.tail_idx();

    for (auto iter = _buf.begin(); iter != _buf.end();) {
        const size_t buf_index = iter->_idx;
        const size_t buf_tail = iter->tail_idx();

        if ((seg_index <= buf_tail && seg_index >= buf_index) || (seg_tail >= buf_index && seg_index <= buf_index)) {
            _merge_seg(seg, *iter);
            _buf_erase(iter++);
        } else
            ++iter;
    }

    _buf_insert(seg);
}

void StreamReassembler::_merge_seg(Segment &seg1, const Segment &seg2) {
    // seg1是传入的字符串段，seg2是_buf中的字符串段
    const size_t seg1_tail = seg1.tail_idx();
    const size_t seg2_tail = seg2.tail_idx();

    if (seg1._idx < seg2._idx && seg1_tail <= seg2_tail) {
        seg1._data = seg1._data.substr(0, seg2._idx - seg1._idx) + seg2._data;
    } else if (seg1._idx >= seg2._idx && seg1_tail > seg2_tail) {
        seg1._data = seg2._data + seg1._data.substr(seg2._idx + seg2.length() - seg1._idx);
        seg1._idx = seg2._idx;
    } else if (seg1._idx >= seg2._idx && seg1_tail <= seg2_tail) {
        seg1._data = seg2._data;
        seg1._idx = seg2._idx;
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _buf.empty(); }
