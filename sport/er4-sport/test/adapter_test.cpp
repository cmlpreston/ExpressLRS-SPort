#include "SerialSportER4.h"
#include "CRSFRouter.h"
#include "common.h"
#include "Er4BenchDefaults.h"
#include <cassert>
#include <cstdio>
uint32_t fakeUartStatus=0, clockUs=0;
uint32_t micros() { return clockUs; }
uint32_t millis() { return clockUs/1000; }
State connectionState=connected;
Router crsfRouter;
uint8_t modes[4]={};
void pinMode(uint8_t pin,uint8_t mode) { assert(pin==1 || pin==3); modes[pin]=mode; }
std::vector<uint8_t> reply(uint8_t temperature) {
    // Standard T1 packet, independently folded checksum of 0x10+4+temp.
    unsigned sum=0x14+temperature;sum=(sum&255)+(sum>>8);
    return {0x10,0,4,temperature,0,0,0,uint8_t(255-sum)};
}
// Production default helper uses the same API as RxConfig.
struct FakeConfig {
    unsigned protocol=99;
    struct Row {unsigned failsafe,channel,mode;} rows[4]{};
    void SetSerialProtocol(unsigned p) {protocol=p;}
    void SetPwmChannel(unsigned i,unsigned fs,unsigned ch,bool inv,unsigned mode,bool stretch) {
        assert(!inv && !stretch);rows[i]={fs,ch,mode};
    }
};
int main() {
    FakeConfig cfg;applyEr4BenchDefaults(cfg);
    assert(cfg.protocol==10 && cfg.rows[0].channel==0 && cfg.rows[0].mode==0);
    assert(cfg.rows[1].mode==10 && cfg.rows[2].mode==10);
    assert(cfg.rows[3].channel==1 && cfg.rows[3].mode==0 && cfg.rows[3].failsafe==1024);
    onboard::Er4TxDrain drain;
    assert(!drain.ready(0,false));assert(!drain.ready(10,true));
    assert(!drain.ready(189,true));assert(drain.ready(190,true));
    drain.begin();assert(!drain.ready(0xffffff80,true));
    assert(!drain.ready(51,true));assert(drain.ready(52,true));
    drain.begin();assert(!drain.ready(0,true));assert(!drain.ready(200,false));
    assert(!drain.ready(300,true));assert(drain.ready(480,true));
    Stream uart;
    {
        SerialSportER4 adapter(uart,uart);
        assert(modes[1]==INPUT && modes[3]==FUNCTION_0 && er4SportCapture.active);
        assert(adapter.sendRCFrame(true,false,nullptr)==0 && uart.tx.empty());
        fakeUartStatus=128<<16;adapter.sendQueuedData(32);assert(uart.tx.empty());
        fakeUartStatus=0;adapter.sendQueuedData(32);
        assert(uart.tx==std::vector<uint8_t>({0x7e,0}) && modes[1]==FUNCTION_0);
        fakeUartStatus=1<<16;clockUs=174;adapter.sendQueuedData(32);
        assert(modes[1]==FUNCTION_0); // FIFO count alone is not shift completion.
        fakeUartStatus=0;clockUs=200;adapter.sendQueuedData(32);
        assert(modes[1]==FUNCTION_0);
        uart.rx={0x7e,0};clockUs=379;adapter.sendQueuedData(32);assert(!uart.rx.empty());
        clockUs=380;adapter.sendQueuedData(32);
        assert(modes[1]==INPUT && uart.rx.empty()); // Echo discarded before release.
        const auto bytes=reply(80);for(auto b:bytes) uart.rx.push_back(b);
        clockUs=1000;adapter.processSerialInput();adapter.sendQueuedData(32);
        assert(er4SportCapture.samples==1 && er4SportCapture.used==1);
        assert(er4SportCapture.entries[0].sample.physicalId==0 && er4SportCapture.entries[0].sample.value==80);
        assert(crsfRouter.frames.size()==1 && crsfRouter.frames[0][2]==0x0d);
        assert(crsfRouter.frames[0][4]==3 && crsfRouter.frames[0][5]==0x20);
        clockUs=12000;adapter.sendQueuedData(32);assert(uart.tx.back()==0xa1);
        clockUs=12400;adapter.sendQueuedData(32);clockUs=12580;adapter.sendQueuedData(32);
        connectionState=disconnected;
        for(auto b:reply(90)) uart.rx.push_back(b);
        clockUs=13000;adapter.processSerialInput();adapter.sendQueuedData(32);
        assert(er4SportCapture.samples==2 && crsfRouter.frames.size()==1);
        // WiFi entry releases even a poll in flight and retains discovery data.
        clockUs=24000;adapter.sendQueuedData(32);assert(modes[1]==FUNCTION_0);
        connectionState=wifiUpdate;adapter.sendQueuedData(32);
        assert(modes[1]==INPUT && !er4SportCapture.active);
        auto sent=uart.tx.size();clockUs=25000;adapter.sendQueuedData(32);assert(uart.tx.size()==sent);
    }
    assert(modes[1]==INPUT && er4SportCapture.samples==2);
    onboard::SportCapture capture;
    for(unsigned i=0;i<33;++i)capture.accept({uint8_t(i),0x0400,i},10);
    assert(capture.used==32 && capture.droppedKeys==1 && capture.samples==33);
    capture.accept({0,0x0400,123},20);
    assert(capture.used==32 && capture.entries[0].count==2 && capture.entries[0].sample.value==123);
    puts("ER4 actual adapter, UART drain, echo, RF gating, shutdown, capture and bench defaults passed");
}
