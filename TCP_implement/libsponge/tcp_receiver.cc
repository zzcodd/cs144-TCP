#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if (!_syn) {
        if (!seg.header().syn)
            return;
        _isn = seg.header().seqno;
        _syn = seg.header().syn;
        _reassembler.push_substring(seg.payload().copy(), 0, seg.header().fin);
        return;
    }

    uint64_t abs_seqno = unwrap(seg.header().seqno, _isn, _reassembler.first_unassembled());

    _reassembler.push_substring(seg.payload().copy(), abs_seqno - 1, seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_syn)
        return std::nullopt;

    uint64_t _ackno = _reassembler.first_unassembled() + 1;

    if (_reassembler.stream_out().input_ended())
        _ackno += 1;

    return wrap(_ackno, _isn);
}

size_t TCPReceiver::window_size() const { return _reassembler.first_unacceptable() - _reassembler.first_unassembled(); }
