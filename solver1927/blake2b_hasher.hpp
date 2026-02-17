#pragma once

#include <cstdint>
#include <cstring>
#include "memory_pool.hpp"
#include "simd_detector.hpp"

// Include the existing Blake2b implementation
extern "C" {
    #include "../blake2/blake2.h"
}

namespace Solver1927 {

/**
 * Blake2b hasher optimized for Equihash 192,7
 * Integrates with memory pool and SIMD acceleration
 */
class Blake2bHasher {
private:
    // Equihash 192,7 Blake2b parameters
    static constexpr size_t HASH_OUTPUT_BYTES = 32;  // 256-bit output
    static constexpr size_t INPUT_BLOCK_SIZE = 140;   // Header + nonce + index
    static constexpr const char* EQUIHASH_PERSONALIZATION = "ZERO_PoW";  // Zero coin personalization
    
    // Blake2b state for current solving session
    blake2b_state base_state;
    bool is_initialized;
    
    // Performance tracking
    size_t hashes_generated;
    
public:
    Blake2bHasher() : is_initialized(false), hashes_generated(0) {}
    
    // Initialize Blake2b with Equihash parameters
    bool initialize(uint32_t n, uint32_t k);
    
    // Set header and nonce for current solve session
    bool set_header_nonce(const uint8_t* header, size_t header_len, 
                         const uint8_t* nonce, size_t nonce_len);
    
    // Generate initial hashes for collision detection
    size_t generate_initial_hashes(MemoryPool* pool, size_t target_count);
    
    // Single hash generation (for testing)
    bool generate_hash(uint32_t index, uint8_t* output);
    
    // SIMD-accelerated batch hashing (stubs for now)
    size_t generate_batch_sse2(MemoryPool* pool, uint32_t start_index, size_t count);
    size_t generate_batch_avx2(MemoryPool* pool, uint32_t start_index, size_t count);
    size_t generate_batch_avx512(MemoryPool* pool, uint32_t start_index, size_t count);
    
    // Statistics and debugging
    size_t get_hash_count() const { return hashes_generated; }
    void reset_stats() { hashes_generated = 0; }
    bool is_ready() const { return is_initialized; }
    
 private:
    // Helper methods
    void setup_blake2b_params(blake2b_param* params, uint32_t n, uint32_t k);
    void write_index_to_buffer(uint32_t index, uint8_t* buffer);
};

/**
 * Blake2b integration manager - coordinates with SIMD dispatcher
 */
class Blake2bManager {
private:
    Blake2bHasher hasher;
    SIMDLevel active_simd;
    
public:
    Blake2bManager() : active_simd(SIMDLevel::NONE) {
        active_simd = g_simd_dispatcher.get_active_level();
    }
    
    // Primary interface
    bool initialize_for_solve(const uint8_t* header, size_t header_len,
                             const uint8_t* nonce, size_t nonce_len);
    
    // Generate hashes using best available SIMD
    size_t generate_hashes(MemoryPool* pool, size_t target_count);
    
    // Statistics
    size_t get_total_hashes() const { return hasher.get_hash_count(); }
    std::string get_performance_info() const;
    
private:
    // SIMD dispatch helpers
    size_t dispatch_hash_generation(MemoryPool* pool, uint32_t start_index, size_t count);
};

} // namespace Solver1927