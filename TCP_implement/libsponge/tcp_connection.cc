#include "tcp_connection.hh"

#include <iostream>
#include <numeric>
// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity();}

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received;}

bool TCPConnection::active() const { return _isactive; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    if(!_isactive) return;

    _time_since_last_segment_received = 0;

    if(seg.header().rst){
        unclean_shutdown();
        return;
    }

    _receiver.segment_received(seg);
    if(TCPState::state_summary(_receiver) == TCPReceiverStateSummary::SYN_RECV && 
        TCPState::state_summary(_sender) == TCPSenderStateSummary::CLOSED){
            connect();
            return;
    }

    if(seg.header().ack)
        _sender.ack_received(seg.header().ackno, seg.header().win);
    
    if(seg.length_in_sequence_space() > 0 && _sender.segments_out().empty())
        _sender.send_empty_segment();
    
    if(_receiver.ackno().has_value() && seg.length_in_sequence_space() == 0 &&
        seg.header().seqno == _receiver.ackno().value() - 1){
            _sender.send_empty_segment();
        }
    
    send_segment();

    clean_shutdown();

    
}

size_t TCPConnection::write(const string &data) {
    if(!_isactive || data.empty())
        return 0;
    
    size_t len = _sender.stream_in().write(data);
    _sender.fill_window();
    send_segment();
    return len;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if(!active())
        return;
    
    _sender.tick(ms_since_last_tick);
    _time_since_last_segment_received += ms_since_last_tick;
    
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS){
        send_rst_segment();
        unclean_shutdown();
        return;
    }

    send_segment();
    clean_shutdown();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    send_segment();
}

void TCPConnection::connect() {
    _sender.fill_window();
    send_segment();
}

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

void TCPConnection::clean_shutdown()
{
    if(_receiver.stream_out().input_ended() && !_sender.stream_in().eof()){
        _linger_after_streams_finish = false;
    }
    else if(TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_ACKED && 
            TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV){
                if(!_linger_after_streams_finish || _time_since_last_segment_received >= 10 * _cfg.rt_timeout)
                    _isactive = false;
            }
}

void TCPConnection::unclean_shutdown()
{
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _isactive = false;
}

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
