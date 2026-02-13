#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <cstring>
#include "memory_pool.hpp"
#include "simd_detector.hpp"

namespace Solver1927 {

// Collision pair for tracking XOR relationships
struct CollisionPair {
    uint32_t index_a;
    uint32_t index_b;
    uint8_t xor_result[32];  // Result of A XOR B
    
    CollisionPair() = default;
    CollisionPair(uint32_t a, uint32_t b) : index_a(a), index_b(b) {}
};

// Stage data structure for pipeline processing
struct StageData {
    std::vector<CollisionPair> collisions;
    size_t collision_count = 0;
    
    void clear() {
        collisions.clear();
        collision_count = 0;
    }
    
    void reserve(size_t capacity) {
        collisions.reserve(capacity);
    }
};

class CollisionDetector {
public:
    // Algorithm parameters
    static constexpr int N = 192;
    static constexpr int K = 7;
    static constexpr int STAGES = K + 1;  // 8 stages total
    static constexpr int COLLISION_BITS = 24;  // N / STAGES = 24
    static constexpr int BUCKET_COUNT = 1 << COLLISION_BITS;  // 2^24 = 16M buckets
    
    CollisionDetector();
    ~CollisionDetector() = default;
    
    // Main collision detection entry point
    bool detect_collisions(MemoryPool* pool, size_t hash_count);
    
    // Stage-specific collision detection
    size_t find_stage_collisions(const uint8_t* input_data, size_t input_count,
                                 StageData& output_stage, int stage_num, bool is_blake2b_input = true);
    
    // Extract collision bit pattern for bucketing
    uint32_t extract_collision_bits(const uint8_t* hash, int stage);
    
    // SIMD-optimized XOR operation
    void compute_xor_simd(const uint8_t* hash_a, const uint8_t* hash_b, uint8_t* result);
    
    // Validate final solutions
    bool validate_solution(const std::vector<uint32_t>& solution_indices);
    
    // Performance metrics
    struct CollisionStats {
        uint64_t total_comparisons = 0;
        uint64_t collisions_found = 0;
        uint64_t buckets_used = 0;
        double avg_bucket_size = 0.0;
        uint32_t max_bucket_size = 0;
    } stats;
    
    void reset_stats() { stats = CollisionStats{}; }
    std::string get_stats_string() const;
    
private:
    // Stage data pipeline
    std::array<StageData, STAGES> stages;
    
    // Bucket management for hash sorting
    struct BucketEntry {
        uint32_t hash_index;
        const uint8_t* hash_ptr;
    };
    
    std::vector<std::vector<BucketEntry>> buckets;
    
    // Internal collision detection methods
    void initialize_buckets();
    void populate_buckets(const uint8_t* hashes, size_t hash_count, int stage, bool is_blake2b_input = true);
    size_t process_bucket_collisions(size_t bucket_id, StageData& output, int stage);
    
    // SIMD dispatch functions
    void (*xor_function)(const uint8_t*, const uint8_t*, uint8_t*) = nullptr;
    
    // Initialize SIMD function pointers based on detected capabilities
    void initialize_simd_functions();
    
    // SIMD implementations
    static void xor_scalar(const uint8_t* a, const uint8_t* b, uint8_t* result);
    static void xor_sse2(const uint8_t* a, const uint8_t* b, uint8_t* result);
    static void xor_avx2(const uint8_t* a, const uint8_t* b, uint8_t* result);
    static void xor_avx512(const uint8_t* a, const uint8_t* b, uint8_t* result);
};

} // namespace Solver1927