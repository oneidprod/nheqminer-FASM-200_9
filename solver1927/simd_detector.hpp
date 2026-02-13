#pragma once

#include <cstdint>
#include <string>

#ifdef _WIN32
#include <intrin.h>
#else
#include <cpuid.h>
#endif

namespace Solver1927 {

/**
 * SIMD capability levels supported by Solver1927
 */
enum class SIMDLevel {
    NONE = 0,     // No SIMD support
    SSE2 = 1,     // 128-bit SIMD (baseline)
    AVX2 = 2,     // 256-bit SIMD (primary target)
    AVX512 = 3    // 512-bit SIMD (maximum performance)
};

/**
 * CPU feature detection and SIMD capability reporting
 */
class SIMDDetector {
private:
    SIMDLevel detected_level;
    bool features_checked;
    
    // Feature flags
    bool has_sse2;
    bool has_avx;
    bool has_avx2;
    bool has_avx512_f;
    bool has_avx512_vl;
    bool has_avx512_bw;
    
    void detect_cpu_features();
    void check_cpuid();
    
public:
    SIMDDetector() : detected_level(SIMDLevel::NONE), features_checked(false),
                     has_sse2(false), has_avx(false), has_avx2(false),
                     has_avx512_f(false), has_avx512_vl(false), has_avx512_bw(false) {
        detect_cpu_features();
    }
    
    // Primary interface
    SIMDLevel get_best_level() const { return detected_level; }
    bool supports_level(SIMDLevel level) const;
    std::string get_feature_string() const;
    std::string get_level_name(SIMDLevel level) const;
    
    // Individual feature queries
    bool supports_sse2() const { return has_sse2; }
    bool supports_avx() const { return has_avx; }
    bool supports_avx2() const { return has_avx2; }
    bool supports_avx512() const { return has_avx512_f && has_avx512_vl && has_avx512_bw; }
    
    // Performance estimation
    int get_simd_width_bits() const;
    int get_parallel_hash_count() const;
};

/**
 * SIMD function dispatcher - selects optimal implementations at runtime
 */
class SIMDDispatcher {
private:
    SIMDLevel active_level;
    SIMDDetector detector;
    
public:
    SIMDDispatcher() : active_level(SIMDLevel::NONE) {
        active_level = detector.get_best_level();
    }
    
    // Current configuration
    SIMDLevel get_active_level() const { return active_level; }
    std::string get_active_name() const { return detector.get_level_name(active_level); }
    
    // Force specific SIMD level (for testing/debugging)
    bool force_level(SIMDLevel level);
    
    // Core algorithm dispatching (stubs for now, will be implemented)
    void blake2b_parallel_hash(const uint8_t* input, uint8_t* output, size_t count);
    void xor_collision_check_256(const uint8_t* data_a, const uint8_t* data_b, uint8_t* result);
    void xor_collision_check_512(const uint8_t* data_a, const uint8_t* data_b, uint8_t* result);
    
private:
    // Implementation stubs (to be filled in Blake2b integration step)
    void blake2b_sse2(const uint8_t* input, uint8_t* output, size_t count);
    void blake2b_avx2(const uint8_t* input, uint8_t* output, size_t count);
    void blake2b_avx512(const uint8_t* input, uint8_t* output, size_t count);
};

// Global SIMD detector instance
extern SIMDDetector g_simd_detector;
extern SIMDDispatcher g_simd_dispatcher;

} // namespace Solver1927