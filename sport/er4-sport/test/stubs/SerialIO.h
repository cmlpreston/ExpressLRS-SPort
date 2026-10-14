#pragma once
#include <cstdint>
#include <cstddef>
#include <deque>
#include <vector>
uint32_t millis();
uint32_t micros();
void pinMode(uint8_t, uint8_t);
constexpr uint8_t INPUT=0, FUNCTION_0=8;
class Stream {
public:
    std::deque<uint8_t> rx;
    std::vector<uint8_t> tx;
    int available() { return int(rx.size()); }
    int read() { if(rx.empty()) return -1; auto b=rx.front();rx.pop_front();return b; }
    size_t write(uint8_t b) { tx.push_back(b);return 1; }
};
class SerialIO {
public:
    SerialIO(Stream *out, Stream *in):_outputPort(out),input_(in) {}
    virtual ~SerialIO() = default;
    virtual uint32_t sendRCFrame(bool,bool,uint32_t*) = 0;
    virtual void sendQueuedData(uint32_t) = 0;
    void processSerialInput() {
        uint8_t bytes[32]; uint16_t n=0;
        while(input_->available() && n<getMaxSerialReadSize()) bytes[n++]=input_->read();
        processBytes(bytes,n);
    }
protected:
    Stream *_outputPort;
    virtual int getMaxSerialReadSize() = 0;
    virtual void processBytes(uint8_t*,uint16_t) = 0;
private:
    Stream *input_;
};
