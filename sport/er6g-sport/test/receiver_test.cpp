#include "SportReceiver.h"
#include "crsf_telemetry.h"
#include <cassert>
#include <cstdio>
#include <vector>
static uint32_t clockMs;
uint32_t millis() { return clockMs; }
using namespace onboard;
using A = SportBus::Action;
static std::vector<uint8_t> reply(uint16_t app, uint32_t value) {
    uint8_t bytes[] = {0x10, uint8_t(app), uint8_t(app>>8), uint8_t(value), uint8_t(value>>8), uint8_t(value>>16), uint8_t(value>>24), 0};
    unsigned sum=0;
    for (unsigned i=0;i<7;++i) { sum+=bytes[i]; sum=(sum&255)+(sum>>8); }
    bytes[7]=255-sum;
    std::vector<uint8_t> wire;
    for (auto byte:bytes) {
        if (byte==0x7e || byte==0x7d) { wire.push_back(0x7d); wire.push_back(byte^0x20); }
        else wire.push_back(byte);
    }
    return wire;
}
static void feed(SportBus &bus, const std::vector<uint8_t> &wire, uint32_t ms) {
    for (auto byte:wire) bus.feed(byte,ms);
}
static void validate(const uint8_t *frame, size_t n) {
    assert(n==size_t(frame[1])+2 && frame[0]==0xea && frame[2]!=0x08);
    // Independent polynomial division rather than the encoder's bit loop.
    unsigned remainder=0;
    for (size_t i=2;i<n;++i) {
        remainder^=unsigned(frame[i])<<8;
        for (unsigned b=0;b<8;++b) {
            remainder<<=1;
            if(remainder&0x10000) remainder^=0x1d500;
        }
    }
    assert((remainder&0xff00)==0);
}
int main() {
    SportBus bus; SportSample sample;
    assert(bus.tick(0,false)==A::Poll && bus.physicalId()==0);
    feed(bus,reply(0x0400,80),0); // TX echo must be ignored.
    assert(!bus.takeSample(sample));
    assert(bus.tick(100,false)==A::None);
    assert(bus.tick(350,true)==A::Release);
    feed(bus,reply(0x0400,0x7d7e),1);
    assert(bus.takeSample(sample) && sample.physicalId==0 && sample.value==0x7d7e);
    assert(bus.samples()==1 && !bus.takeSample(sample));
    feed(bus,reply(0x0400,90),2);
    assert(!bus.takeSample(sample)); // One response per poll, even after delivery.
    assert(bus.tick(11999,true)==A::None);
    assert(bus.tick(12000,true)==A::Poll && bus.physicalId()==0xa1);
    assert(bus.tick(12350,true)==A::Release);
    auto corrupt=reply(0x0400,80); corrupt.back()^=1;
    feed(bus,corrupt,13); assert(!bus.takeSample(sample));
    assert(bus.checksumErrors()==1);
    assert(bus.tick(24000,true)==A::Poll && bus.physicalId()==0x22);
    assert(bus.tick(24350,true)==A::Release);
    feed(bus,{0x10,0,4},25); // Truncated reply must not leak into next poll.
    assert(bus.tick(36000,true)==A::Poll && bus.physicalId()==0x83);
    assert(bus.tick(36350,true)==A::Release);
    feed(bus,reply(0x0400,80),37);
    assert(bus.takeSample(sample) && sample.physicalId==0x83 && sample.value==80);
    assert(bus.checksumErrors()==1);
    for (unsigned i=4;i<32;++i) {
        assert(bus.tick(i*12000,true)==A::Poll);
        assert(bus.tick(i*12000+350,true)==A::Release);
    }
    assert(bus.physicalId()==0xff);
    assert(bus.tick(384000,true)==A::Poll && bus.physicalId()==0);
    SportBus delayed;
    assert(delayed.tick(0,false)==A::Poll);
    assert(delayed.tick(15000,false)==A::None); // No wait/spin when UART is busy.
    assert(delayed.tick(16000,true)==A::Release);
    assert(delayed.tick(25999,true)==A::None); // Preserve full reply window.
    assert(delayed.tick(26000,true)==A::Poll);
    assert(delayed.tick(26350,true)==A::Release);
    feed(delayed,{0x10,0x7d,0x01},27); // Malformed escape rejects packet.
    assert(!delayed.takeSample(sample));
    assert(delayed.tick(38000,true)==A::Poll);
    assert(delayed.tick(38350,true)==A::Release);
    feed(delayed,reply(0x0400,80),39);
    assert(delayed.takeSample(sample));
    SportBus rollover;
    assert(rollover.tick(0xffffff00u,false)==A::Poll);
    assert(rollover.tick(100,true)==A::Release);
    // micros rollover must not contaminate parser's millis clock.
    feed(rollover,reply(0x0400,80),4294968);
    assert(rollover.takeSample(sample));
    assert(rollover.tick(11743,true)==A::None);
    assert(rollover.tick(11744,true)==A::Poll);

    SportBus observed;
    assert(observed.tick(0,false)==A::Poll);
    assert(observed.tick(350,true)==A::Release);
    feed(observed,{0,0,0},1); // Handover noise alone must not create a sample.
    assert(!observed.takeSample(sample) && observed.checksumErrors()==0);
    assert(observed.tick(12000,true)==A::Poll && observed.physicalId()==0xa1);
    assert(observed.tick(12350,true)==A::Release);
    // Captured wire data: zeros inside the payload must remain untouched.
    feed(observed,{0,0x10,0,3,4,0,0,0,0xe8,0},13);
    assert(observed.takeSample(sample) && sample.appId==0x0300 && sample.value==4);
    assert(sample.physicalId==0xa1 && observed.checksumErrors()==0);
    assert(observed.ignoredPrefixZeros()==4);

    SportTelemetry telemetry({{0x22,0x0300},{0x83,0x0400},0xa1,0});
    uint8_t out[19];
    assert(!telemetry.nextFrame(0,out,sizeof(out)));
    telemetry.accept({0x84,0x0400,60},0);
    assert(!telemetry.nextFrame(0,out,sizeof(out)));
    telemetry.accept({0x83,0x0400,uint32_t(-10)},0);
    assert(!telemetry.nextFrame(0,nullptr,19));
    assert(!telemetry.nextFrame(0,out,18));
    auto n=telemetry.nextFrame(0,out,19); assert(n==7); validate(out,n);
    assert(out[2]==0x0d && out[3]==0 && out[4]==0xff && out[5]==0x9c);
    CrsfTelemetryParser tx;
    clockMs=0; for(size_t i=0;i<n;++i) tx.feed(out[i]);
    assert(tx.state().temperatureDeciC==-100 && tx.state().temperatureValid);
    assert(!tx.state().receiverVoltageValid && tx.state().crcErrors==0);
    assert(!telemetry.nextFrame(199,out,19));
    assert(!telemetry.nextFrame(3001,out,19));
    // Complete 4S scan, exactly 3.8/3.9/4.0/4.1 V; no pack 0x08 output.
    auto cells=[](unsigned index,unsigned first,unsigned second) {
        return uint32_t(index|0x40|(first/2<<8)|(second/2<<20));
    };
    telemetry.accept({0x22,0x0300,cells(0,3800,3900)},3100);
    assert(!telemetry.nextFrame(3100,out,19));
    telemetry.accept({0x22,0x0300,cells(2,4000,4100)},3100);
    n=telemetry.nextFrame(3100,out,19); assert(n==13); validate(out,n);
    assert(out[2]==0x0e && out[3]==0 && out[4]==0x0e && out[5]==0xd8 && out[10]==0x10 && out[11]==0x04);
    assert(!telemetry.nextFrame(6101,out,19));
    // Actual FLVS ADV captures include both escaped 0x7d and escaped 0x7e.
    // Replay through the S.Port parser, encoder and the transmitter parser.
    SportTelemetry captured({{0xa1,0x0300},{0xff,0xffff},0xff,0xff});
    SportParser wireParser;
    auto replay=[&](uint8_t id, uint16_t app, uint32_t raw, uint32_t now) {
        std::vector<uint8_t> wire={0x7e,id};
        auto body=reply(app,raw);wire.insert(wire.end(),body.begin(),body.end());
        for(auto b:wire) if(wireParser.feed(b,now,sample)) captured.accept(sample,now);
    };
    replay(0x22,0x0300,0x7d77ca40,7000); // Wrong sensor cannot fill a scan.
    replay(0xa1,0x0300,0x7e07d342,7000);
    assert(!captured.nextFrame(7000,out,19));
    replay(0xa1,0x0301,0x7d77ca40,7000); // Wrong app instance also ignored.
    assert(!captured.nextFrame(7000,out,19));
    replay(0xa1,0x0300,0x7d77ca40,7000);
    assert(!captured.nextFrame(7000,out,19));
    replay(0xa1,0x0300,0x7e07d342,7001);
    n=captured.nextFrame(7001,out,19);assert(n==13);validate(out,n);
    clockMs=7001;for(size_t i=0;i<n;++i) tx.feed(out[i]);
    assert(tx.state().vehicleCellsValid && tx.state().vehiclePackMv==16040);
    assert(tx.state().vehicleCellsMv[0]==3988 && tx.state().vehicleCellsMv[1]==4014);
    assert(tx.state().vehicleCellsMv[2]==4006 && tx.state().vehicleCellsMv[3]==4032);
    assert(tx.state().vehicleMinimumCellMv==3988 && !tx.state().receiverVoltageValid);
    assert(wireParser.checksumErrors()==0 && tx.state().crcErrors==0);
    replay(0xa1,0x0300,0x7d77ca40,7201); // Incomplete next scan cannot emit a mixed pack.
    assert(!captured.nextFrame(7201,out,19));
    replay(0xa1,0x0300,0x7e07d442,7202);
    assert(captured.nextFrame(7202,out,19)==13);
    assert(!captured.nextFrame(10203,out,19));
    clockMs=10002;tx.updateFreshness();assert(!tx.state().vehicleCellsValid);
    // GPS requires every field fresh; use fixed values to check endian/scales.
    telemetry.accept({0xa1,0x0800,600000},6200); // latitude 1 degree
    telemetry.accept({0xa1,0x0800,0xc0000000u|1200000},6200); // longitude -2
    telemetry.accept({0xa1,0x0830,1000},6200); // 1 knot -> 1.9 km/h
    telemetry.accept({0xa1,0x0840,12345},6200);
    assert(!telemetry.nextFrame(6200,out,19));
    telemetry.accept({0xa1,0x0820,uint32_t(-12300)},6200);
    n=telemetry.nextFrame(6200,out,19); assert(n==19); validate(out,n);
    assert(out[2]==2 && out[11]==0 && out[12]==19 && out[15]==3 && out[16]==0x6d && out[17]==0);
    clockMs=6200; for(size_t i=0;i<n;++i) tx.feed(out[i]);
    assert(tx.state().latitudeE7==10000000 && tx.state().longitudeE7==-20000000);
    assert(tx.state().altitudeMetres==-123 && tx.state().satellites==0);
    telemetry.accept({0xa1,0x0820,uint32_t(-100100)},6400);
    assert(!telemetry.nextFrame(6400,out,19)); // altitude underflows wire range
    SportTelemetry timer({{0x22,0x0300},{0x83,0x0400},0xa1,0});
    timer.accept({0x83,0x0400,80},0xffffff80);
    assert(timer.nextFrame(0xffffff80,out,19)==7);
    assert(!timer.nextFrame(50,out,19));
    assert(timer.nextFrame(72,out,19)==7);
    assert(!timer.nextFrame(3000,out,19));
    puts("ER6G S.Port polling, encoding, freshness, rollover and transmitter tests passed");
}
