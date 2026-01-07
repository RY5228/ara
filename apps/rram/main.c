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

// Author: RRAM Test Application

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

// RRAM data - declared as extern, defined in data.S with .rram section
extern double rram_data[] __attribute__((aligned(32 * NR_LANES), section(".rram")));
extern uint64_t rram_data_size;

// Expected data for verification (stored in rram)
extern double expected_data[] __attribute__((aligned(32 * NR_LANES), section(".rram")));

#define THRESHOLD 0.001

// Verify RRAM data
int verify_rram_data(double *rram, double *expected, uint64_t size, double threshold) {
  int errors = 0;
  for (uint64_t i = 0; i < size; ++i) {
    if (!similarity_check(rram[i], expected[i], threshold)) {
      printf("Error at index %llu: RRAM[%llu] = %f, Expected = %f\n", 
             (unsigned long long)i, (unsigned long long)i, rram[i], expected[i]);
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
  printf("= RRAM Test =\n");
  printf("=============\n");
  printf("\n");

  // Get data size
  uint64_t size = rram_data_size;
  printf("RRAM data size: %llu elements\n", (unsigned long long)size); //%llu：无符号长长整型格式
  printf("RRAM data address: 0x%llx\n", (unsigned long long)(uintptr_t)rram_data); //%llx：长长整型十六进制格式
  printf("Expected data address: 0x%llx\n", (unsigned long long)(uintptr_t)expected_data); //(uintptr_t)：将指针转换为无符号整数类型，用于打印地址
  printf("\n");

  // Read and verify first few elements
  printf("Reading first 10 elements from RRAM:\n");
  for (uint64_t i = 0; i < (size < 10 ? size : 10); ++i) {
    printf("  RRAM[%llu] = %f\n", (unsigned long long)i, rram_data[i]);
  }
  printf("\n");

  // Verify all data
  printf("Verifying RRAM data against expected values...\n");
  start_timer();
  int errors = verify_rram_data(rram_data, expected_data, size, THRESHOLD);
  stop_timer();

  int64_t runtime = get_timer();
  printf("Verification took %lld cycles.\n", (long long)runtime);

  if (errors == 0) {
    printf("✓ All %llu elements verified successfully!\n", (unsigned long long)size);
    printf("✓ RRAM test PASSED!\n");
    return 0;
  } else {
    printf("✗ Found %d errors out of %llu elements\n", errors, (unsigned long long)size);
    printf("✗ RRAM test FAILED!\n");
    return 1;
  }
}

