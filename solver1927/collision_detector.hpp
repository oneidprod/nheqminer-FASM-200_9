#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <functional>
#include "memory_pool.hpp"
#include "simd_detector.hpp"

namespace Solver1927 {

// Collision pair for tracking XOR relationships with genealogy
struct CollisionPair {
    uint32_t index_a;
    uint32_t index_b;
    uint8_t xor_result[32];  // Result of A XOR B
    
    // Phase 3: Solution genealogy tracking
    std::vector<uint32_t> ancestor_indices;  // Original hash indices contributing to this collision
    int stage_level = 0;                     // Stage where this collision was found
    
    CollisionPair() = default;
    CollisionPair(uint32_t a, uint32_t b) : index_a(a), index_b(b) {}
    
    // Phase 3: Initialize with original hash indices (Stage 0)
    CollisionPair(uint32_t a, uint32_t b, int stage) : index_a(a), index_b(b), stage_level(stage) {
        if (stage == 0) {
            // Stage 0: direct hash indices
            ancestor_indices = {a, b};
        }
    }
    
    // Phase 3: Initialize with parent collision ancestry (Stage 1+)
    CollisionPair(uint32_t a, uint32_t b, int stage, const CollisionPair& parent_a, const CollisionPair& parent_b) 
        : index_a(a), index_b(b), stage_level(stage) {
        // Merge ancestor indices from both parents
        ancestor_indices.insert(ancestor_indices.end(), parent_a.ancestor_indices.begin(), parent_a.ancestor_indices.end());
        ancestor_indices.insert(ancestor_indices.end(), parent_b.ancestor_indices.begin(), parent_b.ancestor_indices.end());
        
        // Sort for consistent solution representation
        std::sort(ancestor_indices.begin(), ancestor_indices.end());
    }
    
    // Phase 3: Get solution size (should be 2^(stage+1))
    size_t get_solution_size() const { 
        return ancestor_indices.size(); 
    }
    
    // Phase 3: Check if this is a complete solution (Stage K)
    bool is_complete_solution() const { 
        return stage_level == 7 && ancestor_indices.size() == 256; // 2^8 = 256 indices
    }
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
    // Main collision detection entry point with solution callback
    bool detect_collisions(MemoryPool* pool, size_t hash_count, 
                          std::function<void(const std::vector<uint32_t>&, size_t, const unsigned char*)> solution_callback = nullptr);
    
    // Stage-specific collision detection with genealogy tracking
    size_t find_stage_collisions(const uint8_t* input_data, size_t input_count,
                                 StageData& output_stage, int stage_num, bool is_blake2b_input = true,
                                 const StageData* prev_stage = nullptr);
    
    // Extract collision bit pattern for bucketing
    uint32_t extract_collision_bits(const uint8_t* hash, int stage);
    
    // SIMD-optimized XOR operation
    void compute_xor_simd(const uint8_t* hash_a, const uint8_t* hash_b, uint8_t* result);
    
    // Phase 3: Solution extraction and validation
    void extract_solution(const CollisionPair& final_collision, std::function<void(const std::vector<uint32_t>&, size_t, const unsigned char*)> callback);
    
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
    size_t process_bucket_collisions(size_t bucket_id, StageData& output, int stage, const StageData* prev_stage = nullptr);
    
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