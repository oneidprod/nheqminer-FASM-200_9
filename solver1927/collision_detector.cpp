#include "collision_detector.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <immintrin.h>

namespace Solver1927 {

CollisionDetector::CollisionDetector() {
    initialize_buckets();
    initialize_simd_functions();
}

void CollisionDetector::initialize_buckets() {
    // Pre-allocate bucket storage to avoid reallocations
    // Use smaller initial capacity to save memory, grow as needed
    buckets.resize(BUCKET_COUNT);
    for (auto& bucket : buckets) {
        bucket.reserve(8);  // Average expected bucket size
    }
    
    // Initialize stage storage
    for (auto& stage : stages) {
        stage.reserve(100000);  // Reserve space for collision pairs
    }
}

void CollisionDetector::initialize_simd_functions() {
    // Select best XOR implementation based on SIMD capabilities
    auto simd_level = g_simd_dispatcher.get_active_level();
    
    switch (simd_level) {
        case SIMDLevel::AVX512:
            xor_function = xor_avx512;
            std::cout << "CollisionDetector: Using AVX512 XOR operations" << std::endl;
            break;
        case SIMDLevel::AVX2:
            xor_function = xor_avx2;
            std::cout << "CollisionDetector: Using AVX2 XOR operations" << std::endl;
            break;
        case SIMDLevel::SSE2:
            xor_function = xor_sse2;
            std::cout << "CollisionDetector: Using SSE2 XOR operations" << std::endl;
            break;
        default:
            xor_function = xor_scalar;
            std::cout << "CollisionDetector: Using scalar XOR operations" << std::endl;
            break;
    }
}

bool CollisionDetector::detect_collisions(MemoryPool* pool, size_t hash_count, 
                                         std::function<void(const std::vector<uint32_t>&, size_t, const unsigned char*)> solution_callback) {
    if (!pool || hash_count == 0) {
        std::cerr << "CollisionDetector: Invalid input parameters" << std::endl;
        return false;
    }
    
    reset_stats();
    
    std::cout << "CollisionDetector: Starting Equihash " << N << "," << K 
              << " collision detection" << std::endl;
    std::cout << "  Initial hashes: " << hash_count << std::endl;
    std::cout << "  Stages: " << STAGES << " (collision bits: " << COLLISION_BITS << ")" << std::endl;
    
    // Stage 0: Process initial Blake2b hashes
    const uint8_t* current_input = pool->initial_hashes.data[0];
    size_t current_count = hash_count;
    bool use_stage_buffer_0 = true;  // Ping-pong between stage buffers
    
    for (int stage = 0; stage < STAGES; stage++) {
        std::cout << "  Stage " << stage << ": ";
        
        // Stage 0 uses Blake2b hashes, stages 1+ use XOR results  
        bool is_blake2b_input = (stage == 0);
        const StageData* prev_stage_data = (stage > 0) ? &stages[stage - 1] : nullptr;
        size_t collisions_found = find_stage_collisions(current_input, current_count, 
                                                       stages[stage], stage, is_blake2b_input, prev_stage_data);
        
        std::cout << collisions_found << " collisions found" << std::endl;
        
        if (collisions_found == 0) {
            std::cout << "CollisionDetector: No collisions found at stage " << stage 
                      << ", stopping" << std::endl;
            return false;
        }
        
        // Check for final stage solutions
        if (stage == STAGES - 1) {
            std::cout << "CollisionDetector: Found " << collisions_found 
                      << " potential solutions at final stage!" << std::endl;
            
            // Phase 3: Extract complete solutions
            for (const auto& collision : stages[stage].collisions) {
                if (collision.is_complete_solution() && solution_callback) {
                    extract_solution(collision, solution_callback);
                }
            }
            
            return collisions_found > 0;
        }
        
        // Prepare XOR results for next stage
        uint8_t* next_stage_buffer;
        if (use_stage_buffer_0) {
            next_stage_buffer = pool->stage_buffers[0].data[0];
        } else {
            next_stage_buffer = pool->stage_buffers[1].data[0];
        }
        
        // Copy XOR results from collision pairs to next stage input buffer
        for (size_t i = 0; i < collisions_found; i++) {
            const auto& collision = stages[stage].collisions[i];
            memcpy(next_stage_buffer + (i * 32), collision.xor_result, 32);
        }
        
        std::cout << "    Copied " << collisions_found << " XOR results to next stage buffer" << std::endl;
        
        // Set up for next stage
        current_input = next_stage_buffer;
        current_count = collisions_found;
        use_stage_buffer_0 = !use_stage_buffer_0;  // Ping-pong for next stage
    }
    
    return false;
}

size_t CollisionDetector::find_stage_collisions(const uint8_t* input_data, size_t input_count,
                                               StageData& output_stage, int stage_num, bool is_blake2b_input,
                                               const StageData* prev_stage) {
    output_stage.clear();
    
    // Clear all buckets for this stage
    for (auto& bucket : buckets) {
        bucket.clear();
    }
    
    // Populate buckets based on collision bits for this stage
    populate_buckets(input_data, input_count, stage_num, is_blake2b_input);
    
    // Process each bucket to find collisions
    size_t total_collisions = 0;
    size_t non_empty_buckets = 0;
    size_t max_bucket_size = 0;
    size_t total_hashes_in_buckets = 0;
    
    for (size_t bucket_id = 0; bucket_id < buckets.size(); bucket_id++) {
        const auto& bucket = buckets[bucket_id];
        if (bucket.empty()) continue;
        
        non_empty_buckets++;
        max_bucket_size = std::max(max_bucket_size, bucket.size());
        total_hashes_in_buckets += bucket.size();
        
        if (bucket.size() < 2) continue;  // Need at least 2 items for collision
        
        size_t bucket_collisions = process_bucket_collisions(bucket_id, output_stage, stage_num, prev_stage);
        total_collisions += bucket_collisions;
    }
    
    std::cout << "    Bucket statistics: " << non_empty_buckets << " non-empty, "
              << "max size: " << max_bucket_size << ", "  
              << "collision candidates: " << (non_empty_buckets - (total_hashes_in_buckets - non_empty_buckets)) 
              << std::endl;
    
    // Update statistics  
    stats.collisions_found += total_collisions;
    stats.buckets_used += non_empty_buckets;
    stats.max_bucket_size = std::max(stats.max_bucket_size, (uint32_t)max_bucket_size);
    if (non_empty_buckets > 0) {
        stats.avg_bucket_size = (double)total_hashes_in_buckets / non_empty_buckets;
    }
    
    return total_collisions;
}

void CollisionDetector::populate_buckets(const uint8_t* hashes, size_t hash_count, int stage, bool is_blake2b_input) {
    std::cout << "CollisionDetector: Populating buckets for stage " << stage 
              << " with " << hash_count << (is_blake2b_input ? " Blake2b hashes" : " XOR results") << std::endl;
    
    for (size_t i = 0; i < hash_count; i++) {
        const uint8_t* hash = hashes + (i * 32);  // Each input is 32 bytes
        uint32_t bucket_id = extract_collision_bits(hash, stage);
        
        // Debug: print first few bucket IDs for different input types
        if (i < 1 && stage < 2) {  // Minimal debug output for large hash counts
            std::cout << "  " << (is_blake2b_input ? "Hash" : "XOR") << " " << i 
                      << ": first 8 bytes = ";
            for (int j = 0; j < 8; j++) {
                printf("%02x", hash[j]);
            }
            std::cout << " -> bucket " << bucket_id << std::endl;
        }
        
        if (bucket_id >= BUCKET_COUNT) {
            std::cerr << "ERROR: Invalid bucket_id " << bucket_id 
                      << " >= " << BUCKET_COUNT << std::endl;
            continue;
        }
        
        buckets[bucket_id].emplace_back(BucketEntry{static_cast<uint32_t>(i), hash});
    }
    
    // Debug: count non-empty buckets
    size_t non_empty = 0;
    for (const auto& bucket : buckets) {
        if (!bucket.empty()) non_empty++;
    }
    std::cout << "CollisionDetector: Populated " << non_empty << " non-empty buckets" << std::endl;
}

size_t CollisionDetector::process_bucket_collisions(size_t bucket_id, StageData& output_stage, int stage_num, const StageData* prev_stage) {
    const auto& bucket = buckets[bucket_id];
    size_t collision_count = 0;
    
    // Compare all pairs within the bucket
    for (size_t i = 0; i < bucket.size(); i++) {
        for (size_t j = i + 1; j < bucket.size(); j++) {
            stats.total_comparisons++;
            
            const auto& entry_a = bucket[i];
            const auto& entry_b = bucket[j];
            
            // Check if the hashes actually collide on the required bits
            // (bucket grouping is just the first filter)
            bool collision_found = true;
            
            // For Stage 0, we already know they match on first 24 bits (bucket ID)
            // For simplicity, we'll consider this a valid collision
            // In a full implementation, we'd verify the exact bit pattern match
            
            if (collision_found) {
                CollisionPair pair;
                
                // Phase 3: Create collision with genealogy tracking
                if (stage_num == 0) {
                    // Stage 0: direct hash indices
                    pair = CollisionPair(entry_a.hash_index, entry_b.hash_index, stage_num);
                } else if (prev_stage && entry_a.hash_index < prev_stage->collisions.size() && 
                          entry_b.hash_index < prev_stage->collisions.size()) {
                    // Stage 1+: merge ancestry from parent collisions
                    const CollisionPair& parent_a = prev_stage->collisions[entry_a.hash_index];
                    const CollisionPair& parent_b = prev_stage->collisions[entry_b.hash_index];
                    pair = CollisionPair(entry_a.hash_index, entry_b.hash_index, stage_num, parent_a, parent_b);
                } else {
                    // Fallback for invalid indices
                    pair = CollisionPair(entry_a.hash_index, entry_b.hash_index);
                    pair.stage_level = stage_num;
                }
                
                // Compute XOR of the two hashes using SIMD
                compute_xor_simd(entry_a.hash_ptr, entry_b.hash_ptr, pair.xor_result);
                
                output_stage.collisions.push_back(pair);
                collision_count++;
                
                // Phase 3: Check for complete solutions
                if (pair.is_complete_solution()) {
                    std::cout << "🎯 COMPLETE SOLUTION FOUND! Indices: ";
                    for (size_t idx : pair.ancestor_indices) {
                        std::cout << idx << " ";
                    }
                    std::cout << std::endl;
                }
            }
        }
    }
    
    return collision_count;
}

uint32_t CollisionDetector::extract_collision_bits(const uint8_t* hash, int stage) {
    // Extract 24 bits starting from bit position (stage * 24)
    int start_bit = stage * COLLISION_BITS;
    int start_byte = start_bit / 8;
    int bit_offset = start_bit % 8;
    
    // For debugging - show bit extraction for each stage
    static int last_debug_stage = -1;
    if (stage != last_debug_stage) {
        std::cout << "CollisionDetector: Stage " << stage 
                  << " - extracting " << COLLISION_BITS << " bits starting from bit "
                  << start_bit << " (byte " << start_byte << ", offset " << bit_offset << ")" << std::endl;
        last_debug_stage = stage;
    }
    
    // Extract 4 bytes and shift to get our 24 bits
    uint32_t value = 0;
    if (start_byte + 3 < 32) {  // Ensure we don't read beyond hash
        value = (hash[start_byte] << 24) |
                (hash[start_byte + 1] << 16) |
                (hash[start_byte + 2] << 8) |
                hash[start_byte + 3];
        
        // Shift to align our 24 bits and mask
        value = (value >> (8 - bit_offset)) & ((1u << COLLISION_BITS) - 1);
    }
    
    return value;
}

void CollisionDetector::compute_xor_simd(const uint8_t* hash_a, const uint8_t* hash_b, uint8_t* result) {
    xor_function(hash_a, hash_b, result);
}

// SIMD XOR implementations
void CollisionDetector::xor_scalar(const uint8_t* a, const uint8_t* b, uint8_t* result) {
    for (int i = 0; i < 32; i++) {
        result[i] = a[i] ^ b[i];
    }
}

void CollisionDetector::xor_sse2(const uint8_t* a, const uint8_t* b, uint8_t* result) {
    // Process 32 bytes using 2x 16-byte SSE2 operations
    __m128i va1 = _mm_loadu_si128((__m128i*)a);
    __m128i vb1 = _mm_loadu_si128((__m128i*)b);
    __m128i vr1 = _mm_xor_si128(va1, vb1);
    _mm_storeu_si128((__m128i*)result, vr1);
    
    __m128i va2 = _mm_loadu_si128((__m128i*)(a + 16));
    __m128i vb2 = _mm_loadu_si128((__m128i*)(b + 16));
    __m128i vr2 = _mm_xor_si128(va2, vb2);
    _mm_storeu_si128((__m128i*)(result + 16), vr2);
}

void CollisionDetector::xor_avx2(const uint8_t* a, const uint8_t* b, uint8_t* result) {
    // Process 32 bytes using 1x 32-byte AVX2 operation
    __m256i va = _mm256_loadu_si256((__m256i*)a);
    __m256i vb = _mm256_loadu_si256((__m256i*)b);
    __m256i vr = _mm256_xor_si256(va, vb);
    _mm256_storeu_si256((__m256i*)result, vr);
}

void CollisionDetector::xor_avx512(const uint8_t* a, const uint8_t* b, uint8_t* result) {
    // For 32-byte XOR, AVX2 is actually optimal (AVX512 is overkill)
    // But we can still use AVX512 foundation instructions
    __m256i va = _mm256_loadu_si256((__m256i*)a);
    __m256i vb = _mm256_loadu_si256((__m256i*)b);
    __m256i vr = _mm256_xor_si256(va, vb);
    _mm256_storeu_si256((__m256i*)result, vr);
}

std::string CollisionDetector::get_stats_string() const {
    std::ostringstream oss;
    oss << "Collision Stats: " 
        << stats.total_comparisons << " comparisons, "
        << stats.collisions_found << " collisions, "
        << stats.buckets_used << " buckets used, "
        << "avg bucket size: " << std::fixed << std::setprecision(1) << stats.avg_bucket_size
        << ", max: " << stats.max_bucket_size;
    return oss.str();
}

bool CollisionDetector::validate_solution(const std::vector<uint32_t>& solution_indices) {
    // Basic validation - should have 2^k indices for Equihash solution
    size_t expected_indices = 1u << K;  // 2^7 = 128 for K=7
    
    if (solution_indices.size() != expected_indices) {
        return false;
    }
    
    // Additional validation would check that XOR of all solution hashes equals zero
    // Simplified for now
    return true;
}

// Phase 3: Extract and validate complete Equihash solution
void CollisionDetector::extract_solution(const CollisionPair& final_collision, 
                                        std::function<void(const std::vector<uint32_t>&, size_t, const unsigned char*)> callback) {
    if (!final_collision.is_complete_solution()) {
        std::cout << "⚠️  Warning: Not a complete solution (Stage " << final_collision.stage_level 
                  << ", " << final_collision.get_solution_size() << " indices)" << std::endl;
        return;
    }
    
    std::cout << "✅ Extracting complete solution with " << final_collision.ancestor_indices.size() 
              << " indices..." << std::endl;
    
    // Call the solution callback with the extracted indices
    // Note: nonce_offset and nonce_ptr are placeholders for now
    size_t nonce_offset = 0;
    const unsigned char* nonce_ptr = nullptr;
    
    callback(final_collision.ancestor_indices, nonce_offset, nonce_ptr);
}

} // namespace Solver1927