#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <algorithm>
using std::min;
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
    : _rto(retx_timeout)
    , _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity){}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }
bool TCPSender::send_segment(TCPSegment seg) {
    //! 如果当前报文内容str为空，且无fin、syn标识，不需要发送任何报文
    if (seg.length_in_sequence_space()==0 )
        return false;
    else if(seg.length_in_sequence_space() >_windows_size){
        _outstanding_segments.push_back(seg);
        return true;
    }
    uint16_t len = seg.length_in_sequence_space(),pos=0;
    if(seg.length_in_sequence_space()<TCPConfig::MAX_PAYLOAD_SIZE){
        _segments_out.push(seg);
        _outstanding_segments.push_back(seg);
        _windows_size-=seg.length_in_sequence_space();
    }
    else
    while(len!=0){

        TCPSegment subseg;
        uint16_t npos=min(len,static_cast<uint16_t>(TCPConfig::MAX_PAYLOAD_SIZE));
        subseg.header() = seg.header();
        subseg.header().seqno = seg.header().seqno+pos;
        subseg.payload()=Buffer(seg.payload().copy().substr(pos,npos));
        pos += npos;
        _segments_out.push(subseg);
        _outstanding_segments.push_back(subseg);
        _windows_size-=subseg.length_in_sequence_space();
        len -= npos;
    }

    return true;
}
void TCPSender::fill_window() {

    TCPSegment seg;
    // 判断是否发syn包
    if(_next_seqno == 0){
        seg.header().syn = true;
        _windows_size=1;
    }

    //! 为TCP报文增加payload
    string str = _stream.read(_windows_size);
    seg.payload() = Buffer(std::move(str));
    seg.header().seqno = wrap(_next_seqno,_isn);

    //! 如果windows_size仍有空间，且_stream已经达到eof,则应该发送fin包
    if(_windows_size - seg.length_in_sequence_space()> 0 && _stream.eof() && !_is_finsent){
        //! 设置FIN flag
        seg.header().fin = true;
        _is_finsent = true;
    }

    if(send_segment(seg)){
        _bytes_in_flight += seg.length_in_sequence_space();
        //! 更新下一个应发送的报文的绝对序列号
        _next_seqno += seg.length_in_sequence_space();
    };

    //! 开启重传计时器
    if(_is_stopped){
        _is_stopped = false;
        _timer = _rto;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    _windows_size = window_size;

    bool has_newdata= false;
    size_t sequence_occupied = 0;
    //! 寻找到seqno等于ackno的未确定报文，并删除
    for (auto it = _outstanding_segments.begin();it!=_outstanding_segments.end();) {
        if(it->header().seqno + it->length_in_sequence_space() !=ackno){
            it++;
            continue ;
        }
        if(it->header().fin)
            _is_finsent = true;
        sequence_occupied += it->length_in_sequence_space();
        _outstanding_segments.erase(it++);
        has_newdata = true;

    }

    //! 判断是否有新的成功确认的报文
    if(has_newdata){
        _bytes_in_flight -= sequence_occupied;
        //! 将rto设置为初始值
        _rto = _initial_retransmission_timeout;
        //! 重置连续重传次数
        _contiguous_num = 0;
        //! 重置重传计时器并启动
        if(!_outstanding_segments.empty()) {
            _timer = _rto;
            _is_stopped = false;
        }
    }
    //! 所有未完成的数据都被确认后，停⽌重传定时器
    if(_outstanding_segments.empty()){
        _is_stopped = true;
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (_timer <= ms_since_last_tick && !_is_stopped){
        TCPSegment seg = _outstanding_segments.front();

        if(send_segment(seg)){
            _outstanding_segments.pop_back();
        }

        if (_windows_size!=0){
            //! 跟踪重传次数
            _contiguous_num++;
            //! rot翻倍，设置退避指数
            _rto*=2;
        }
        _is_stopped = false;
        _timer = _rto;
    }
    else
        _timer -= ms_since_last_tick;


}

unsigned int TCPSender::consecutive_retransmissions() const { return _contiguous_num; }

void TCPSender::send_empty_segment() {}

