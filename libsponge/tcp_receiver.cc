#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    bool eof = false;
    TCPHeader header = seg.header();
    //! 优先判断syn flag，设置初始序列号ISN
    if (header.syn) {
        _isn = header.seqno;
    }
    //! 判断fin flag，设置eof标记该报文结束
    if (header.fin) {
        eof = true;
    }
    if (!_isn.has_value())
        return;
    //! 获取tcp报文有效载荷
    Buffer buffer = seg.payload();
    uint64_t index = header.syn ? 0 : unwrap(header.seqno, _isn.value(), _checkpoint) - 1;
    _reassembler.push_substring(static_cast<string>(buffer.str()), index, eof);


    index = _reassembler.stream_out().input_ended() ? _reassembler.stream_out().bytes_written() + 1
                                                    : _reassembler.stream_out().bytes_written();
    _checkpoint = index;
    _ackno = wrap(index + 1, _isn.value());
}

optional<WrappingInt32> TCPReceiver::ackno() const { return _ackno; }

size_t TCPReceiver::window_size() const { return _reassembler.stream_out().remaining_capacity(); }
