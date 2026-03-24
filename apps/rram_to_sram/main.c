// Copyright 2024 ETH Zurich and University of Bologna.
//
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: RRAM_to_SRAM Test Application

#include <string.h>
#include "runtime.h"
#include "util.h"

#ifdef SPIKE
#include <stdio.h>
#elif defined ARA_LINUX
#include <stdio.h>
#else
#include "printf.h"
#endif
// ... existing code ...

// RRAM data - declared as extern, defined in data.S with .rram section
extern double rram_data[] __attribute__((aligned(32 * NR_LANES), section(".rram")));
extern uint64_t rram_data_size;

// Expected data for verification (stored in L2)
extern double expected_data[] __attribute__((aligned(32 * NR_LANES), section(".l2")));

// L2/SRAM buffer to store data copied from RRAM
extern double l2_buffer[] __attribute__((aligned(32 * NR_LANES), section(".l2")));

#define THRESHOLD 0.001

// Copy data from RRAM to L2/SRAM
void copy_rram_to_l2(double *src, double *dst, uint64_t size) {
  for (uint64_t i = 0; i < size; ++i) {
    dst[i] = src[i];
  }
}

// Verify L2 data matches expected data in L2
int verify_l2_data(double *l2, double *expected, uint64_t size, double threshold) {
  int errors = 0;
  for (uint64_t i = 0; i < size; ++i) {
    if (!similarity_check(l2[i], expected[i], threshold)) {
      printf("Error at index %llu: L2[%llu] = %f, Expected = %f\n", 
             (unsigned long long)i, (unsigned long long)i, l2[i], expected[i]);
      errors++;
      if (errors > 10) {  // Limit error output
        printf("Too many errors, stopping verification...\n");
        break;
      }
    }
  }
  return errors;
}

int main() {
  printf("\n");
  printf("=============\n");
  printf("= RRAM to L2 Test =\n");
  printf("=============\n");
  printf("\n");

  // Get data size
  uint64_t size = rram_data_size;
  printf("RRAM data size: %llu elements\n", (unsigned long long)size);
  printf("RRAM data address: 0x%llx\n", (unsigned long long)(uintptr_t)rram_data);
  printf("L2 buffer address: 0x%llx\n", (unsigned long long)(uintptr_t)l2_buffer);
  printf("Expected data address (L2): 0x%llx\n", (unsigned long long)(uintptr_t)expected_data);
  printf("\n");

  // Read and verify first few elements from RRAM
  printf("Reading first 10 elements from RRAM:\n");
  for (uint64_t i = 0; i < (size < 10 ? size : 10); ++i) {
    printf("  RRAM[%llu] = %f\n", (unsigned long long)i, rram_data[i]);
  }
  printf("\n");

  // Display expected data in L2
  printf("Expected data in L2 (first 10 elements):\n");
  for (uint64_t i = 0; i < (size < 10 ? size : 10); ++i) {
    printf("  Expected[%llu] = %f\n", (unsigned long long)i, expected_data[i]);
  }
  printf("\n");

  // Copy data from RRAM to L2/SRAM
  printf("Copying data from RRAM to L2/SRAM...\n");
  start_timer();
  copy_rram_to_l2(rram_data, l2_buffer, size);
  stop_timer();
  
  int64_t copy_runtime = get_timer();
  printf("Copy operation took %lld cycles.\n", (long long)copy_runtime);
  printf("\n");

  // Display first few elements from L2 after copy
  printf("L2 buffer after copy (first 10 elements):\n");
  for (uint64_t i = 0; i < (size < 10 ? size : 10); ++i) {
    printf("  L2[%llu] = %f\n", (unsigned long long)i, l2_buffer[i]);
  }
  printf("\n");

  // Verify L2 data matches expected data in L2
  printf("Verifying L2 data matches expected data in L2...\n");
  start_timer();
  int l2_errors = verify_l2_data(l2_buffer, expected_data, size, THRESHOLD);
  stop_timer();
  
  int64_t verify_runtime = get_timer();
  printf("L2 verification took %lld cycles.\n", (long long)verify_runtime);
  
  if (l2_errors == 0) {
    printf("✓ All %llu elements copied and verified successfully!\n", (unsigned long long)size);
    printf("✓ RRAM to L2 copy test PASSED!\n");
    return 0;
  } else {
    printf("✗ Found %d errors in L2 data\n", l2_errors);
    printf("✗ RRAM to L2 copy test FAILED!\n");
    return 1;
  }
}