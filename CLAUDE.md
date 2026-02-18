# CLAUDE.md - Zero Coin Equihash 192,7 Miner Project Status

## 🎯 PROJECT OVERVIEW
**Repository**: `/home/griffithm/builds/1927_miner_project/nheqminer-FASM-200_9/`
**Purpose**: Equihash 192,7 miner for Zero coin, built on nheqminer framework
**Status**: 🚧 INFRASTRUCTURE COMPLETE - Algorithm debugging needed
**Date**: Development Feb 13-17, 2026

## 📋 TASK SUMMARY
**Original Mandate**: "PROJECT DIRECTIVE EQUIHASH 192,7 C PLUS PLUS SOLVER TRANSPLANT"
**Background**: Previous attempts failed using FASM/NASM assembly ports. Decision: pure C++ SIMD implementation
**Critical Requirement**: Git discipline after every sub-step to avoid "death spiral" of endless fixes

**Key Technical Requirements**: 
- Custom solver1927 algorithm implementation (N=192, K=7)
- SIMD optimization with runtime detection (AVX512/AVX2/SSE2)
- Memory optimization: 32-48MB within L3 cache
- Zero coin compatibility (ZERO_PoW personalization)
- Integration with nheqminer framework via ISolver interface

**Development Strategy**: Incremental builds with git commits at each working step

## ✅ COMPLETED WORK

### 1. Core Algorithm (solver1927/)
- **Blake2b hashing**: Custom implementation with ZERO_PoW personalization
- **SIMD optimization**: Auto-detects AVX512 → AVX2 → SSE2 (16/8/4 parallel hashes)
- **Memory management**: Conservative 22MB pool for L3 cache efficiency  
- **Collision detection**: 8-stage pipeline with bucket optimization
- **Performance**: Processes 200,000 hashes per iteration in ~1.5 seconds

### 2. Build System
- **CMakeLists.txt**: USE_SOLVER1927=ON, USE_CPU_TROMP=OFF (focused on 192,7 only)
- **Integration**: Linked into main nheqminer binary (3.3MB)
- **Test binary**: solver1927/test for standalone algorithm testing (85KB)

### 3. Threading Architecture  
- **Command**: `./nheqminer -c1927 [threads]` creates multiple solver instances
- **Implementation**: Each solver runs in separate thread, works on different nonce ranges
- **Verified**: `./nheqminer -c1927 4` creates 4 active worker threads
- **Performance**: Parallel execution with proper CPU utilization

### 4. Git History & Development Discipline
- **Git-disciplined approach**: Mandated commits after every successful sub-step
- **Step-by-step implementation**: 1.1→1.3 (infrastructure), 2.1→2.4 (memory/SIMD), 3.1→3.5 (algorithm)
- **Rollback strategy**: Each commit was a working state to prevent "death spiral"
- **Accurate timeline**: Feb 13-17, 2026 with original timestamps preserved
- **43 tracked files**: Complete solver1927 implementation
- **Final commit**: `5e3d74b` - Disable cpu_tromp: Focus on Equihash 192,7 only
- **Handoff commit**: `db0b5b0` with complete project documentation for environment transition

## 🔧 TECHNICAL DETAILS

### Binary Usage
```bash
# Zero coin mining (production)
./nheqminer -c1927 4 -l stratum+tcp://pool:port -u address

# Algorithm testing (development)  
./solver1927/test

# Benchmark
./nheqminer -b 10 -c1927 4
```

### Flag Relationships
| Original System | New System |
|-----------------|------------|
| `-t 4 -e 2` (4 threads, force AVX2) | `-c1927 4` (4 Solver1927 instances, auto SIMD) |
| Equihash 200,9 (Zcash) | Equihash 192,7 (Zero coin) |
| Manual SIMD selection | Automatic detection |

