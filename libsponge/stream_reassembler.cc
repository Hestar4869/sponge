#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    int n = data.size();
    for (int i = 0; i < n; ++i) {
        // 丢弃已经加入字节流的序列号
        if (i+index < _nindex)
            continue ;
        _unassembled_bytes[i + index] = data[i];
    }
    if (eof) {
        _eof = true;
        _eof_index = n + index;
    }

    for (auto it = _unassembled_bytes.begin(); it != _unassembled_bytes.end();) {
        //! 当前未整理的集合里不存在所希望的下个序列号的字节
        if (it->first != _nindex)
            break;
        string str;
        size_t written_bytes = _output.write(str+it->second);
        if (!written_bytes)
            break;
        _nindex++;
        //! erase(it++)保证安全遍历map
        _unassembled_bytes.erase(it++);
    }

    //! 判断此时所有字节已经整理完成
    if (_eof && _eof_index == _nindex) {
        //! 关闭字节流输入端
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes.size(); }

bool StreamReassembler::empty() const { return unassembled_bytes() == 0; }
