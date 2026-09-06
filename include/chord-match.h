#ifndef ROFI_CHORD_MATCH_H
#define ROFI_CHORD_MATCH_H

#include "rofi-types.h"
#include <glib.h>

/**
 * The chord of an entry is the first character of each '-' separated segment:
 * "rofi-path-dotfiles" is "rpd". Empty segments are skipped, so "a--b" is
 * "ab". Pango markup is stripped first, so -markup-rows labels chord the same
 * as their plain text.
 *
 * This is not a regex like the other matching methods. Initials are not
 * something a regex expresses well, and the two things `-matching chord` needs
 * on top of a yes/no answer — where the initials sit, and whether the input
 * has spelled all of them — are a scan of the string either way.
 */

/**
 * @param chord The typed chord.
 * @param entry The entry to test.
 * @param case_sensitive Whether case is significant.
 *
 * @returns TRUE when `entry`'s chord starts with `chord`.
 */
gboolean rofi_chord_match(const char *chord, const char *entry,
                          int case_sensitive);

/**
 * @param chord The typed chord.
 * @param entry The entry to test.
 * @param case_sensitive Whether case is significant.
 *
 * Whether `chord` spells out the *complete* chord of `entry`. This is what
 * `-chord-select` fires on: a chord that merely narrows to one candidate is
 * not enough, otherwise the trailing keystrokes of a longer chord would leak
 * into whatever window comes up next.
 *
 * @returns TRUE when the chord is complete.
 */
gboolean rofi_chord_is_complete(const char *chord, const char *entry,
                                int case_sensitive);

/**
 * @param chord The typed chord.
 * @param entry The entry to highlight.
 *
 * Byte ranges, within `entry`, of the initials `chord` has spelled out so far.
 * Lets the matched initials light up as they are typed while the rest of the
 * entry stays plain.
 *
 * @returns a #GArray of #rofi_range_pair, to unref after use.
 */
GArray *rofi_chord_initial_spans(const char *chord, const char *entry);

#endif // ROFI_CHORD_MATCH_H
