#include <Elementary.h>

/** @file perf.h
 * @brief Definitions and function prototypes for the performance testing framework.
 */

#define NUM_MANY 1024 /**< A large number of items, typically for stress testing. */
#define NUM        64 /**< A moderate number of items. */
#define NUM_FEW     8 /**< A small number of items. */

/**
 * @brief Seeds the pseudo-random number generator.
 * This function should be called once before using rnd() to ensure
 * different sequences of random numbers on different program runs (if desired,
 * typically by seeding with time).
 */
void         srnd(void);

/**
 * @brief Generates a pseudo-random unsigned integer.
 * @return A pseudo-random unsigned integer.
 */
unsigned int rnd(void);

/**
 * @brief Adds an Evas_Object to a list for later cleanup.
 * Objects added to this list will be deleted when a test finishes.
 * @param o The Evas_Object to add to the cleanup list.
 */
void         cleanup_add(Evas_Object *o);

/**
 * @brief Macro to generate a test function name.
 * Used internally to construct names like `test_mytest_init` or `test_mytest_tick`.
 * @param x The base name of the test.
 * @param y The suffix for the function type (e.g., init, tick).
 */
#define TST(x, y) \
   void test_ ## x ## _ ## y

/**
 * @brief Macro to generate prototypes for a test's init and tick functions.
 * @param x The base name of the test. This will be used to generate
 *          `test_x_init(Evas *e)` and
 *          `test_x_tick(Evas *e, double f, Evas_Coord win_w, Evas_Coord win_h)`.
 */
#define TPROT(x) \
   TST(x, init)(Evas *e); \
   TST(x, tick)(Evas *e, double f, Evas_Coord win_w, Evas_Coord win_h)
