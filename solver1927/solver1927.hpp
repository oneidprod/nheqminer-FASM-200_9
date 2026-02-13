#pragma once

#include <string>
#include <functional>
#include <vector>
#include <cstdint>
#include "memory_pool.hpp"
#include "simd_detector.hpp"
#include "blake2b_hasher.hpp"

#pragma once

#include <string>
#include <functional>
#include <vector>
#include <cstdint>
#include "memory_pool.hpp"
#include "simd_detector.hpp"
#include "blake2b_hasher.hpp"
#include "collision_detector.hpp"
#include "../nheqminer/ISolver.h"

class solver1927 : public ISolver {
public:
    // Configuration
    int use_opt = 0;
    
    // ISolver interface implementation (for direct usage)
    solver1927() {}
    solver1927(int platf_id, int dev_id) {}
    virtual ~solver1927() {}
    
    virtual void start() override {
        initialize_memory();
    }
    
    virtual void stop() override {
        cleanup_memory();
    }
    
    virtual void solve(const char *tequihash_header,
                     unsigned int tequihash_header_len,
                     const char* nonce,
                     unsigned int nonce_len,
                     std::function<bool()> cancelf,
                     std::function<void(const std::vector<uint32_t>&, size_t, const unsigned char*)> solutionf,
                     std::function<void(void)> hashdonef) override;
    
    virtual std::string getdevinfo() override { 
        auto level = Solver1927::g_simd_dispatcher.get_active_level();
        std::string simd_name = Solver1927::g_simd_dispatcher.get_active_name();
        return "Solver1927 CPU (N=192, K=7) - " + simd_name + " optimized"; 
    }
    
    virtual std::string getname() override { 
        return "Solver1927 (Equihash 192,7 CPU)"; 
    }
    
    virtual SolverType GetType() const override {
        return SolverType::CPU;
    }
    
    // Static interface methods (for template wrapper compatibility)
    static void start(solver1927& context) {
        context.start();
    }
    
    static void stop(solver1927& context) {
        context.stop();
    }
    
    static void solve(const char *tequihash_header,
                     unsigned int tequihash_header_len,
                     const char* nonce,
                     unsigned int nonce_len,
                     std::function<bool()> cancelf,
                     std::function<void(const std::vector<uint32_t>&, size_t, const unsigned char*)> solutionf,
                     std::function<void(void)> hashdonef,
                     solver1927& context) {
        context.solve(tequihash_header, tequihash_header_len, nonce, nonce_len, 
                     cancelf, solutionf, hashdonef);
    }
    
    // Static utility methods for compatibility
    static int getcount() { return 1; }
    
    static void getinfo(int platf_id, int d_id, std::string& gpu_name, int& sm_count, std::string& version) {
        auto simd_name = Solver1927::g_simd_dispatcher.get_active_name();
        gpu_name = "Solver1927 CPU (" + simd_name + ")";
        sm_count = Solver1927::g_simd_detector.get_parallel_hash_count();
        version = "0.3.0";  
    }
    
    static void print_opencl_devices() {}
    
private:
    // Algorithm parameters for N=192, K=7
    static constexpr int N = 192;
    static constexpr int K = 7;
    static constexpr int COLLISION_BITS = 24; // N / (K+1) = 192/8 = 24
    
    // Memory management
    Solver1927::MemoryManager memory_manager;
    Solver1927::Blake2bManager blake2b_manager;
    Solver1927::CollisionDetector collision_detector;
    
    // Internal methods
    bool initialize_memory();
    void cleanup_memory();
    void report_simd_capabilities() const;
    bool test_blake2b_integration(const char* header, unsigned int header_len,
                                  const char* nonce, unsigned int nonce_len);
    bool run_collision_detection(const char* header, unsigned int header_len,
                                const char* nonce, unsigned int nonce_len,
                                std::function<void(const std::vector<uint32_t>&, size_t, const unsigned char*)> solutionf);
};