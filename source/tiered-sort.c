#include "tiered-sort.h"

#include <glib.h>
#include <string.h>

int rofi_scorer_tiered_evaluate(const char *pattern, glong plen,
                                const char *str, G_GNUC_UNUSED glong slen,
                                const int case_sensitive) {
  // Separation between tiers. Per-tier tiebreaks are clamped below this so
  // tiers never overlap, keeping the ordering prefix < substring < spread.
  const int TIER_STRIDE = 1 << 24;

  if (plen == 0) {
    return 0;
  }

  char *p = case_sensitive ? g_strdup(pattern) : g_utf8_casefold(pattern, -1);
  char *s = case_sensitive ? g_strdup(str) : g_utf8_casefold(str, -1);
  int s_bytes = (int)strlen(s);

  int tier;
  int tiebreak;
  const char *hit;

  if (g_str_has_prefix(s, p)) {
    tier = 0;
    tiebreak = s_bytes;
  } else if ((hit = strstr(s, p)) != NULL) {
    tier = 1;
    tiebreak = (int)(hit - s) * 256 + s_bytes;
  } else {
    const char *pit = p;
    const char *sit = s;
    int first = -1, last = -1, idx = 0;
    while (*pit && *sit) {
      if (g_utf8_get_char(pit) == g_utf8_get_char(sit)) {
        if (first < 0) {
          first = idx;
        }
        last = idx;
        pit = g_utf8_next_char(pit);
      }
      sit = g_utf8_next_char(sit);
      idx++;
    }
    if (*pit == '\0') {
      tier = 2;
      tiebreak = (last - first) * 256 + s_bytes;
    } else {
      // Not even a subsequence; should not occur under fuzzy matching.
      tier = 3;
      tiebreak = s_bytes;
    }
  }

  g_free(p);
  g_free(s);

  if (tiebreak < 0) {
    tiebreak = 0;
  }
  if (tiebreak >= TIER_STRIDE) {
    tiebreak = TIER_STRIDE - 1;
  }
  return tier * TIER_STRIDE + tiebreak;
}
