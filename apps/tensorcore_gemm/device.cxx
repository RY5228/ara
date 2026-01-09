#include "riscv/mmio_plugin.h"
#include <cstring>
#include <cstdio>
#include <cstdint>
#include "kernel/gemm_instruction.h"

// TensorCore外设状态
struct tensorcore_state {
    // 指令缓冲区 (40字节 = 5个64位字)
    uint64_t instruction_buffer[5];
    bool instruction_valid;
    
    // 状态寄存器
    uint64_t status_reg;  // 0xD0002000
    
    // 执行状态
    bool executing;
    uint64_t instruction_count;
    
    tensorcore_state(const std::string& args) {
        memset(instruction_buffer, 0, sizeof(instruction_buffer));
        instruction_valid = false;
        status_reg = 0;
        executing = false;
        instruction_count = 0;
        
        printf("[TensorCore] GEMM device initialized\n");
        if (!args.empty()) {
            printf("[TensorCore] Args: %s\n", args.c_str());
        }
    }
    
    // 解析并执行指令
    void execute_instruction() {
        if (!instruction_valid) return;
        
        // 将缓冲区转换为指令结构
        mma_instruction_t inst;
        memcpy(inst.raw64, instruction_buffer, sizeof(instruction_buffer));
        
        // 解析指令字段
        op_info_t din = inst.fields.Din;
        op_info_t b = inst.fields.B;
        op_info_t a = inst.fields.A;
        op_info_t dout = inst.fields.Dout;
        mma_meta_t meta = inst.fields.mma_meta;
        
        printf("\n[TensorCore] ===== GEMM Instruction Received =====\n");
        printf("[TensorCore] Instruction Type: %u\n", meta.info.instruction_type);
        printf("[TensorCore] Matrix Dimensions: M=%u, N=%u, K=%u\n", 
               meta.info.M, meta.info.N, meta.info.K);
        printf("[TensorCore] Transpose: A=%u, B=%u\n", 
               meta.info.if_A_transpose, meta.info.if_B_transpose);
        
        printf("[TensorCore] Din: base=0x%06x, stride_minor=%u, stride_major=%u\n",
               din.info.base_addr, din.info.stride_minor, din.info.stride_major);
        printf("[TensorCore] A:  base=0x%06x, stride_minor=%u, stride_major=%u\n",
               a.info.base_addr, a.info.stride_minor, a.info.stride_major);
        printf("[TensorCore] B:  base=0x%06x, stride_minor=%u, stride_major=%u\n",
               b.info.base_addr, b.info.stride_minor, b.info.stride_major);
        printf("[TensorCore] Dout: base=0x%06x, stride_minor=%u, stride_major=%u\n",
               dout.info.base_addr, dout.info.stride_minor, dout.info.stride_major);
        
        // 计算实际地址（根据gemm.c中的地址计算方式）
        // base_addr是除以对齐大小后的值，需要转换回实际地址
        uint64_t din_addr = (uint64_t)din.info.base_addr * 16384;
        uint64_t a_addr = (uint64_t)a.info.base_addr * 4096;
        uint64_t b_addr = (uint64_t)b.info.base_addr * 4096;
        uint64_t dout_addr = (uint64_t)dout.info.base_addr * 16384;
        
        printf("[TensorCore] Calculated Addresses:\n");
        printf("[TensorCore]   Din:  0x%016lx\n", din_addr);
        printf("[TensorCore]   A:    0x%016lx\n", a_addr);
        printf("[TensorCore]   B:    0x%016lx\n", b_addr);
        printf("[TensorCore]   Dout: 0x%016lx\n", dout_addr);
        
        // 模拟GEMM执行
        // 在实际硬件中，这里会执行: Dout = A * B + Din
        // 在仿真中，我们只记录指令，不实际计算
        
        // 更新状态寄存器
        status_reg = 0x1; // 指令已接收
        instruction_count++;
        
        printf("[TensorCore] Instruction executed (count: %lu)\n", instruction_count);
        printf("[TensorCore] ======================================\n\n");
        
        instruction_valid = false;
    }
    
