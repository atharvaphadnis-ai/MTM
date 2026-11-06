# MicroTask Monitor build config for the host-side test suite.
# Run with `make` and execute with `./mtm_tests`.

CXX      ?= g++
CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra -Wno-unused-parameter \
            -I../src -I. -DHOST_BUILD
LDFLAGS  ?=

SRCS = MTMTests.cpp \
       ../src/telemetry/MTMSerializers.cpp \
       ../src/alerts/MTMAlerts.cpp \
       ../src/hal/MTMPlatformFactory.cpp

OBJS = $(patsubst %.cpp,%.o,$(SRCS))
TARGET = mtm_tests

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(SRCS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o ../src/telemetry/*.o ../src/alerts/*.o ../src/hal/*.o

.PHONY: all run clean
