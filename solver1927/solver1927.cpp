#include "solver1927.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include <iomanip>

bool solver1927::initialize_memory() {
    std::cout << "Solver1927: Initializing memory pool..." << std::endl;
    
    // Report SIMD capabilities first
    report_simd_capabilities();
    
    if (!memory_manager.allocate()) {
        std::cerr << "Solver1927: ERROR - Failed to allocate memory pool!" << std::endl;
        return false;
    }
    
    double memory_mb = memory_manager.get_memory_mb();
    std::cout << "Solver1927: Memory pool initialized - " 
              << std::fixed << std::setprecision(2) << memory_mb << " MB allocated" << std::endl;
    
    // Validate memory is within L3 cache limits (32-48MB target)
    if (memory_mb > 50.0) {
        std::cout << "Solver1927: WARNING - Memory usage (" << memory_mb 
                  << " MB) may exceed L3 cache capacity" << std::endl;
    } else if (memory_mb < 30.0) {
        std::cout << "Solver1927: INFO - Conservative memory usage (" << memory_mb 
                  << " MB) well within L3 cache" << std::endl;
    } else {
        std::cout << "Solver1927: INFO - Optimal memory usage for L3 cache" << std::endl;
    }
    
    return true;
}

void solver1927::cleanup_memory() {
    std::cout << "Solver1927: Cleaning up memory pool..." << std::endl;
    memory_manager.deallocate();
}

void solver1927::report_simd_capabilities() const {
    using namespace Solver1927;
    
    std::cout << "SIMD Detection Results:" << std::endl;
    std::cout << "  " << g_simd_detector.get_feature_string() << std::endl;
    std::cout << "  Best SIMD level: " << g_simd_dispatcher.get_active_name() << std::endl;
    std::cout << "  SIMD width: " << g_simd_detector.get_simd_width_bits() << " bits" << std::endl;
    std::cout << "  Parallel hashes: " << g_simd_detector.get_parallel_hash_count() << std::endl;
}

bool solver1927::test_blake2b_integration(const char* header, unsigned int header_len,
                                         const char* nonce, unsigned int nonce_len) {
    std::cout << "Testing Blake2b integration..." << std::endl;
    
    // Initialize Blake2b for this solve session
    if (!blake2b_manager.initialize_for_solve(reinterpret_cast<const uint8_t*>(header), header_len,
                                              reinterpret_cast<const uint8_t*>(nonce), nonce_len)) {
        return false;
    }
    
    // Generate a small batch of test hashes
    auto* pool = memory_manager.get();
    if (!pool) {
        std::cerr << "Memory pool not available for Blake2b test!" << std::endl;
        return false;
    }
    
    size_t test_hashes = 100;  // Generate 100 test hashes
    size_t generated = blake2b_manager.generate_hashes(pool, test_hashes);
    
    if (generated != test_hashes) {
        std::cerr << "Blake2b test failed: Expected " << test_hashes 
                  << " hashes, got " << generated << std::endl;
        return false;
    }
    
    // Verify first few hashes are different (basic sanity check)
    bool hashes_differ = false;
    for (int i = 1; i < 5 && i < generated; i++) {
        if (memcmp(pool->initial_hashes.data[0], pool->initial_hashes.data[i], 32) != 0) {
            hashes_differ = true;
            break;
        }
    }
    
    if (!hashes_differ) {
        std::cerr << "Blake2b test warning: Generated hashes appear identical" << std::endl;
    }
    
    // Display first hash for verification
    std::cout << "Blake2b test successful - First hash: ";
    for (int i = 0; i < 8; i++) {
        printf("%02x", pool->initial_hashes.data[0][i]);
    }
    std::cout << "..." << std::endl;
    
    return true;
}

void solver1927::solve(const char *tequihash_header,
                      unsigned int tequihash_header_len,
                      const char* nonce,
                      unsigned int nonce_len,
                      std::function<bool()> cancelf,
                      std::function<void(const std::vector<uint32_t>&, size_t, const unsigned char*)> solutionf,
                      std::function<void(void)> hashdonef) 
{
    // Verify memory pool is available
    if (!memory_manager.is_valid()) {
        std::cerr << "Solver1927: ERROR - Memory pool not initialized!" << std::endl;
        hashdonef();
        return;
    }
    
    auto* pool = memory_manager.get();
    std::cout << "Solver1927: Starting solve with N=" << N << ", K=" << K << std::endl;
    std::cout << "Header length: " << tequihash_header_len << " bytes" << std::endl;
    std::cout << "Nonce length: " << nonce_len << " bytes" << std::endl;
    std::cout << "Memory pool: " << std::fixed << std::setprecision(2) 
              << memory_manager.get_memory_mb() << " MB" << std::endl;
    std::cout << "Active SIMD: " << Solver1927::g_simd_dispatcher.get_active_name() << std::endl;
    
    // Test Blake2b integration
    if (!test_blake2b_integration(tequihash_header, tequihash_header_len,
                                  nonce, nonce_len)) {
        std::cerr << "Solver1927: Blake2b integration test failed!" << std::endl;
        hashdonef();
        return;
    }
    
    // Run the actual Equihash collision detection algorithm
    bool found_solutions = run_collision_detection(tequihash_header, tequihash_header_len,
                                                   nonce, nonce_len, solutionf);
    
    if (found_solutions) {
        std::cout << "Solver1927: Solutions found and reported!" << std::endl;
    } else {
        std::cout << "Solver1927: No valid solutions found in this iteration" << std::endl;
    }
    
    // Display collision detection statistics
    std::cout << "Solver1927: " << collision_detector.get_stats_string() << std::endl;
    
    // Call hash done callback to indicate completion
    hashdonef();
}

bool solver1927::run_collision_detection(const char* header, unsigned int header_len,
                                         const char* nonce, unsigned int nonce_len,
                                         std::function<void(const std::vector<uint32_t>&, size_t, const unsigned char*)> solutionf) {
    auto* pool = memory_manager.get();
    
    // Generate initial hashes for collision detection  
    // Scale up significantly to enable deeper stage progression
    // For 24-bit collision space, need ~50K+ hashes for stage 2-3 progression
    size_t hash_count = 800000;  // Increased for Stage 1+ progression - need more Stage 0 collisions
    std::cout << "Solver1927: Generating " << hash_count << " initial hashes..." << std::endl;
    
    // Initialize Blake2b for this solve session
    if (!blake2b_manager.initialize_for_solve(reinterpret_cast<const uint8_t*>(header), header_len,
                                              reinterpret_cast<const uint8_t*>(nonce), nonce_len)) {
        std::cerr << "Failed to initialize Blake2b for collision detection!" << std::endl;
        return false;
    }
    
    // Generate the initial hash set
    size_t generated = blake2b_manager.generate_hashes(pool, hash_count);
    if (generated != hash_count) {
        std::cerr << "Blake2b failed to generate expected hash count: " 
                  << generated << "/" << hash_count << std::endl;
        return false;
    }
    
    std::cout << "Solver1927: Generated " << generated << " hashes, starting collision detection..." << std::endl;
    
    // Run the collision detection algorithm with solution callback
    bool found_solutions = collision_detector.detect_collisions(pool, generated, solutionf);
    
    if (found_solutions) {
        std::cout << "Solver1927: ✅ Solutions found and reported via callback!" << std::endl;
        return true;
    } else {
        std::cout << "Solver1927: No valid solutions found in this iteration" << std::endl;
        return false;
    }
}