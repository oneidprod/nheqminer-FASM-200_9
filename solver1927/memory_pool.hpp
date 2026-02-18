#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include <array>

#ifdef _WIN32
#include <malloc.h>
#else
#include <cstdlib>
#include <mm_malloc.h>
#endif

#ifndef _WIN32
// For posix_memalign
#include <cstdlib>
#endif

namespace Solver1927 {

// Equihash 192,7 algorithm constants
constexpr int N = 192;
constexpr int K = 7;
constexpr int COLLISION_BITS = 24;  // N / (K+1) = 192/8 = 24
constexpr int STAGES = 8;           // K+1 = 8 collision stages
constexpr int BUCKETS = 256;        // 2^8 buckets per stage

// Memory sizing calculations for L3 cache optimization
constexpr size_t INITIAL_HASHES = 16 * 1024 * 1024;   // 16,777,216 initial hashes (16M proven Stage 3)
constexpr size_t STAGE_ENTRIES = 1 << 22;       // 4,194,304 entries per stage buffer (scale for high collision density)
constexpr size_t BUCKET_SIZE = 8192;            // Large buckets for high collision density

/**
 * 64-byte aligned memory allocation for cache optimization
 */
class AlignedAllocator {
public:
    static void* allocate(size_t size, size_t alignment = 64) {
        #ifdef _WIN32
            return _aligned_malloc(size, alignment);
        #else
            void* ptr = nullptr;
            if (posix_memalign(&ptr, alignment, size) != 0) {
                return nullptr;
            }
            return ptr;
        #endif
    }
    
    static void deallocate(void* ptr) {
        #ifdef _WIN32
            _aligned_free(ptr);
        #else
            std::free(ptr);
        #endif
    }
};

/**
 * Core memory pool for Equihash 192,7 solver
 * Designed for 32-48MB total allocation to fit in L3 cache
 */
struct alignas(64) MemoryPool {
    // Stage 0: Initial Blake2b hash generation (~8.4MB)
    struct {
        alignas(64) uint8_t data[INITIAL_HASHES][32];
        size_t count;
    } initial_hashes;
    
    // Double-buffered stage data for collision detection (~25.2MB total)
    struct {
        alignas(64) uint8_t data[STAGE_ENTRIES][48];  // 48 bytes: 24 collision + 24 indices
        size_t count;
    } stage_buffers[2];
    
    // Bucket arrays for fast collision lookup (~2MB)
    struct {
        alignas(64) uint32_t indices[BUCKETS][BUCKET_SIZE];
        uint16_t counts[BUCKETS];
    } buckets;
    
    // Solution tracking (variable size, <1MB typically)
    struct {
        std::vector<uint32_t> indices;
        std::vector<uint8_t> collision_data;
    } solutions[STAGES];
    
    // Memory usage statistics  
    size_t total_allocated_bytes;
    bool is_initialized;
    
    MemoryPool() : total_allocated_bytes(0), is_initialized(false) {
        // Zero-initialize critical sections
        memset(&initial_hashes, 0, sizeof(initial_hashes)); 
        memset(&stage_buffers, 0, sizeof(stage_buffers));
        memset(&buckets, 0, sizeof(buckets));
    }
    
    // Calculate actual memory usage
    size_t get_memory_usage() const {
        size_t usage = sizeof(initial_hashes) + sizeof(stage_buffers) + sizeof(buckets);
        for (int i = 0; i < STAGES; i++) {
            usage += solutions[i].indices.capacity() * sizeof(uint32_t);
            usage += solutions[i].collision_data.capacity();
        }
        return usage;
    }
};

/**
 * Memory pool manager with RAII and statistics
 */
class MemoryManager {
private:
    MemoryPool* pool;
    
public:
    MemoryManager() : pool(nullptr) {}
    
    ~MemoryManager() {
        deallocate();
    }
    
    bool allocate() {
        if (pool) return true;  // Already allocated
        
        pool = static_cast<MemoryPool*>(
            AlignedAllocator::allocate(sizeof(MemoryPool), 64)
        );
        
        if (!pool) return false;
        
        // Initialize with placement new
        new(pool) MemoryPool();
        pool->is_initialized = true;
        pool->total_allocated_bytes = sizeof(MemoryPool);
        
        return true;
    }
    
    void deallocate() {
        if (pool) {
            pool->~MemoryPool();
            AlignedAllocator::deallocate(pool);
            pool = nullptr;
        }
    }
    
    MemoryPool* get() { return pool; }
    const MemoryPool* get() const { return pool; }
    
    bool is_valid() const { 
        return pool && pool->is_initialized; 
    }
    
    // Memory usage reporting
    size_t get_total_memory() const {
        return pool ? pool->get_memory_usage() : 0;
    }
    
    double get_memory_mb() const {
        return get_total_memory() / (1024.0 * 1024.0);
    }
};

} // namespace Solver1927