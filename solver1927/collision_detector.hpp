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

// Collision pair for tracking XOR relationships with memory-optimized genealogy
struct CollisionPair {
    uint32_t index_a;
    uint32_t index_b;
    uint8_t xor_result[32];  // Result of A XOR B
    
    // Memory-optimized genealogy: only track at later stages
    // Early stages (0-3) use indices directly, late stages (4-7) track full genealogy
    union {
        struct {
            uint32_t ancestors[16];  // Fixed-size array for Stage 4+ (max 2^4 = 16 solutions)
            uint8_t ancestor_count;
        } late_stage;
        struct {
            uint32_t parent_a_idx;   // Index into previous stage's collision array
            uint32_t parent_b_idx;
        } early_stage;
    } genealogy;
    
    int stage_level = 0;                     // Stage where this collision was found
    
    CollisionPair() = default;
    CollisionPair(uint32_t a, uint32_t b) : index_a(a), index_b(b) {}
    
    // Memory-optimized initialization for early stages (0-3)
    CollisionPair(uint32_t a, uint32_t b, int stage) : index_a(a), index_b(b), stage_level(stage) {
        if (stage <= 3) {
            // Early stages: just store parent indices
            genealogy.early_stage.parent_a_idx = a;
            genealogy.early_stage.parent_b_idx = b;
        } else {
            // Late stages: initialize ancestor array
            genealogy.late_stage.ancestor_count = 0;
        }
    }
    
    // Memory-optimized initialization with parent tracking
    CollisionPair(uint32_t a, uint32_t b, int stage, const CollisionPair& parent_a, const CollisionPair& parent_b) 
        : index_a(a), index_b(b), stage_level(stage) {
        if (stage <= 3) {
            // Early stages: store parent collision indices for later reconstruction
            genealogy.early_stage.parent_a_idx = a;
            genealogy.early_stage.parent_b_idx = b;
        } else {
            // Late stages: build full ancestor list only when needed
            genealogy.late_stage.ancestor_count = 0;
            // Reconstruct full genealogy from parent collisions when we reach late stages
            build_full_genealogy(parent_a, parent_b);
        }
    }
    
    // Build full genealogy only for late stages (Stage 4+)
    void build_full_genealogy(const CollisionPair& parent_a, const CollisionPair& parent_b) {
        // Implementation deferred - only needed if we reach Stage 4+
        genealogy.late_stage.ancestor_count = 0;
    }
    
    // Get solution size efficiently
    size_t get_solution_size() const { 
        if (stage_level <= 3) {
            return 1 << (stage_level + 1);  // 2^(stage+1)
        } else {
            return genealogy.late_stage.ancestor_count;
        }
    }
    
    // Phase 3: Check if this is a complete solution (final stage K)
    bool is_complete_solution() const { 
        return stage_level == K && get_solution_size() == (1u << K); // 2^7 = 128 indices at final Stage 7
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
    
    // Processing limits to prevent O(n²) explosion in bucket processing
    static constexpr size_t MAX_BUCKET_SIZE_LIMIT = 3000;      // Skip buckets larger than this (increased for later stages)
    static constexpr size_t MAX_BUCKET_PAIRS = 750000;         // Max pairs per bucket (increased capacity)  
    static constexpr size_t MAX_TOTAL_COLLISIONS_PER_STAGE = 8000000;  // Cap total collisions per stage (Stage 0: 8M for downstream density)
    
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
    bool verify_collision_bits(const uint8_t* hash_a, const uint8_t* hash_b, int stage);
    
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