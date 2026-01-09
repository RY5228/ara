# 编译TensorCore GEMM设备插件
ARA_DIR := $(shell git rev-parse --show-toplevel 2>/dev/null || echo $$ARA_DIR)
SPIKE_DIR := $(ARA_DIR)/install/riscv-isa-sim
CXX = g++
CXXFLAGS = -fPIC -shared -std=c++11 -O2 -Wall
INCLUDES = -I$(SPIKE_DIR)/include

DEVICE_LIB = libtensorcore_gemm.so

all: $(DEVICE_LIB)

$(DEVICE_LIB): device.cxx
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $<

clean:
	rm -f $(DEVICE_LIB)

.PHONY: all clean