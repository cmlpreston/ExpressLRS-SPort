CXX ?= c++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -g -fsanitize=address,undefined
RX := ../../src/src/rx-serial
INC := ../../src/include
FIX := ../compatibility/jr-r1
.PHONY: test clean
test: build/receiver_test build/adapter_test
	./build/receiver_test
	./build/adapter_test
build/receiver_test: test/receiver_test.cpp $(RX)/SportReceiver.cpp $(RX)/SportReceiver.h $(RX)/sport_crsf.cpp $(RX)/vehicle_sensors.cpp $(INC)/sport_crsf.h $(INC)/vehicle_sensors.h $(FIX)/src/crsf_telemetry.cpp $(FIX)/include/crsf_telemetry.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -I$(RX) -I$(INC) -I../sport-crsf/test -I$(FIX)/include test/receiver_test.cpp $(RX)/SportReceiver.cpp $(RX)/sport_crsf.cpp $(RX)/vehicle_sensors.cpp $(FIX)/src/crsf_telemetry.cpp -o $@
clean:
	rm -rf build

# Stage the canonical adapter so its neighbouring Arduino headers can be mocked.
build/adapter/SerialSport.cpp: $(RX)/SerialSport.cpp $(RX)/SerialSport.h $(INC)/SportSources.h
	@mkdir -p build/adapter
	cp $(RX)/SerialSport.cpp $(RX)/SerialSport.h build/adapter/
build/adapter_test: test/adapter_test.cpp build/adapter/SerialSport.cpp $(RX)/SportReceiver.cpp $(RX)/sport_crsf.cpp $(RX)/vehicle_sensors.cpp $(INC)/Er6gSportProfile.h $(wildcard test/stubs/*.h) test/stubs/soc/uart_struct.h test/stubs/hal/uart_ll.h
	$(CXX) $(CXXFLAGS) -DTARGET_RX=1 -DPLATFORM_ESP32=1 -DSPORT_FAS_PHYSICAL_ID=0x22 -DSPORT_CELLS_PHYSICAL_ID=0xa1 -DSPORT_CELLS_APP_ID=0x0300 -Ibuild/adapter -Itest/stubs -I../er4-sport/test/stubs -I$(RX) -I$(INC) test/adapter_test.cpp build/adapter/SerialSport.cpp $(RX)/SportReceiver.cpp $(RX)/sport_crsf.cpp $(RX)/vehicle_sensors.cpp -o $@
