#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity):_capacity(capacity){ }

size_t ByteStream::write(const string &data) {
    size_t writed_num=0;
    while(_buffer.size()<_capacity && writed_num<data.size()){
        _buffer.push_back( data.at(writed_num) );
        writed_num++;
    }
    _total_written+=writed_num;
    return writed_num;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string output(_buffer.begin(),min(_buffer.end(),_buffer.begin()+len));

    return output;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    _total_read += min(_buffer.size(),len);
    _buffer.erase(_buffer.begin(),min(_buffer.end(),_buffer.begin()+len));
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string output= peek_output(len);

    pop_output(len);

    return output;
}

void ByteStream::end_input() { _end_input = true;}

bool ByteStream::input_ended() const { return _end_input; }

size_t ByteStream::buffer_size() const { return _buffer.size(); }

bool ByteStream::buffer_empty() const { return _buffer.empty(); }

bool ByteStream::eof() const { return input_ended()&&_buffer.empty(); }

size_t ByteStream::bytes_written() const { return _total_written; }

size_t ByteStream::bytes_read() const { return _total_read; }

size_t ByteStream::remaining_capacity() const { return _capacity-_buffer.size(); }
