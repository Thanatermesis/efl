#include "evas_common_private.h"

/**
 * @brief Bitmask storing the detected and enabled CPU features.
 *
 * This variable is initialized by evas_common_cpu_init() and is used
 * by evas_common_cpu_has_feature() to check for specific CPU capabilities.
 * Each bit corresponds to a CPU_FEATURE_* constant.
 */
static int cpu_feature_mask = 0;

/**
 * @brief Checks if a specific CPU feature is present.
 * @param f The CPU feature to check, from Eina_Cpu_Features enum.
 * @return EINA_TRUE if the feature is present, EINA_FALSE otherwise.
 *
 * This function internally calls eina_cpu_features_get() to retrieve
 * the available CPU features and then checks if the requested feature 'f'
 * is part of the retrieved set.
 */
static Eina_Bool
_cpu_check(Eina_Cpu_Features f)
{
   Eina_Cpu_Features features = eina_cpu_features_get();
   return (features & f) == f;
}

/**
 * @brief Initializes the CPU feature detection.
 *
 * This function detects available CPU features like MMX, SSE, SSE2, SSE3,
 * AltiVec, and NEON. It checks for environment variables (e.g., EVAS_CPU_NO_MMX)
 * that can disable specific features. This function should be called once
 * at application startup.
 *
 * The detected and enabled features are stored in the global
 * cpu_feature_mask.
 */
EVAS_API void
evas_common_cpu_init(void)
{
   static int called = 0;

   if (called) return;
   called = 1;
#ifdef BUILD_MMX
   if (getenv("EVAS_CPU_NO_MMX"))
     cpu_feature_mask &= ~CPU_FEATURE_MMX;
   else
     cpu_feature_mask |= _cpu_check(EINA_CPU_MMX) * CPU_FEATURE_MMX;
   if (getenv("EVAS_CPU_NO_MMX2"))
     cpu_feature_mask &= ~CPU_FEATURE_MMX2;
   else /* It seems "MMX2" is actually part of SSE (and 3DNow)? */
     cpu_feature_mask |= _cpu_check(EINA_CPU_SSE) * CPU_FEATURE_MMX2;
   if (getenv("EVAS_CPU_NO_SSE"))
     cpu_feature_mask &= ~CPU_FEATURE_SSE;
   else
     cpu_feature_mask |= _cpu_check(EINA_CPU_SSE) * CPU_FEATURE_SSE;
# ifdef BUILD_SSE3
   if (getenv("EVAS_CPU_NO_SSE3"))
     cpu_feature_mask &= ~CPU_FEATURE_SSE3;
   else
     cpu_feature_mask |= _cpu_check(EINA_CPU_SSE3) * CPU_FEATURE_SSE3;
# endif /* BUILD_SSE3 */
#endif /* BUILD_MMX */

#ifdef BUILD_ALTIVEC
   if (getenv("EVAS_CPU_NO_ALTIVEC"))
     cpu_feature_mask &= ~CPU_FEATURE_ALTIVEC;
   else
     cpu_feature_mask |= _cpu_check(CPU_FEATURE_ALTIVEC) * CPU_FEATURE_ALTIVEC;
#endif /* BUILD_ALTIVEC */

#if defined(__ARM_ARCH__)
# ifdef BUILD_NEON
   if (getenv("EVAS_CPU_NO_NEON"))
     cpu_feature_mask &= ~CPU_FEATURE_NEON;
   else
     cpu_feature_mask |= _cpu_check(EINA_CPU_NEON) * CPU_FEATURE_NEON;
# endif
#endif

#if defined(__aarch64__)
# ifdef BUILD_NEON
   if (getenv("EVAS_CPU_NO_NEON"))
     cpu_feature_mask &= ~CPU_FEATURE_NEON;
   else
     cpu_feature_mask |= CPU_FEATURE_NEON;
# endif
   if (getenv("EVAS_CPU_NO_SVE"))
     cpu_feature_mask &= ~CPU_FEATURE_SVE;
   else
     cpu_feature_mask |= _cpu_check(EINA_CPU_SVE) * CPU_FEATURE_SVE;
#endif
}

/**
 * @brief Checks if a specific CPU feature is enabled.
 * @param feature The CPU feature to check (e.g., CPU_FEATURE_MMX).
 * @return Non-zero if the feature is enabled, 0 otherwise.
 *
 * This function checks against the globally initialized cpu_feature_mask.
 */
int
evas_common_cpu_has_feature(unsigned int feature)
{
   return (cpu_feature_mask & feature);
}

/**
 * @brief Checks if the CPUID instruction is available.
 * @return Always returns 0 (CPUID instruction availability is not explicitly checked here).
 * @note This function currently always returns 0. The actual CPU feature
 *       detection relies on eina_cpu_features_get().
 */
int
evas_common_cpu_have_cpuid(void)
{
   return 0;
}

/**
 * @brief Reports which major CPU instruction sets (MMX, SSE, SSE2) are available.
 * @param[out] mmx Pointer to an integer that will be set to 1 if MMX is available, 0 otherwise.
 * @param[out] sse Pointer to an integer that will be set to 1 if SSE or MMX2 (SSE part) is available, 0 otherwise.
 * @param[out] sse2 Pointer to an integer that will be set to 1 if SSE2 is available, 0 otherwise. (Currently always 0 as MMX2 is mapped to sse)
 *
 * This function checks the cpu_feature_mask for MMX, MMX2 (treated as part of SSE),
 * and SSE features. The results are cached on the first call.
 */
EVAS_API void
evas_common_cpu_can_do(int *mmx, int *sse, int *sse2)
{
   static int do_mmx = 0, do_sse = 0, do_sse2 = 0, done = 0;

   if (!done)
     {
        if (cpu_feature_mask & CPU_FEATURE_MMX) do_mmx = 1;
        if (cpu_feature_mask & CPU_FEATURE_MMX2) do_sse = 1;
        if (cpu_feature_mask & CPU_FEATURE_SSE) do_sse = 1;
        done = 1;
     }

   *mmx = do_mmx;
   *sse = do_sse;
   *sse2 = do_sse2;
}

#ifdef BUILD_MMX
/**
 * @brief Finalizes MMX/MMX2 operations.
 *
 * If MMX or MMX2 features are enabled and used, this function calls
 * the `emms` instruction (Empty MMX Technology State) to clear the MMX state.
 * This is important to avoid conflicts with floating-point operations on x86.
 * This function should be called after a block of MMX/MMX2 optimized code.
 */
EVAS_API void
evas_common_cpu_end_opt(void)
{
   if (cpu_feature_mask & (CPU_FEATURE_MMX | CPU_FEATURE_MMX2))
     {
        emms();
     }
}
#else
/**
 * @brief Finalizes MMX/MMX2 operations (stub for non-MMX builds).
 *
 * This is a stub function for builds where MMX is not enabled.
 * It does nothing.
 */
EVAS_API void
evas_common_cpu_end_opt(void)
{
}
#endif
