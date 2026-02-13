#pragma once

#include <string>
#include <functional>
#include <vector>
#include <cstdint>
#include "memory_pool.hpp"

class solver1927 {
public:
    // Configuration
    int use_opt = 0;
    
    // Required interface methods for solver compatibility
    solver1927() {}
    solver1927(int platf_id, int dev_id) {}
    
    std::string getdevinfo() { 
        return "Solver1927 CPU (N=192, K=7) - AVX2/AVX512 optimized"; 
    }
    
    static int getcount() { return 1; }
    
    static void getinfo(int platf_id, int d_id, std::string& gpu_name, int& sm_count, std::string& version) {
        gpu_name = "Solver1927 CPU";
        sm_count = 0;
        version = "0.2.0";
    }
    
    static void start(solver1927& device_context) {
        device_context.initialize_memory();
    }
    
    static void stop(solver1927& device_context) {
        device_context.cleanup_memory();
    }
    
    static void solve(const char *tequihash_header,
                     unsigned int tequihash_header_len,
                     const char* nonce,
                     unsigned int nonce_len,
                     std::function<bool()> cancelf,
                     std::function<void(const std::vector<uint32_t>&, size_t, const unsigned char*)> solutionf,
                     std::function<void(void)> hashdonef,
                     solver1927& device_context);
                     
    std::string getname() { 
        return "Solver1927 (Equihash 192,7 CPU)"; 
    }
    
    static void print_opencl_devices() {}
    
private:
    // Algorithm parameters for N=192, K=7
    static constexpr int N = 192;
    static constexpr int K = 7;
    static constexpr int COLLISION_BITS = 24; // N / (K+1) = 192/8 = 24
    
    // Memory management
    Solver1927::MemoryManager memory_manager;
    
    // Internal methods
    bool initialize_memory();
    void cleanup_memory();
};