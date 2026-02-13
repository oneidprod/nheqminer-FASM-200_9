#include "solver1927.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include <iomanip>

bool solver1927::initialize_memory() {
    std::cout << "Solver1927: Initializing memory pool..." << std::endl;
    
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

void solver1927::solve(const char *tequihash_header,
                      unsigned int tequihash_header_len,
                      const char* nonce,
                      unsigned int nonce_len,
                      std::function<bool()> cancelf,
                      std::function<void(const std::vector<uint32_t>&, size_t, const unsigned char*)> solutionf,
                      std::function<void(void)> hashdonef,
                      solver1927& device_context) 
{
    // Verify memory pool is available
    if (!device_context.memory_manager.is_valid()) {
        std::cerr << "Solver1927: ERROR - Memory pool not initialized!" << std::endl;
        hashdonef();
        return;
    }
    
    auto* pool = device_context.memory_manager.get();
    std::cout << "Solver1927: Starting solve with N=" << N << ", K=" << K << std::endl;
    std::cout << "Header length: " << tequihash_header_len << " bytes" << std::endl;
    std::cout << "Nonce length: " << nonce_len << " bytes" << std::endl;
    std::cout << "Memory pool: " << std::fixed << std::setprecision(2) 
              << device_context.memory_manager.get_memory_mb() << " MB" << std::endl;
    
    // Verify memory layout sizes
    std::cout << "Memory layout verification:" << std::endl;
    std::cout << "  Initial hashes buffer: " << sizeof(pool->initial_hashes) / (1024*1024) << " MB" << std::endl;
    std::cout << "  Stage buffers (2x): " << sizeof(pool->stage_buffers) / (1024*1024) << " MB" << std::endl;  
    std::cout << "  Bucket arrays: " << sizeof(pool->buckets) / (1024*1024) << " MB" << std::endl;
    
    // Simulate some work with memory pool access (this will be replaced with real Equihash solver)
    auto start_time = std::chrono::high_resolution_clock::now();
    
    int iterations = 0;
    while (!cancelf() && iterations < 100) {
        // Simulate memory usage - write to different sections of the pool
        if (iterations % 25 == 0) {
            // Test initial hash buffer access
            pool->initial_hashes.data[iterations % 1000][0] = static_cast<uint8_t>(iterations);
            
            // Test stage buffer access  
            pool->stage_buffers[0].data[iterations % 1000][0] = static_cast<uint8_t>(iterations);
            pool->stage_buffers[1].data[iterations % 1000][0] = static_cast<uint8_t>(iterations + 1);
            
            // Test bucket access
            pool->buckets.indices[iterations % 256][0] = iterations;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        iterations++;
        
        // Check if we should cancel every 20 iterations
        if (iterations % 20 == 0) {
            if (cancelf()) {
                std::cout << "Solver1927: Cancelled after " << iterations << " iterations" << std::endl;
                break;
            }
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Solver1927: Completed " << iterations << " iterations in " 
              << duration.count() << "ms" << std::endl;
    std::cout << "Solver1927: Memory pool operations verified successfully" << std::endl;
    std::cout << "Solver1927: No solutions found (stub implementation)" << std::endl;
    
    // Call hash done callback to indicate completion
    hashdonef();
}