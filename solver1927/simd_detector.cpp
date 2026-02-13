#include "simd_detector.hpp"
#include <iostream>
#include <sstream>

namespace Solver1927 {

// Global instances
SIMDDetector g_simd_detector;
SIMDDispatcher g_simd_dispatcher;

void SIMDDetector::check_cpuid() {
#ifdef _WIN32
    int cpu_info[4];
    
    // Check for basic CPUID support
    __cpuid(cpu_info, 0);
    if (cpu_info[0] < 1) return;
    
    // Get feature flags from CPUID(1)
    __cpuid(cpu_info, 1);
    has_sse2 = (cpu_info[3] & (1 << 26)) != 0;  // EDX bit 26
    has_avx = (cpu_info[2] & (1 << 28)) != 0;   // ECX bit 28
    
    // Check extended features with CPUID(7)
    if (cpu_info[0] >= 7) {
        __cpuidex(cpu_info, 7, 0);
        has_avx2 = (cpu_info[1] & (1 << 5)) != 0;      // EBX bit 5
        has_avx512_f = (cpu_info[1] & (1 << 16)) != 0;  // EBX bit 16  
        has_avx512_vl = (cpu_info[1] & (1 << 31)) != 0; // EBX bit 31
        has_avx512_bw = (cpu_info[1] & (1 << 30)) != 0; // EBX bit 30
    }
    
#else
    unsigned int eax, ebx, ecx, edx;
    
    // Check for CPUID support
    if (__get_cpuid_max(0, nullptr) < 1) return;
    
    // Get feature flags from CPUID(1)
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        has_sse2 = (edx & (1U << 26)) != 0;  // EDX bit 26
        has_avx = (ecx & (1U << 28)) != 0;   // ECX bit 28
    }
    
    // Check extended features with CPUID(7)  
    if (__get_cpuid_max(0, nullptr) >= 7) {
        if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
            has_avx2 = (ebx & (1U << 5)) != 0;      // EBX bit 5
            has_avx512_f = (ebx & (1U << 16)) != 0;  // EBX bit 16
            has_avx512_vl = (ebx & (1U << 31)) != 0; // EBX bit 31 
            has_avx512_bw = (ebx & (1U << 30)) != 0; // EBX bit 30
        }
    }
#endif
}

void SIMDDetector::detect_cpu_features() {
    if (features_checked) return;
    
    // Initialize all features to false
    has_sse2 = has_avx = has_avx2 = false;
    has_avx512_f = has_avx512_vl = has_avx512_bw = false;
    
    // Run CPUID detection
    check_cpuid();
    
    // Determine best SIMD level based on available features
    if (supports_avx512()) {
        detected_level = SIMDLevel::AVX512;
    } else if (has_avx2) {
        detected_level = SIMDLevel::AVX2;
    } else if (has_sse2) {
        detected_level = SIMDLevel::SSE2;
    } else {
        detected_level = SIMDLevel::NONE;
    }
    
    features_checked = true;
}

bool SIMDDetector::supports_level(SIMDLevel level) const {
    switch (level) {
        case SIMDLevel::NONE:   return true;
        case SIMDLevel::SSE2:   return has_sse2;
        case SIMDLevel::AVX2:   return has_avx && has_avx2;
        case SIMDLevel::AVX512: return supports_avx512();
        default: return false;
    }
}

std::string SIMDDetector::get_feature_string() const {
    std::ostringstream oss;
    oss << "CPU Features: ";
    if (has_sse2) oss << "SSE2 ";
    if (has_avx) oss << "AVX ";
    if (has_avx2) oss << "AVX2 ";
    if (has_avx512_f) oss << "AVX512F ";
    if (has_avx512_vl) oss << "AVX512VL ";
    if (has_avx512_bw) oss << "AVX512BW ";
    
    std::string result = oss.str();
    if (result.empty() || result == "CPU Features: ") {
        return "CPU Features: None detected";
    }
    return result;
}

std::string SIMDDetector::get_level_name(SIMDLevel level) const {
    switch (level) {
        case SIMDLevel::NONE:   return "None";
        case SIMDLevel::SSE2:   return "SSE2";
        case SIMDLevel::AVX2:   return "AVX2"; 
        case SIMDLevel::AVX512: return "AVX512";
        default: return "Unknown";
    }
}

int SIMDDetector::get_simd_width_bits() const {
    switch (detected_level) {
        case SIMDLevel::SSE2:   return 128;
        case SIMDLevel::AVX2:   return 256;
        case SIMDLevel::AVX512: return 512;
        default: return 0;
    }
}

int SIMDDetector::get_parallel_hash_count() const {
    // Number of 32-byte hashes we can process in parallel
    switch (detected_level) {
        case SIMDLevel::SSE2:   return 4;   // 128 bits / 32 bits per hash = 4
        case SIMDLevel::AVX2:   return 8;   // 256 bits / 32 bits per hash = 8  
        case SIMDLevel::AVX512: return 16;  // 512 bits / 32 bits per hash = 16
        default: return 1;
    }
}

// SIMDDispatcher implementation
bool SIMDDispatcher::force_level(SIMDLevel level) {
    if (!detector.supports_level(level)) {
        std::cerr << "Warning: Requested SIMD level " << detector.get_level_name(level) 
                  << " not supported by CPU" << std::endl;
        return false;
    }
    
    active_level = level;
    std::cout << "SIMD level forced to: " << get_active_name() << std::endl;
    return true;
}

// SIMD function stubs (to be implemented in Blake2b integration)
void SIMDDispatcher::blake2b_parallel_hash(const uint8_t* input, uint8_t* output, size_t count) {
    switch (active_level) {
        case SIMDLevel::AVX512:
            blake2b_avx512(input, output, count);
            break;
        case SIMDLevel::AVX2:
            blake2b_avx2(input, output, count);
            break;
        case SIMDLevel::SSE2:
            blake2b_sse2(input, output, count);
            break;
        default:
            // Fallback to scalar implementation
            std::cout << "Warning: Using scalar fallback for Blake2b" << std::endl;
            break;
    }
}

void SIMDDispatcher::xor_collision_check_256(const uint8_t* data_a, const uint8_t* data_b, uint8_t* result) {
    // Placeholder - will implement XOR-based collision detection
    std::cout << "XOR collision check (256-bit) - stub" << std::endl;
}

void SIMDDispatcher::xor_collision_check_512(const uint8_t* data_a, const uint8_t* data_b, uint8_t* result) {
    // Placeholder - will implement XOR-based collision detection  
    std::cout << "XOR collision check (512-bit) - stub" << std::endl;
}

// Implementation stubs for different SIMD levels
void SIMDDispatcher::blake2b_sse2(const uint8_t* input, uint8_t* output, size_t count) {
    std::cout << "Blake2b SSE2 implementation - stub (" << count << " hashes)" << std::endl;
}

void SIMDDispatcher::blake2b_avx2(const uint8_t* input, uint8_t* output, size_t count) {
    std::cout << "Blake2b AVX2 implementation - stub (" << count << " hashes)" << std::endl;
}

void SIMDDispatcher::blake2b_avx512(const uint8_t* input, uint8_t* output, size_t count) {
    std::cout << "Blake2b AVX512 implementation - stub (" << count << " hashes)" << std::endl;
}

} // namespace Solver1927