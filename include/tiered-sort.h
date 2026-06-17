#ifndef ROFI_TIERED_SORT_H
#define ROFI_TIERED_SORT_H

#include <glib.h>

/**
 * @param pattern   The input to match against.
 * @param plen      Length of pattern.
 * @param str       The entry to match.
 * @param slen      Length of str.
 * @param case_sensitive Whether case is significant.
 *
 *  rofi_scorer_tiered_evaluate ranks entries in tiers (lower is better):
 *  - Tier 0: `str` starts with `pattern` (exact prefix match).
 *  - Tier 1: `str` contains `pattern` as a contiguous substring.
 *  - Tier 2: `pattern` is a (non-contiguous) subsequence of `str` (spread).
 *
 *  Within a tier shorter entries, earlier matches and tighter spreads rank
 * first. Intended to be paired with `-matching fuzzy`, which admits all three
 * kinds of match as candidates.
 *
 * @returns the sorting weight (lower sorts higher).
 */
int rofi_scorer_tiered_evaluate(const char *pattern, glong plen,
                                const char *str, glong slen,
                                const int case_sensitive);

/**
 * Same tiering as rofi_scorer_tiered_evaluate, but all non-alphabetic
 * characters are stripped from both pattern and entry before matching, so a
 * leading '.' (e.g. ".dotfiles") does not demote the entry. Entries sharing a
 * tier are left in their original input order (no intra-tier tiebreak).
 *
 * @returns the tier (lower sorts higher).
 */
int rofi_scorer_tiered_alphabetic_evaluate(const char *pattern, glong plen,
                                           const char *str, glong slen,
                                           const int case_sensitive);

#endif // ROFI_TIERED_SORT_H