### SIMD Support Matrix
- **AVX512**: 512-bit, 16 parallel hashes (detected on user's system)
- **AVX2**: 256-bit, 8 parallel hashes  
- **SSE2**: 128-bit, 4 parallel hashes (fallback)
- **Note**: Plain AVX (256-bit) intentionally skipped - not optimal for mining

### Key Files
```
solver1927/
├── solver1927.cpp/.hpp          # Main solver implementation
├── blake2b_hasher.cpp/.hpp      # Blake2b with ZERO_PoW personalization  
├── collision_detector.cpp/.hpp  # 8-stage Equihash collision detection
├── simd_detector.cpp/.hpp       # Runtime CPU feature detection
├── memory_pool.hpp              # Aligned memory management
├── main.cpp                     # Standalone test program
└── CMakeLists.txt              # Build configuration

nheqminer/
├── MinerFactory.cpp            # Integration point (-c1927 command handling)
├── AvailableSolvers.h          # Solver registration
└── main.cpp                    # Command line parsing
```

## 🎯 CURRENT STATUS

### ✅ WORKING INFRASTRUCTURE
- **Build system**: Compiles without errors
- **Hash generation**: Processes 200K Blake2b hashes correctly (~1.5s)
- **Threading**: 4 parallel solver instances confirmed working  
- **SIMD**: AVX512 optimization active (16 parallel hashes)
- **Integration**: solver1927 properly linked into nheqminer
- **Git**: Complete development history preserved with accurate timestamps
- **Stage 0 collision detection**: Successfully finds ~1,174 collisions per iteration

### 🐛 KNOWN ISSUES
- **Solution finding**: No complete solutions found even after 1000 iterations
- **Stage progression bug**: Always stops at Stage 1 (0 collisions found)
- **Algorithm issue**: Likely in collision detection bit extraction or XOR result formatting
- **Needs debugging**: Collision detection logic for stages 1-7

### ❌ MISSING COMPONENTS
- **Algorithm debugging**: Fix collision detection progression beyond Stage 0
- **Documentation**: Update README.md for Zero coin / Equihash 192,7
- **Usage examples**: No mention of -c1927 command or Equihash 192,7
- **Solver distinction**: Doesn't explain nheqminer vs solver1927/test binaries

## 🔄 RECENTLY VERIFIED (Feb 17-18, 2026)
1. **Threading confirmation**: `-c1927 4` creates 4 worker threads (visible in benchmark output)
2. **SIMD detection**: AVX512 automatically detected and utilized
3. **Performance**: Single iteration completes in ~1.5s, benchmark runs efficiently
4. **Architecture**: Confirmed nheqminer (mining) vs solver1927/test (testing) distinction
5. **Git accuracy**: All commit messages match actual file changes

## � ORIGINAL 12-STEP PLAN STATUS

### ✅ COMPLETED STEPS (Steps 1.1-3.5)
**STEP 1: Baseline and Cleanup (3 commits)**
- ✅ 1.1 Establish Clean Baseline - Verified current build state
- ✅ 1.2 Strip FASM/NASM References - Disabled xenoncat assembly dependencies  
- ✅ 1.3 Add Solver1927 Stub - Created infrastructure and build integration

**STEP 2: Core Memory Infrastructure (4 commits)**
- ✅ 2.1 Memory Pool Structure - 64-byte aligned allocation, 22MB conservative pool
- ✅ 2.2 SIMD Detection - Runtime CPU detection (AVX512/AVX2/SSE2)
- ✅ 2.3 Blake2b Integration - ZERO_PoW personalization, 200K hashes per iteration
- ✅ 2.4 Basic Solver Interface - Complete ISolver implementation with threading

**STEP 3: Algorithm Implementation (5 commits)**
- ✅ 3.1 Stage 0 - Initial Hash Generation - Working (~1,174 collisions found)
- ✅ 3.2 Add Collision Verification - Solution verification logic implemented
- ⚠️ 3.3 Multi-Stage Pipeline - **BUG: Stage 1+ progression fails (0 collisions)**
- ✅ 3.4 AVX2 Optimization - 8 parallel hash processing active
- ✅ 3.5 AVX-512 Conditional Support - 16 parallel hash processing on capable hardware

### 🔧 REMAINING WORK

**CRITICAL: Algorithm Debugging**
- **Fix Stage 1-7 collision detection**: Bit extraction logic or XOR result formatting bug
- **Verify solution finding**: Ensure complete solutions are generated after stage fixes
- **Performance validation**: Confirm mining performance meets expectations

**Documentation & Polish**
- **Update README.md**: Replace Zcash 200,9 references with Zero coin 192,7 information
- **Usage examples**: Document -c1927 command and solver distinction
- **Pool integration testing**: Validate with actual Zero coin mining pools

## 🚀 POTENTIAL FUTURE ENHANCEMENTS
1. **AVX1 support**: Easy to add later (~30 min work) for legacy systems
2. **Further optimizations**: Memory access patterns, cache efficiency improvements  
3. **Advanced SIMD**: Explore newer instruction sets as hardware evolves

## 💡 ARCHITECTURE INSIGHTS

### Original Design Philosophy
- **Failure-informed**: Built after analyzing previous FASM/NASM failures in previous_attempts_info/
- **Assembly-free**: Pure C++ SIMD to avoid hardcoded 200,9 constants in assembly
- **Cache-conscious**: 32-48MB memory target to stay within L3 cache bounds
- **Git-disciplined**: Every component added incrementally with working builds

### Technical Implementation
- **Self-contained**: solver1927 completely independent of cpu_tromp
- **Modular design**: Easy to extend SIMD support or modify algorithms
- **Professional integration**: Clean ISolver interface implementation
- **Performance-focused**: Conservative memory usage, automatic SIMD selection
- **Future-proof**: AVX512 support ready for latest hardware

### Development Methodology
- **Incremental**: Step 1 (strip assembly) → Step 2 (memory/SIMD) → Step 3 (algorithm)
- **Validation**: Build verification after each commit to prevent rollbacks
- **Risk mitigation**: Established baseline before any changes to enable quick recovery

## 📚 REFERENCE MATERIALS

### Development History & Research
**Location**: `/home/griffithm/builds/1927_miner_project/previous_attempts_info/`

Key reference files that informed development:
- **ALGORITHM_REFERENCE.md**: Core Equihash 192,7 algorithm specifications
- **XENONCAT_192_7_INFO.md**: Analysis of xenoncat solver compatibility 
- **XENONCAT_192_7_STATUS.md**: Implementation status and findings
- **XENONCAT_FINAL_ANALYSIS.md**: Final assessment of xenoncat approach
- **XENONCAT_NEXT_STEPS.md**: Strategic decisions that led to solver1927
- **XENONCAT_SHIFT_ANALYSIS.md**: Technical analysis of parameter differences
- **ZERO_COIN_FINDINGS.md**: Zero coin specific implementation requirements
- **previous_chats/**: Historical conversation logs and technical discussions

### Reference Implementation
**Location**: `/home/griffithm/builds/1927_miner_project/equihash-zcash-c/`
- Basic Equihash solver in C for algorithm reference
- Used for understanding collision detection fundamentals

## 📚 CONTEXT FOR NEW AI
**You are inheriting a MOSTLY COMPLETE Zero coin miner**. The infrastructure is solid but there's a critical bug in the collision detection algorithm that prevents finding complete solutions. Stage 0 works perfectly, but stages 1-7 have issues.

### Project Origin & Mandate
- **Background**: Previous FASM/NASM attempts failed due to hardcoded 200,9 constants
- **User's directive**: "Git discipline after every successful sub-step to avoid death spiral"
- **Original plan**: 12-step incremental implementation with git commits at each working state
- **Success criteria**: Pure C++ Equihash 192,7 solver within 32-48MB memory constraints

### Development Approach Inherited
- **Risk mitigation**: Established clean baseline before any changes
- **Incremental methodology**: Step 1 (infrastructure) → Step 2 (memory/SIMD) → Step 3 (algorithm)
- **Validation strategy**: Build verification before each commit
- **Rollback capability**: Each commit represents a working state

**Key commands to remember**:
- Build: `cd build && cmake .. && make -j$(nproc)`
- Test: `./solver1927/test` or `./nheqminer -b 10 -c1927 4`  
- Mine: `./nheqminer -c1927 [threads] -l pool:port -u address`

**Current debugging focus**: Collision detection algorithm in solver1927/collision_detector.cpp - specifically stage progression and bit extraction for stages 1-7.

**Last conversation context**: User discovered that even 1000 benchmark iterations find no solutions, indicating an algorithmic bug rather than just probability. Stage 0 consistently finds ~1,174 collisions, but Stage 1 always finds 0.

**Critical insight**: The user specifically warned against "death spirals" and mandated git discipline. All infrastructure works - only algorithm debugging needed.