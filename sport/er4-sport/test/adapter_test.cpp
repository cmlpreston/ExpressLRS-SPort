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
        assert(er4SportCapture.rxBytes==2 && er4SportCapture.txPhaseBytes==2);
        assert(er4SportCapture.releaseDiscardedBytes==2 && er4SportCapture.maxReleaseUs==380);
        assert(er4SportCapture.wire[0].txLength==2 && er4SportCapture.wire[0].tx[0]==0x7e);
        const auto bytes=reply(80);for(auto b:bytes) uart.rx.push_back(b);
        clockUs=1000;adapter.processSerialInput();adapter.sendQueuedData(32);
        assert(er4SportCapture.samples==1 && er4SportCapture.used==1);
        assert(er4SportCapture.replyPhaseBytes==8 && er4SportCapture.rxBytes==10);
        assert(er4SportCapture.wire[0].rxLength==8 && er4SportCapture.wire[0].rx[0]==0x10);
        assert(er4SportCapture.wire[0].firstRxUs==1000);
        assert(er4SportCapture.entries[0].sample.physicalId==0 && er4SportCapture.entries[0].sample.value==80);
        assert(crsfRouter.frames.size()==1 && crsfRouter.frames[0][2]==0x0d);
        assert(crsfRouter.frames[0][4]==3 && crsfRouter.frames[0][5]==0x20);
        clockUs=12000;adapter.sendQueuedData(32);assert(uart.tx.back()==0xa1);
        // Echo read by the normal loop while TX is active is counted, not parsed.
        uart.rx={0x7e,0xa1};adapter.processSerialInput();
        assert(er4SportCapture.txPhaseBytes==4 && er4SportCapture.samples==1);
        assert(er4SportCapture.wire[1].physicalId==0xa1);
        clockUs=12400;adapter.sendQueuedData(32);clockUs=12580;adapter.sendQueuedData(32);
        connectionState=disconnected;
        // Exact bytes captured from the user's cell monitor, including the
        // leading handover zero and trailing zero. Payload checksum is valid.
        uart.rx={0,0x10,0,3,4,0,0,0,0xe8,0};
        clockUs=13000;adapter.processSerialInput();adapter.sendQueuedData(32);
        assert(er4SportCapture.samples==2 && crsfRouter.frames.size()==1);
        assert(er4SportCapture.used==2 && er4SportCapture.checksumErrors==0);
        assert(er4SportCapture.ignoredPrefixZeros==1);
        assert(er4SportCapture.entries[1].sample.physicalId==0xa1);
        assert(er4SportCapture.entries[1].sample.appId==0x0300);
        assert(er4SportCapture.entries[1].sample.value==4);
        assert(er4SportCapture.entries[1].minRawValue==4 && er4SportCapture.entries[1].maxRawValue==4);
        assert(er4SportCapture.recentUsed==2 && er4SportCapture.recent[1].sample.appId==0x0300);
        // WiFi preserves the normal UART handover and accepts live replies.
        clockUs=24000;adapter.sendQueuedData(32);assert(modes[1]==FUNCTION_0);
        connectionState=wifiUpdate;
        clockUs=24400;adapter.sendQueuedData(32);
        clockUs=24580;adapter.sendQueuedData(32);
        assert(modes[1]==INPUT && er4SportCapture.active);
        for(auto b:reply(90)) uart.rx.push_back(b);
        clockUs=25000;adapter.processSerialInput();adapter.sendQueuedData(32);
        assert(er4SportCapture.samples==3 && crsfRouter.frames.size()==1);
        clockUs=36000;adapter.sendQueuedData(32);assert(modes[1]==FUNCTION_0);
        // A real upload releases immediately, even mid-poll, and stays paused.
        er4SportBeginUpload();
        assert(modes[1]==INPUT && !er4SportCapture.active && er4SportUploadInProgress);
        auto sent=uart.tx.size();
        for(auto b:reply(100)) uart.rx.push_back(b);
        clockUs=37000;adapter.processSerialInput();adapter.sendQueuedData(32);
        assert(uart.tx.size()==sent && er4SportCapture.samples==3);
        connectionState=connected; // Failed upload must not silently restart polling.
        clockUs=48000;adapter.sendQueuedData(32);assert(uart.tx.size()==sent);
        // Simulate a fresh boot solely to exercise non-WiFi failure states.
        er4SportUploadInProgress=false;
        connectionState=radioFailed;adapter.sendQueuedData(32);
        assert(modes[1]==INPUT && !er4SportCapture.active && uart.tx.size()==sent);
        connectionState=serialUpdate;adapter.sendQueuedData(32);
        assert(modes[1]==INPUT && !er4SportCapture.active && uart.tx.size()==sent);
    }
    assert(modes[1]==INPUT && er4SportCapture.samples==3);
    // Selected 4S cells are decoded in WiFi, but queue only with an RF link.
    connectionState=wifiUpdate;er4SportCapture.reset();uart.rx.clear();
    {
        SerialSportER4 adapter(uart,uart);
        const auto before=crsfRouter.frames.size();
        for(unsigned poll=0;poll<=33;++poll) {
            clockUs=100000+poll*12000;adapter.sendQueuedData(32);
            clockUs+=400;adapter.sendQueuedData(32);
            clockUs+=180;adapter.sendQueuedData(32);
            if(poll!=1 && poll!=33) continue;
            std::vector<uint8_t> body=poll==1 ?
                std::vector<uint8_t>{0x10,0,3,0x40,0xca,0x77,0x7d} :
                std::vector<uint8_t>{0x10,0,3,0x42,0xd3,7,0x7e};
            unsigned sum=0;for(auto b:body){sum+=b;sum=(sum&255)+(sum>>8);}
            body.push_back(uint8_t(255-sum));
            for(auto b:body) {
                if(b==0x7d || b==0x7e){uart.rx.push_back(0x7d);uart.rx.push_back(b^0x20);}
                else uart.rx.push_back(b);
            }
            clockUs+=420;adapter.processSerialInput();adapter.sendQueuedData(32);
            assert(crsfRouter.frames.size()==before && er4SportCellsQueued==0);
            uint16_t cells[4],total,minimum;
            bool valid=er4SportCellSnapshot(millis(),cells,total,minimum);
            assert(valid==(poll==33));
            if(valid) assert(cells[0]==3988 && cells[1]==4014 && cells[2]==4006 && cells[3]==4032 && total==16040 && minimum==3988);
        }
        connectionState=connected;adapter.sendQueuedData(32);
        assert(crsfRouter.frames.size()==before+1 && er4SportCellsQueued==1);
        assert(crsfRouter.frames.back()[2]==0x0e && crsfRouter.frames.back()[3]==0);
        clockUs+=199000;adapter.sendQueuedData(32);assert(er4SportCellsQueued==1);
        clockUs+=1000;adapter.sendQueuedData(32);assert(er4SportCellsQueued==2);
        clockUs+=3001000;adapter.sendQueuedData(32);assert(er4SportCellsQueued==2);
        uint16_t cells[4],total,minimum;
        assert(!er4SportCellSnapshot(millis(),cells,total,minimum));
        er4SportBeginUpload();adapter.sendQueuedData(32);assert(er4SportCellsQueued==2);
    }
    er4SportUploadInProgress=false;
    connectionState=wifiUpdate;
    {
        SerialSportER4 adapter(uart,uart);
        uart.rx={0x55,0xaa};
        auto sent=uart.tx.size();
        assert(er4SportEnableSniffer());
        assert(er4SportSniffMode && modes[1]==INPUT && uart.rx.empty());
        assert(er4SportCapture.samples==0 && er4SportCapture.used==0);
        clockUs=100000;adapter.sendQueuedData(32);assert(uart.tx.size()==sent);
        // Independent full X4R poll/reply fixture with a nonzero 4S cell pair.
        std::vector<uint8_t> wire={0x7e,0xa1};
        std::vector<uint8_t> body={0x10,0,3,0x40,0x34,0xa8,0x82};
        unsigned sum=0;
        for(auto b:body) {sum+=b;sum=(sum&255)+(sum>>8);}
        body.push_back(uint8_t(255-sum));
        for(auto b:body) {
            if(b==0x7d || b==0x7e) {wire.push_back(0x7d);wire.push_back(b^0x20);}
            else wire.push_back(b);
        }
        for(unsigned i=0;i<5;++i) uart.rx.push_back(wire[i]);
        adapter.processSerialInput();assert(er4SportCapture.samples==0);
        for(unsigned i=5;i<wire.size();++i) uart.rx.push_back(wire[i]);
        adapter.processSerialInput();adapter.sendQueuedData(32);
        assert(er4SportObservedPolls==1 && er4SportCapture.polls==0);
        assert(er4SportCapture.samples==1 && er4SportCapture.checksumErrors==0);
        assert(er4SportCapture.entries[0].sample.physicalId==0xa1);
        assert(er4SportCapture.entries[0].sample.appId==0x300);
        assert(er4SportCapture.entries[0].sample.value==0x82a83440);
        assert(uart.tx.size()==sent && modes[1]==INPUT);
        clockUs=200000;adapter.sendQueuedData(32);assert(uart.tx.size()==sent);
        er4SportBeginUpload();assert(!er4SportEnableSniffer());
        for(auto b:wire) uart.rx.push_back(b);
        adapter.processSerialInput();adapter.sendQueuedData(32);
        assert(er4SportCapture.samples==1 && uart.tx.size()==sent && modes[1]==INPUT);
    }
    assert(!er4SportEnableSniffer());
    er4SportUploadInProgress=false;er4SportSniffMode=false;connectionState=connected;
    crsfRouter.frames.clear();er4SportCellsQueued=0;
    {
        Stream fasUart;SerialSportER4 adapter(fasUart,fasUart);
        for(unsigned poll=0;poll<=34;++poll){
            clockUs=5000000+poll*12000;fakeUartStatus=0;adapter.sendQueuedData(32);
            clockUs+=400;adapter.sendQueuedData(32);clockUs+=180;adapter.sendQueuedData(32);
            if(poll!=2 && poll!=34)continue;
            const uint32_t value=poll==2?11559:0;const uint16_t app=poll==2?0x0200:0x0210;
            std::vector<uint8_t> body={0x10,uint8_t(app),uint8_t(app>>8),uint8_t(value),uint8_t(value>>8),uint8_t(value>>16),uint8_t(value>>24)};
            unsigned sum=0;for(auto b:body){sum+=b;sum=(sum&255)+(sum>>8);}body.push_back(uint8_t(255-sum));
            for(auto b:body){if(b==0x7d || b==0x7e){fasUart.rx.push_back(0x7d);fasUart.rx.push_back(b^0x20);}else fasUart.rx.push_back(b);}
            clockUs+=420;adapter.processSerialInput();adapter.sendQueuedData(32);
        }
        clockUs+=200000;adapter.sendQueuedData(32);
        bool haveCurrent=false,haveVoltage=false;
        for(const auto &f:crsfRouter.frames){
            if(f[2]==0x08){assert(f.size()==13 && f[11]==1 && f[5]==0x2d && f[6]==0x27);haveCurrent=true;}
            if(f[2]==0x0e && f[3]==128){assert(f.size()==7 && f[4]==0 && f[5]==0);haveVoltage=true;}
        }
        assert(haveCurrent && haveVoltage && er4SportCellsQueued==0);
    }
    onboard::SportCapture capture;
    for(unsigned i=0;i<33;++i)capture.accept({uint8_t(i),0x0400,i},10);
    assert(capture.used==32 && capture.droppedKeys==1 && capture.samples==33);
    capture.accept({0,0x0400,123},20);
    assert(capture.used==32 && capture.entries[0].count==2 && capture.entries[0].sample.value==123);
    assert(capture.entries[0].minRawValue==0 && capture.entries[0].maxRawValue==123);
    assert(capture.recentUsed==16);
    assert(capture.recent[capture.recentNext].sample.value==18);
    assert(capture.recent[(capture.recentNext+15)%16].sample.value==123);
    capture.accept({0,0x0400,1},30);
    assert(capture.entries[0].minRawValue==0 && capture.entries[0].maxRawValue==123);
    assert(capture.entries[0].sample.value==1 && capture.entries[0].count==3);
    // Traces are bounded and retain the last nonempty observation over empty polls.
    capture.beginPoll(0xa1, 0xfffffff0);
    for (unsigned i=0; i<40; ++i) capture.observeByte(uint8_t(i), false, false, 20);
    assert(capture.wire[1].rxLength==16 && capture.wire[1].rx[15]==15);
    assert(capture.wire[1].rxBytes==40 && capture.wire[1].firstRxUs==36);
    capture.released(30);assert(capture.maxReleaseUs==46);
    capture.beginPoll(0xa1, 100);
    assert(capture.wire[1].rxLength==16);
    capture.observeByte(0x10, false, false, 120);
    assert(capture.wire[1].rxLength==1 && capture.wire[1].rx[0]==0x10);
    assert(capture.wire[1].rxBytes==41 && capture.wire[1].firstRxUs==20);
    capture.reset();
    assert(capture.used==0 && capture.recentUsed==0 && capture.samples==0);
    assert(capture.rxBytes==0 && capture.wire[1].rxBytes==0);
    puts("ER4 actual adapter, UART drain, echo, RF gating, shutdown, capture and bench defaults passed");
}
