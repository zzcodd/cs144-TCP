#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _last_ackno; }

void TCPSender::fill_window() {
    TCPSegment seg;

    if (next_seqno_absolute() == 0) {
        seg.header().syn = true;
        send_segment(seg);
        return;
    } else if (next_seqno_absolute() > 0 && next_seqno_absolute() == bytes_in_flight()) {
        return;
    }

    uint16_t cur_win = _last_win == 0 ? 1 : _last_win;

    size_t remaining_win = cur_win - bytes_in_flight();
    while ((remaining_win = cur_win - bytes_in_flight())) {
        if (!stream_in().eof() && next_seqno_absolute() > bytes_in_flight()) {
            size_t payload_size = min(TCPConfig::MAX_PAYLOAD_SIZE, remaining_win);
            seg.payload() = Buffer(stream_in().read(payload_size));

            if (stream_in().eof() && seg.length_in_sequence_space() < remaining_win)
                seg.header().fin = true;
            if (seg.length_in_sequence_space() == 0)
                return;
            send_segment(seg);
        } else if (stream_in().eof()) {
            if (next_seqno_absolute() < stream_in().bytes_written() + 2) {
                seg.header().fin = true;
                send_segment(seg);
                return;
            } else
                return;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t abs_ack = unwrap(ackno, _isn, _last_ackno);

    if (abs_ack > _next_seqno)
        return;

    if (abs_ack > _last_ackno) {
        _last_ackno = abs_ack;

        while (!_outstanding_seg.empty()) {
            const TCPSegment &seg = _outstanding_seg.front();
            if (seg.header().seqno.raw_value() + seg.length_in_sequence_space() <= ackno.raw_value())
                _outstanding_seg.pop();
            else
                break;
        }

        _retransmission_count = 0;
        _RTO = _initial_retransmission_timeout;

        if (!_outstanding_seg.empty())
            _timer.start(_RTO);
        else
            _timer.stop();
    }

    _last_win = window_size;
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _timer.tick(ms_since_last_tick);

    if (!_outstanding_seg.empty() && _timer.is_expired()) {
        _segments_out.push(_outstanding_seg.front());
        if (_last_win > 0) {
            _retransmission_count++;
            _RTO *= 2;
        }

        _timer.start(_RTO);
    } else if (_outstanding_seg.empty())
        _timer.stop();
}

unsigned int TCPSender::consecutive_retransmissions() const { return _retransmission_count; }

void TCPSender::send_segment(TCPSegment &seg) {
    seg.header().seqno = next_seqno();
    _next_seqno += seg.length_in_sequence_space();
    _segments_out.push(seg);
    _outstanding_seg.push(seg);

    if (!_timer.is_start()) {
        _timer.start(_RTO);
    }
}

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    _segments_out.push(seg);
}
