#include "SerialSport.h"
#include "Er6gSportProfile.h"
#include "common.h"
#include "CRSFRouter.h"
#include "soc/uart_struct.h"
#include <cassert>
#include <cstdio>
uint32_t nowUs;
uint32_t millis(){return nowUs/1000;}
uint32_t micros(){return nowUs;}
State connectionState=connected;
Router crsfRouter;
uart_dev_t UART1;
bool idle=true;
int mode=-1, rxPin=-1, txPin=-1;
bool rxInverted=false,txInverted=false;
bool uart_ll_is_tx_idle(uart_dev_t*){return idle;}
void pinMode(uint8_t pin,uint8_t value){assert(pin==4);mode=value;}
void digitalWrite(uint8_t pin,uint8_t value){assert(pin==4&&value==LOW);}
void pinMatrixOutDetach(uint8_t pin,bool,bool){assert(pin==4);txPin=-1;}
void pinMatrixInAttach(uint8_t pin,int signal,bool invert){assert(signal==U1RXD_IN_IDX);rxPin=pin;rxInverted=invert;}
void pinMatrixOutAttach(uint8_t pin,int signal,bool invert,bool){assert(signal==U1TXD_OUT_IDX);txPin=pin;txInverted=invert;}
static void reply(Stream &in,uint32_t value,uint16_t app=0x0300){
    uint8_t bytes[]={0x10,uint8_t(app),uint8_t(app>>8),uint8_t(value),uint8_t(value>>8),uint8_t(value>>16),uint8_t(value>>24),0};
    unsigned sum=0;for(unsigned i=0;i<7;++i){sum+=bytes[i];sum=(sum&255)+(sum>>8);}bytes[7]=255-sum;
    for(auto byte:bytes){if(byte==0x7d||byte==0x7e){in.rx.push_back(0x7d);in.rx.push_back(byte^0x20);}else in.rx.push_back(byte);}
}
int main(){
    int pins[]={14,1,3,15,2,4};
    assert(er6gSportPinAllowed(pins,6,4,-1));
    assert(!er6gSportPinAllowed(pins,4,4,-1));
    assert(!er6gSportPinAllowed(pins,6,1,-1));
    assert(!er6gSportPinAllowed(pins,6,4,3));
    pins[0]=0;assert(!er6gSportPinAllowed(pins,6,4,-1));
    Stream out,in;
    {
        SerialSport driver(out,in,4);
        assert(mode==INPUT&&rxPin==4&&rxInverted&&txPin==-1);
        for(unsigned poll=0;poll<=33;++poll){
            nowUs=poll*12000;idle=false;driver.sendQueuedData(32);
            assert(mode==OUTPUT&&txPin==4&&txInverted);
            assert(out.tx[out.tx.size()-2]==0x7e);
            nowUs+=200;driver.sendQueuedData(32);assert(mode==OUTPUT); // busy UART must retain drive
            in.rx.push_back(0x7e); // local echo drained at handover
            nowUs+=200;idle=true;driver.sendQueuedData(32);
            assert(mode==INPUT&&txPin==-1&&rxPin==4&&rxInverted&&in.rx.empty());
            if(poll==1||poll==33){
                reply(in,poll==1?0x7d77ca40:0x7e07d342);
                driver.processSerialInput();driver.sendQueuedData(32);
            }
        }
        assert(crsfRouter.frames.size()==1);
        const auto &frame=crsfRouter.frames.back();
        assert(frame[2]==0x0e&&frame[3]==0&&frame[4]==0x0f&&frame[5]==0x94);
        const auto count=crsfRouter.frames.size();
        connectionState=disconnected;nowUs+=300000;driver.sendQueuedData(32);
        assert(crsfRouter.frames.size()==count); // never forward to RF without a link
    }
    assert(mode==INPUT&&txPin==-1);
    connectionState=connected;
    {
        SerialSport driver(out,in,4);
        for(unsigned poll=0;poll<=34;++poll){
            nowUs=1000000+poll*12000;idle=false;driver.sendQueuedData(32);
            nowUs+=400;idle=true;driver.sendQueuedData(32);
            if(poll==2||poll==34){
                reply(in,poll==2?11559:0,poll==2?0x0200:0x0210);
                driver.processSerialInput();driver.sendQueuedData(32);
            }
        }
        // Allow the two independently queued telemetry types through the 5Hz limiter.
        nowUs+=200000;driver.sendQueuedData(32);
        nowUs+=400;driver.sendQueuedData(32);
        bool haveCurrent=false,haveVoltage=false;
        for(const auto &f:crsfRouter.frames){
            if(f[2]==0x08){assert(f.size()==13 && f[11]==1 && f[5]==0x2d && f[6]==0x27);haveCurrent=true;}
            if(f[2]==0x0e && f[3]==128){assert(f.size()==7 && f[4]==0 && f[5]==0);haveVoltage=true;}
        }
        assert(haveCurrent && haveVoltage);
        const auto writes=out.tx.size();const auto forwarded=crsfRouter.frames.size();
        connectionState=wifiUpdate;nowUs+=12000;driver.sendQueuedData(32);
        assert(mode==INPUT && txPin==-1 && out.tx.size()==writes);
        reply(in,123,0x0200);driver.processSerialInput();driver.sendQueuedData(32);
        assert(crsfRouter.frames.size()==forwarded && out.tx.size()==writes);
    }
    puts("ER6G canonical ESP32 adapter: one-wire routing, inverted poll, final-stop release, escaped 4S forwarding, RF gating and pin guard PASS");
}
