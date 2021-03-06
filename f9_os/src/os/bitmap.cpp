/*
 * bitmap.cpp
 *
 *  Created on: Apr 22, 2017
 *      Author: warlo
 */

#include <os/bitmap.hpp>

namespace bitops {

uint32_t test_and_set_word(uint32_t *word)
{
	register int result = 1;

	__asm__ __volatile__(
	    "mov r1, #1\n"
	    "mov r2, %[word]\n"
	    "ldrex r0, [r2]\n"	/* Load value [r2] */
	    "cmp r0, #0\n"	/* Checking is word set to 1 */

	    "itt eq\n"
	    "strexeq r0, r1, [r2]\n"
	    "moveq %[result], r0\n"
	    : [result] "=r"(result)
	    : [word] "r"(word)
	    : "r0", "r1", "r2");

	return result == 0;
}

uint32_t test_and_set_bit(uint32_t *word, int bitmask)
{
	register int result = 1;

	__asm__ __volatile__(
	    "mov r2, %[word]\n"
	    "ldrex r0, [r2]\n"		/* Load value [r2] */
	    "tst r0, %[bitmask]\n"	/* Compare value with bitmask */

	    "ittt eq\n"
	    "orreq r1, r0, %[bitmask]\n"	/* Set bit: r1 = r0 | bitmask */
	    "strexeq r0, r1, [r2]\n"		/* Write value back to [r2] */
	    "moveq %[result], r0\n"
	    : [result] "=r"(result)
	    : [word] "r"(word), [bitmask] "r"(bitmask)
	    : "r0", "r1", "r2");

	return result == 0;
}
//};
};

