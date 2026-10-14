CXX ?= c++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -g -fsanitize=address,undefined
RX := ../../src/src/rx-serial
INC := ../../src/include
FIX := ../compatibility/jr-r1
.PHONY: test clean
test: build/receiver_test
	./build/receiver_test
build/receiver_test: test/receiver_test.cpp $(RX)/SportReceiver.cpp $(RX)/SportReceiver.h $(RX)/sport_crsf.cpp $(RX)/vehicle_sensors.cpp $(INC)/sport_crsf.h $(INC)/vehicle_sensors.h $(FIX)/src/crsf_telemetry.cpp $(FIX)/include/crsf_telemetry.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -I$(RX) -I$(INC) -I../sport-crsf/test -I$(FIX)/include test/receiver_test.cpp $(RX)/SportReceiver.cpp $(RX)/sport_crsf.cpp $(RX)/vehicle_sensors.cpp $(FIX)/src/crsf_telemetry.cpp -o $@
clean:
	rm -rf build
