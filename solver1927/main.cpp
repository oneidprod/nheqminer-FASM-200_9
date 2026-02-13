#include "solver1927.hpp"
#include <iostream>
#include <vector>
#include <functional>
#include <cstring>

int main() {
    std::cout << "🔧 Starting Equihash 192,7 Solver1927 Test..." << std::endl;
    
    solver1927 solver;
    solver.start();
    
    // Dummy test data
    const char* test_header = "test_block_header_data_192_7";
    const char* test_nonce = "nonce123";
    
    // Solution callback function
    auto solution_callback = [](const std::vector<uint32_t>& solution, size_t nonce_offset, const unsigned char* nonce) {
        std::cout << "🎯 Solution found! Indices: ";
        for (size_t i = 0; i < solution.size(); i++) {
            std::cout << solution[i] << " ";
        }
        std::cout << " Nonce offset: " << nonce_offset << std::endl;
    };
    
    // Cancel function (never cancel for testing)
    auto cancel_callback = []() { return false; };
    
    // Hash done callback
    auto hash_done_callback = []() { 
        std::cout << "📈 Hash generation completed." << std::endl; 
    };
    
    solver.solve(test_header, strlen(test_header), 
                 test_nonce, strlen(test_nonce), 
                 cancel_callback, solution_callback, hash_done_callback);
    
    solver.stop();
    std::cout << "✅ Test completed." << std::endl;
    return 0;
}