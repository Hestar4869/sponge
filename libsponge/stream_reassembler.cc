#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _unreassembleData(), _nindex(0),_unassembled_bytes(0),_eof(false),_output(capacity),_capacity(capacity) {

}

//! 将两个能拼接在一起的字符串进行拼接
void StreamReassembler::stichSubstring(StreamReassembler::substring_t &answer, StreamReassembler::substring_t &sub1, StreamReassembler::substring_t &sub2) {
    answer.index = sub1.index;
    answer.eof = sub2.eof;
    int len = sub2.index + sub2.data.size() - sub1.index - sub1.data.size();
    if(len > 0) {
        answer.data = sub1.data + sub2.data.substr(sub2.data.size()-len,len);
    }
}
//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    substring_t newSubsting{data,index,eof};
    _unreassembleData.push(newSubsting);

    while(_unreassembleData.size()>=2){
        substring_t sub1,sub2;
        sub1 = _unreassembleData.top();
        _unreassembleData.pop();
        sub2 = _unreassembleData.top();
        _unreassembleData.pop();

        //! 判断两个区间是否有重合的部分
        if(sub1.index+sub1.data.size()>=sub2.index){
            stichSubstring(newSubsting,sub1,sub2);
            _unreassembleData.push(newSubsting);
        }
        else{
            _unreassembleData.push(sub1);
            _unreassembleData.push(sub2);
            break ;
        }
    }
    newSubsting = _unreassembleData.top();
    _unreassembleData.pop();
    int len = newSubsting.data.size() + newSubsting.index - _nindex;
    //! 将该substring加入到字节流中
    if(  len > 0 && _nindex>=newSubsting.index ){
        //! 可能因为bytestream的空间大小不够，导致实际读入的字符数
        int written_bits=_output.write(newSubsting.data.substr(newSubsting.data.size()-len, len));
        //! 更新nindex
        _nindex += written_bits;
        if(newSubsting.eof){
            //todo
            _eof = true;
            _output.end_input();
        }
    }
    else if(_nindex < newSubsting.data.size()+newSubsting.index)
        _unreassembleData.push(newSubsting);
    else if(len == 0 && eof){
        _eof = true;
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const {
    //todo
    return _unreassembleData.top().data.size();
}

bool StreamReassembler::empty() const { return unassembled_bytes() == 0; }
