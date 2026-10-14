CXX ?= c++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -g -fsanitize=address,undefined
RX := ../../src/src/rx-serial
INC := ../../src/include
SOURCES := build/adapter/SerialSportER4.cpp $(RX)/SportReceiver.cpp $(RX)/sport_crsf.cpp $(RX)/vehicle_sensors.cpp
HEADERS := $(RX)/SerialSportER4.h $(RX)/Er4TxDrain.h $(RX)/SportReceiver.h $(INC)/SportCapture.h $(INC)/Er4BenchDefaults.h $(INC)/sport_crsf.h $(INC)/vehicle_sensors.h $(wildcard test/stubs/*.h)
.PHONY: test clean
test: build/adapter_test
	./build/adapter_test
	python3 test/upload_guard_test.py
# Stage the canonical adapter without the neighbouring Arduino SerialIO header,
# so simulated UART/pin/router boundaries can compile the real adapter on host.
build/adapter/SerialSportER4.cpp: $(RX)/SerialSportER4.cpp $(HEADERS)
	@mkdir -p build/adapter
	cp $(RX)/SerialSportER4.cpp $(RX)/SerialSportER4.h $(RX)/Er4TxDrain.h build/adapter/
build/adapter_test: test/adapter_test.cpp $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -DTARGET_RX=1 -DPLATFORM_ESP8266=1 -DER4_SPORT_TRIAL=1 -DSPORT_TEMP_PHYSICAL_ID=0 -DSPORT_TEMP_APP_ID=0x400 -DUS_CHANNEL_VALUE_CENTER=1500 -DUS_CHANNEL_VALUE_MIN=476 -DPROTOCOL_SPORT=10 -Dsom50Hz=0 -DsomSerial=10 -Ibuild/adapter -Itest/stubs -I$(RX) -I$(INC) test/adapter_test.cpp $(SOURCES) -o $@
clean:
	rm -rf build
