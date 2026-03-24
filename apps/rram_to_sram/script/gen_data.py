#!/usr/bin/env python3
# Copyright 2024 ETH Zurich and University of Bologna.
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Author: Generated for RRAM test

# Generate test data for RRAM
# arg1: size of data array (number of elements)

import random as rand
import numpy as np
import sys

def emit(name, array, alignment='8'):
  """Emit assembly code for an array"""
  print(".global %s" % name)
  print(".balign " + alignment)
  print("%s:" % name)
  bs = array.tobytes()  # Convert array to bytes
  for i in range(0, len(bs), 4):
    s = ""
    for n in range(4):
      s += "%02x" % bs[i+3-n]  # Little-endian byte order
    print("    .word 0x%s" % s)

############
## SCRIPT ##
############

if len(sys.argv) == 2:
  SIZE = int(sys.argv[1])
else:
  print("Error. Give me one argument: SIZE (number of elements).")
  print("Usage: python3 gen_data.py <SIZE>")
  sys.exit(1)

dtype = np.float64

# Generate test data for RRAM
# Create a simple pattern: [1.0, 2.0, 3.0, ..., SIZE]
rram_data = np.arange(1, SIZE + 1, dtype=dtype).astype(dtype)
# Also create expected values for verification
expected_data = np.arange(1, SIZE + 1, dtype=dtype).astype(dtype)

# Create the file
# Note: Use .rram section instead of .data for RRAM
print(".section .rram,\"a\",@progbits")
emit("rram_data", rram_data, 'NR_LANES*4')
emit("rram_data_size", np.array(SIZE, dtype=np.uint64), '8')

# Also emit expected data in .data section (for verification in L2)
print(".section .l2,\"aw\",@progbits")
emit("expected_data", expected_data, 'NR_LANES*4')

# L2 buffer for storing data copied from RRAM
print(".section .l2,\"aw\",@progbits")
emit("l2_buffer", np.zeros(SIZE, dtype=dtype), 'NR_LANES*4')
