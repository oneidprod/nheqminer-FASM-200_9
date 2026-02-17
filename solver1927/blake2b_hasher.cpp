#include "blake2b_hasher.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace Solver1927 {

bool Blake2bHasher::initialize(uint32_t n, uint32_t k) {
    blake2b_param params;
    setup_blake2b_params(&params, n, k);
    
    int result = blake2b_init_param(&base_state, &params);
    if (result != 0) {
        std::cerr << "Blake2bHasher: Failed to initialize Blake2b parameters" << std::endl;
        return false;
    }
    
    is_initialized = true;
    hashes_generated = 0;
    
    std::cout << "Blake2bHasher: Initialized for Equihash N=" << n << ", K=" << k << std::endl;
    std::cout << "  Hash output: " << HASH_OUTPUT_BYTES << " bytes" << std::endl;
    std::cout << "  Personalization: " << EQUIHASH_PERSONALIZATION << std::endl;
    
    return true;
}

void Blake2bHasher::setup_blake2b_params(blake2b_param* params, uint32_t n, uint32_t k) {
    // Zero-initialize the parameter structure
    memset(params, 0, sizeof(blake2b_param));
    
    // Set Blake2b parameters for Equihash
    params->digest_length = HASH_OUTPUT_BYTES;  // 32 bytes output
    params->key_length = 0;                     // No key
    params->fanout = 1;                         // Sequential hashing
    params->depth = 1;                          // Single level
    params->leaf_length = 0;                    // Not a tree hash
    params->node_offset = 0;                    // Root node
    params->node_depth = 0;                     // Leaf node
    params->inner_length = 0;                   // Not inner node
    
    // Set Equihash personalization: "ZERO_PoW" + n + k
    memcpy(params->personal, EQUIHASH_PERSONALIZATION, 8);
    
    // Encode n and k in little-endian format (last 8 bytes of personalization)
    params->personal[8] = n & 0xff;
    params->personal[9] = (n >> 8) & 0xff;  
    params->personal[10] = (n >> 16) & 0xff;
    params->personal[11] = (n >> 24) & 0xff;
    params->personal[12] = k & 0xff;
    params->personal[13] = (k >> 8) & 0xff;
    params->personal[14] = (k >> 16) & 0xff;
    params->personal[15] = (k >> 24) & 0xff;
}

bool Blake2bHasher::set_header_nonce(const uint8_t* header, size_t header_len,
                                     const uint8_t* nonce, size_t nonce_len) {
    if (!is_initialized) {
        std::cerr << "Blake2bHasher: Not initialized!" << std::endl;
        return false;
    }
    
    // For Equihash, we create a base state with header + nonce
    // Each hash will append a 4-byte index to this base
    base_state = blake2b_state{}; // Reset state
    
    blake2b_param params;
    setup_blake2b_params(&params, 192, 7);  // Use explicit N=192, K=7 values
    blake2b_init_param(&base_state, &params);
    
    // Add header to Blake2b state
    blake2b_update(&base_state, header, header_len);
    
    // Add nonce to Blake2b state  
    blake2b_update(&base_state, nonce, nonce_len);
    
    std::cout << "Blake2bHasher: Set header (" << header_len 
              << " bytes) + nonce (" << nonce_len << " bytes)" << std::endl;
    
    return true;
}

bool Blake2bHasher::generate_hash(uint32_t index, uint8_t* output) {
    if (!is_initialized) {
        return false;
    }
    
    // Create a copy of the base state
    blake2b_state hash_state = base_state;
    
    // Append the index as little-endian 4-byte value
    uint8_t index_bytes[4];
    write_index_to_buffer(index, index_bytes);
    
    blake2b_update(&hash_state, index_bytes, 4);
    blake2b_final(&hash_state, output, HASH_OUTPUT_BYTES);
    
    hashes_generated++;
    return true;
}

void Blake2bHasher::write_index_to_buffer(uint32_t index, uint8_t* buffer) {
    // Write index in little-endian format
    buffer[0] = index & 0xff;
    buffer[1] = (index >> 8) & 0xff;
    buffer[2] = (index >> 16) & 0xff;
    buffer[3] = (index >> 24) & 0xff;
}

