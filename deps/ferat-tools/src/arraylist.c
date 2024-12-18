/* Author: Marcel Simader (marcel.simader@jku.at) */
/* Date: 10.06.2023 */
/* (c) Marcel Simader 2023 */

#include "arraylist.h"

#include <stdio.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ Default Functions ~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define _IMPL_AL(ET, AT, methprefix, gt, eq, fmt, fmt_args)               \
                                                                          \
    AT *methprefix##_insert_sorted(AT *al, ET element) {                  \
        assert(al != NULL);                                               \
        uint32_t i = 0;                                                   \
        while ((i < al->size) && gt(element, al->array[i])) i++;          \
        return methprefix##_insert(al, element, i);                       \
    }                                                                     \
                                                                          \
    int64_t methprefix##_index(AT const *al, ET element) {                \
        assert(al != NULL);                                               \
        ET other_element;                                                 \
        for (uint32_t i = 0; i < al->size; ++i) {                         \
            other_element = al->array[i];                                 \
            if (eq(element, other_element)) return i;                     \
        }                                                                 \
        return -1;                                                        \
    }                                                                     \
                                                                          \
    int64_t methprefix##_binary_search_index(AT const *al, ET element) {  \
        assert(al != NULL);                                               \
        if (al->size < 1) return -1;                                      \
        uint32_t mid, low = 0, high = al->size - 1;                       \
        ET other_element;                                                 \
        while (high >= low) {                                             \
            /* We do this to prevent overflows when calculating 'mid'. */ \
            mid = low + (high - low) / 2;                                 \
            other_element = al->array[mid];                               \
            if (eq(element, other_element)) {                             \
                return mid;                                               \
            } else if (gt(element, other_element)) {                      \
                low = mid + 1;                                            \
            } else {                                                      \
                if (mid == 0) break;                                      \
                high = mid - 1;                                           \
            }                                                             \
        }                                                                 \
        return -1;                                                        \
    }                                                                     \
                                                                          \
    bool methprefix##_contains(AT const *al, ET element) {                \
        return (methprefix##_index(al, element) != -1);                   \
    }                                                                     \
                                                                          \
    bool methprefix##_binary_search_contains(AT const *al, ET element) {  \
        return (methprefix##_binary_search_index(al, element) != -1);     \
    }                                                                     \
                                                                          \
    void methprefix##_print(AT const *al, char const *const prefix) {     \
        assert(al != NULL);                                               \
        COMMENT("%s" #AT "<%luB> {", prefix, sizeof(ET) * al->cap);       \
        if (al->size < 1) {                                               \
            printf("/}");                                                 \
            return;                                                       \
        }                                                                 \
        printf("\n");                                                     \
        uint32_t i;                                                       \
        void const *addr;                                                 \
        for (i = 0; i < al->cap; ++i) {                                   \
            addr = al->array + i;                                         \
            COMMENT("%s  [%u]%p: ", prefix, i, addr);                     \
            if (i < al->size) {                                           \
                printf(fmt "\n", fmt_args(al->array[i]));                 \
            } else {                                                      \
                printf("-\n");                                            \
            }                                                             \
        }                                                                 \
        COMMENT("%s}", prefix);                                           \
    }

#define _IMPL_AL_DEF_GT(e0, e1) ((e0) > (e1))
#define _IMPL_AL_DEF_EQ(e0, e1) ((e0) == (e1))
#define _IMPL_AL_DEF_FMT(e)     (e)

/* ~~~~~~~~~~~~~~~~~~~~ Basic Types ~~~~~~~~~~~~~~~~~~~~ */

_IMPL_AL(uint8_t, ArrayList_uint8_t, al8, _IMPL_AL_DEF_GT, _IMPL_AL_DEF_EQ, "%hhu",
         _IMPL_AL_DEF_FMT)
_IMPL_AL(Variable, ArrayList_Variable_t, alvar, _IMPL_AL_DEF_GT, _IMPL_AL_DEF_EQ, "%u",
         _IMPL_AL_DEF_FMT)
_IMPL_AL(Literal, ArrayList_Literal_t, allit, _IMPL_AL_DEF_GT, _IMPL_AL_DEF_EQ, LIT_FMT,
         LIT_FMT_ARGS)
_IMPL_AL(uint32_t, ArrayList_uint32_t, al32, _IMPL_AL_DEF_GT, _IMPL_AL_DEF_EQ, "%u",
         _IMPL_AL_DEF_FMT)
_IMPL_AL(void *, ArrayList_ptr_t, alptr, _IMPL_AL_DEF_GT, _IMPL_AL_DEF_EQ, "%p",
         _IMPL_AL_DEF_FMT)

/* ~~~~~~~~~~~~~~~~~~~~ Custom Types ~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~ Special Functions ~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

char *
al8_to_str(ArrayList_uint8_t *al) {
    // '+ 1' for '\0' byte
    char *str = malloc(al->size + 1);
    assert(str != NULL);
    for (uint32_t i = 0; i < al->size; ++i) {
        str[i] = al8_get(al, i);
    }
    str[al->size] = '\0';
    return str;
}
