#ifndef SPONGE_LIBSPONGE_BYTE_STREAM_HH
#define SPONGE_LIBSPONGE_BYTE_STREAM_HH

#include <deque>
#include <string>
//! \brief An in-order byte stream.

//! Bytes are written on the "input" side and read from the "output"
//! side.  The byte stream is finite: the writer can end the input,
//! and then no more bytes can be written.
class ByteStream {
  private:
    // Your code here -- add private members as necessary.

    // Hint: This doesn't need to be a sophisticated data structure at
    // all, but if any of your tests are taking longer than a second,
    // that's a sign that you probably want to keep exploring
    // different approaches.
    // 双端队列用于存储字节流中的数据。 容量 读取的字节数 写入的字节数 
    // 流结束表示为true 就无法再写入数据 error标识符标识流是否发生错误
    std::deque<char> _buff;
    size_t capacity;
    size_t bytes_r;
    size_t bytes_w;
    bool _end_input;
    bool _error{};  //!< Flag indicating that the stream suffered an error.

  public:
    //! Construct a stream with room for `capacity` bytes.
    // 初始化ByteStream对象，并设置容量
    ByteStream(const size_t capacity);

    //! \name "Input" interface for the writer
    //!@{

    //! Write a string of bytes into the stream. Write as many
    //! as will fit, and return how many were written.
    //! \returns the number of bytes accepted into the stream
    // 将字符串中的字节写入字节流
    size_t write(const std::string &data);

    //! \returns the number of additional bytes that the stream has space for
    size_t remaining_capacity() const;

    //! Signal that the byte stream has reached its ending
    // 标记输入结束，不允许再写入新的字节
    void end_input();

    //! Indicate that the stream suffered an error.
    // 标记字节流发生错误
    void set_error() { _error = true; }
    //!@}

    //! \name "Output" interface for the reader
    //!@{

    //! Peek at next "len" bytes of the stream
    //! \returns a string
    // 查看但不移除字节流前n个字节
    std::string peek_output(const size_t len) const;

    //! Remove bytes from the buffer
    //  从字节流中移除前n个字节
    void pop_output(const size_t len);

    //! Read (i.e., copy and then pop) the next "len" bytes of the stream
    //! \returns a string
    // 读取前n个字节，复制并移除
    std::string read(const size_t len);

    //! \returns `true` if the stream input has ended
    // 返回true表示输入已经结束
    bool input_ended() const;

    //! \returns `true` if the stream has suffered an error
    // 返回true表示字节流发生错误
    bool error() const { return _error; }

    //! \returns the maximum amount that can currently be read from the stream
    // 返回当前缓冲区的字节数
    size_t buffer_size() const;

    //! \returns `true` if the buffer is empty
    // 返回true表示缓冲区为空
    bool buffer_empty() const;

    //! \returns `true` if the output has reached the ending
    // 返回true表示已经读取到字节流的末尾 并且缓冲区为空
    bool eof() const;
    //!@}

    //! \name General accounting
    //!@{
// 通用接口 返回总共写入的字节数和总共读取的字节数

    //! Total number of bytes written
    size_t bytes_written() const;

    //! Total number of bytes popped
    size_t bytes_read() const;
    //!@}
};

#endif  // SPONGE_LIBSPONGE_BYTE_STREAM_HH