size_t Blake2bHasher::generate_initial_hashes(MemoryPool* pool, size_t target_count) {
    if (!pool || !is_initialized) {
        return 0;
    }
    
    // Limit to available space in initial_hashes buffer
    size_t max_hashes = std::min(target_count, INITIAL_HASHES);
    
    std::cout << "Blake2bHasher: Generating " << max_hashes << " initial hashes..." << std::endl;
    
    size_t generated = 0;
    for (uint32_t i = 0; i < max_hashes; i++) {
        if (generate_hash(i, pool->initial_hashes.data[i])) {
            generated++;
        } else {
            std::cerr << "Blake2bHasher: Failed to generate hash " << i << std::endl;
            break;
        }
        
        // Progress reporting for large batches
        if (generated % 10000 == 0 && generated > 0) {
            std::cout << "  Generated " << generated << " hashes..." << std::endl;
        }
    }
    
    pool->initial_hashes.count = generated;
    std::cout << "Blake2bHasher: Successfully generated " << generated << " hashes" << std::endl;
    
    return generated;
}

// SIMD batch generation stubs (to be optimized later)
size_t Blake2bHasher::generate_batch_sse2(MemoryPool* pool, uint32_t start_index, size_t count) {
    std::cout << "Blake2bHasher: SSE2 batch generation (" << count << " hashes) - using scalar fallback" << std::endl;
    
    size_t generated = 0;
    for (uint32_t i = 0; i < count && (start_index + i) < INITIAL_HASHES; i++) {
        if (generate_hash(start_index + i, pool->initial_hashes.data[start_index + i])) {
            generated++;
        }
    }
    return generated;
}

size_t Blake2bHasher::generate_batch_avx2(MemoryPool* pool, uint32_t start_index, size_t count) {
    std::cout << "Blake2bHasher: AVX2 batch generation (" << count << " hashes) - using scalar fallback" << std::endl;
    
    size_t generated = 0;
    for (uint32_t i = 0; i < count && (start_index + i) < INITIAL_HASHES; i++) {
        if (generate_hash(start_index + i, pool->initial_hashes.data[start_index + i])) {
            generated++;
        }
    }
    return generated;
}

size_t Blake2bHasher::generate_batch_avx512(MemoryPool* pool, uint32_t start_index, size_t count) {
    std::cout << "Blake2bHasher: AVX512 batch generation (" << count << " hashes) - using scalar fallback" << std::endl;
    
    size_t generated = 0;
    for (uint32_t i = 0; i < count && (start_index + i) < INITIAL_HASHES; i++) {
        if (generate_hash(start_index + i, pool->initial_hashes.data[start_index + i])) {
            generated++;
        }
    }
    return generated;
}

// Blake2bManager implementation
bool Blake2bManager::initialize_for_solve(const uint8_t* header, size_t header_len,
                                          const uint8_t* nonce, size_t nonce_len) {
    // Initialize hasher with Equihash 192,7 parameters
    if (!hasher.initialize(192, 7)) {  // Use explicit N=192, K=7 values
        return false;
    }
    
    // Set the header and nonce for this solve session
    return hasher.set_header_nonce(header, header_len, nonce, nonce_len);
}

size_t Blake2bManager::generate_hashes(MemoryPool* pool, size_t target_count) {
    if (!hasher.is_ready()) {
        std::cerr << "Blake2bManager: Hasher not ready!" << std::endl;
        return 0;
    }
    
    std::cout << "Blake2bManager: Generating hashes using " 
              << g_simd_dispatcher.get_active_name() << " SIMD" << std::endl;
    
    // For now, use the basic batch generation
    // Later this will be optimized with proper SIMD batching
    size_t generated = dispatch_hash_generation(pool, 0, target_count);
    
    std::cout << "Blake2bManager: Generated " << generated << " / " << target_count 
              << " requested hashes" << std::endl;
    
    return generated;
}

size_t Blake2bManager::dispatch_hash_generation(MemoryPool* pool, uint32_t start_index, size_t count) {
    // Dispatch to appropriate SIMD implementation
    switch (active_simd) {
        case SIMDLevel::AVX512:
            return hasher.generate_batch_avx512(pool, start_index, count);
        case SIMDLevel::AVX2:
            return hasher.generate_batch_avx2(pool, start_index, count);
        case SIMDLevel::SSE2:
            return hasher.generate_batch_sse2(pool, start_index, count);
        default:
            // Fallback to basic generation
            return hasher.generate_initial_hashes(pool, count);
    }
}

std::string Blake2bManager::get_performance_info() const {
    std::ostringstream oss;
    oss << "Blake2b Performance: ";
    oss << get_total_hashes() << " hashes generated, ";
    oss << "SIMD: " << g_simd_dispatcher.get_active_name();
    return oss.str();
}

} // namespace Solver1927