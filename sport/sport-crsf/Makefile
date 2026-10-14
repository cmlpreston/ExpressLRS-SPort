CXX ?= c++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -g -fsanitize=address,undefined
RX := ../../src/src/rx-serial
INC := ../../src/include
FIX := ../compatibility/jr-r1
.PHONY: test clean
test: build/protocol_test
	./build/protocol_test
build/protocol_test: test/protocol_test.cpp $(RX)/sport_crsf.cpp $(INC)/sport_crsf.h $(RX)/vehicle_sensors.cpp $(INC)/vehicle_sensors.h $(FIX)/src/crsf_telemetry.cpp $(FIX)/include/crsf_telemetry.h test/Arduino.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -I$(INC) -Itest -I$(FIX)/include test/protocol_test.cpp $(RX)/sport_crsf.cpp $(RX)/vehicle_sensors.cpp $(FIX)/src/crsf_telemetry.cpp -o $@
clean:
	rm -rf build
