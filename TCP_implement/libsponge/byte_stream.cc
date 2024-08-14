#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

// ByteStream::ByteStream(const size_t capacity) { DUMMY_CODE(capacity); }

ByteStream::ByteStream(const size_t _capacity)
    : _buff(), capacity(_capacity), bytes_r(0), bytes_w(0), _end_input(false), _error(false) {}

size_t ByteStream::write(const string &data) {
    if (input_ended())
        return 0;
    size_t write_size = min(data.size(), remaining_capacity());
    bytes_w += write_size;

    for (size_t i = 0; i < write_size; i++)
        _buff.push_back(data[i]);
    return write_size;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t peek_size = min(len, _buff.size());

    return string(_buff.begin(), _buff.begin() + peek_size);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t pop_size = min(len, _buff.size());
    bytes_r += pop_size;

    for (size_t i = 0; i < pop_size; i++)
        _buff.pop_front();
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string r;
    r = this->peek_output(len);
    this->pop_output(len);

    return r;
}

void ByteStream::end_input() { _end_input = true; }

bool ByteStream::input_ended() const { return _end_input; }

size_t ByteStream::buffer_size() const { return _buff.size(); }

bool ByteStream::buffer_empty() const { return _buff.empty(); }

bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return bytes_w; }

size_t ByteStream::bytes_read() const { return bytes_r; }

size_t ByteStream::remaining_capacity() const { return capacity - _buff.size(); }