    // 读取寄存器
    bool load(reg_t addr, size_t len, uint8_t* bytes) {
        uint64_t offset = addr;
        
        // 状态寄存器 (0xD0002000)
        if (offset == 0x1000) {  // 相对于基地址0xD0001000的偏移
            uint64_t value = status_reg;
            if (len == 8) {
                memcpy(bytes, &value, 8);
            } else if (len == 4) {
                uint32_t val32 = (uint32_t)value;
                memcpy(bytes, &val32, 4);
            } else {
                return false;
            }
            printf("[TensorCore] Read status: 0x%016lx\n", value);
            return true;
        }
        
        // 指令缓冲区读取（通常不支持，但可以用于调试）
        if (offset < 0x28) {  // 40字节 = 0x28
            uint64_t idx = offset / 8;
            uint64_t off = offset % 8;
            if (idx < 5) {
                uint64_t value = instruction_buffer[idx];
                if (len == 8 && off == 0) {
                    memcpy(bytes, &value, 8);
                } else if (len == 4 && off == 0) {
                    uint32_t val32 = (uint32_t)value;
                    memcpy(bytes, &val32, 4);
                } else {
                    return false;
                }
                return true;
            }
        }
        
        printf("[TensorCore] Invalid read offset: 0x%lx (len=%zu)\n", 
               (unsigned long)offset, len);
        return false;
    }
    
    // 写入寄存器
    bool store(reg_t addr, size_t len, const uint8_t* bytes) {
        uint64_t offset = addr;
        
        // 指令缓冲区写入 (0xD0001000 + offset, 0 <= offset < 40)
        if (offset < 0x28) {  // 40字节 = 0x28
            uint64_t idx = offset / 8;
            uint64_t off = offset % 8;
            
            if (idx < 5 && off == 0 && len == 8) {
                // 64位写入
                memcpy(&instruction_buffer[idx], bytes, 8);
                printf("[TensorCore] Write instruction[%lu] = 0x%016lx\n", 
                       idx, instruction_buffer[idx]);
                
                // 检查是否写完了整个指令（5个64位字 = 40字节）
                // 当写入最后一个字时，触发指令执行
                if (idx == 4) {
                    instruction_valid = true;
                    printf("[TensorCore] Complete instruction received, executing...\n");
                    execute_instruction();
                }
                return true;
            } else if (len == 4 && off == 0) {
                // 32位写入（需要合并到64位）
                uint32_t val32;
                memcpy(&val32, bytes, 4);
                if (idx < 5) {
                    // 写入低32位
                    instruction_buffer[idx] = (instruction_buffer[idx] & 0xFFFFFFFF00000000ULL) | val32;
                    printf("[TensorCore] Write instruction[%lu][31:0] = 0x%08x\n", idx, val32);
                    return true;
                }
            } else {
                printf("[TensorCore] Unaligned or invalid write: offset=0x%lx, len=%zu\n",
                       (unsigned long)offset, len);
                return false;
            }
        }
        
        // 状态寄存器写入（通常只读，但可以用于清除状态）
        if (offset == 0x1000) {
            if (len == 8) {
                memcpy(&status_reg, bytes, 8);
                printf("[TensorCore] Status register written: 0x%016lx\n", status_reg);
            } else if (len == 4) {
                uint32_t val32;
                memcpy(&val32, bytes, 4);
                status_reg = (status_reg & 0xFFFFFFFF00000000ULL) | val32;
                printf("[TensorCore] Status register written (low 32): 0x%08x\n", val32);
            }
            return true;
        }
        
        printf("[TensorCore] Invalid write offset: 0x%lx (len=%zu)\n", 
               (unsigned long)offset, len);
        return false;
    }
};

// 使用模板自动注册插件
// 注意：基地址会在运行时通过--device参数指定
static mmio_plugin_registration_t<tensorcore_state> tensorcore_registration("tensorcore_gemm");