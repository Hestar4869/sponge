#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <algorithm>
#include <random>
using std::min;
// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {
    _current_rto = _initial_retransmission_timeout;
}

uint64_t TCPSender::bytes_in_flight() const {
    uint64_t bytes = 0;
    for (const auto &it : _outstanding_seg) {
        bytes += it.length_in_sequence_space();
    }
    return bytes;
}

void TCPSender::fill_window() {
    //! 当_next_seqno为0时表明还未发送syn报文
    if (_next_seqno == 0) {
        TCPSegment seg;
        //! 单独处理syn报文
        seg.header().syn = true;
        seg.header().seqno = wrap(_next_seqno,_isn);
        //! 将报文推入待确认报文队列
        _outstanding_seg.push_back(seg);

        //! 报文发送
        send_segment(seg);
        _next_seqno = 1;

        return;
    }


    while ( (_stream.buffer_size() > 0 || (_stream.eof()&&!_is_fin_sent)) && _remaining_window > 0 ) {
        TCPSegment seg;
        seg.header().seqno = wrap(_next_seqno, _isn);
        //! 确保每个报文内容不超过1452btyes
        string str = _stream.read(min(_remaining_window, static_cast<uint16_t>(TCPConfig::MAX_PAYLOAD_SIZE)));
        seg.payload() = Buffer(std::move(str));

        //! 判断是否加上fin标志
        if(_stream.eof() && _remaining_window-seg.length_in_sequence_space()>0){
            seg.header().fin = true;
            _is_fin_sent = true;
        }

        //! 将报文推入待确认报文队列
        _outstanding_seg.push_back(seg);

        //! 报文发送
        send_segment(seg);

        //! 更新相关数据
        _remaining_window -= seg.length_in_sequence_space();
        _next_seqno += seg.length_in_sequence_space();
    }

    //! 单独处理Fin报文
//    if (_stream.eof() && _remaining_window > 0) {
//        TCPSegment seg;
//        seg.header().fin = true;
//        seg.header().seqno = wrap(_next_seqno, _isn);
//        //! 将报文推入待确认报文队列
//        _outstanding_seg.push_back(seg);
//        //! 报文发送
//        send_segment(seg);
//
//        //! 更新相关数据
//        _remaining_window -= seg.length_in_sequence_space();
//        _next_seqno += seg.length_in_sequence_space();
//    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {

    //! 更新剩余窗口
    _is_zero_window = window_size==0;
    _window_size = window_size==0 ? 1 : window_size;
    _remaining_window = _window_size - (_next_seqno - unwrap(ackno, _isn, _next_seqno));
    //! 搜索是否为某个未确认的报文
    bool flag = false;
    for (auto it = _outstanding_seg.begin()
             ; it != _outstanding_seg.end();) {

        if(it->header().seqno+it->length_in_sequence_space()==ackno){
            _outstanding_seg.erase(it++);
            flag = true;
            break ;
        }
        else
            _outstanding_seg.erase(it++);
    }
    //! 确认某个报文发送成功
    if(flag){
        _is_syn_ack = true;
        _current_rto = _initial_retransmission_timeout;
        //! 若仍有未确认的报文，则再次重启定时器
        if(!_outstanding_seg.empty()){
            _timer = _current_rto;
            _is_timer_stopped = false;
        }
        _consecutive_retransmissions = 0;
    }


    //! 若所有报文成功发送则停止定时器
    if(_outstanding_seg.empty()){
        _is_timer_stopped = true;
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if(_is_timer_stopped)
        return ;
    if(_timer <= ms_since_last_tick){
        _is_timer_stopped = true;
        if(!_is_zero_window){
            _consecutive_retransmissions++;
            _current_rto *= 2;
        }
        TCPSegment seg = _outstanding_seg.front();
        send_segment(seg);
    }
    else
        _timer -= ms_since_last_tick;
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {}

//! 发送报文并启动定时器
void TCPSender::send_segment(TCPSegment seg) {
    //! 推入待发送报文队列
    _segments_out.push(seg);

    //! 无论重传或第一次发送报文，如果定时器停止，则开启
    if (_is_timer_stopped) {
        _is_timer_stopped = false;
        _timer = _current_rto;
    }
}
