#include "tiered-sort.h"

#include <glib.h>
#include <string.h>

// Returns a newly-allocated copy of `s` keeping only alphabetic characters,
// casefolded unless case_sensitive. Used so leading/embedded punctuation (e.g.
// a leading '.') does not demote an entry into a lower tier.
static char *alpha_only(const char *s, const int case_sensitive) {
  char *folded = case_sensitive ? g_strdup(s) : g_utf8_casefold(s, -1);
  GString *out = g_string_new(NULL);
  for (const char *it = folded; *it; it = g_utf8_next_char(it)) {
    gunichar c = g_utf8_get_char(it);
    if (g_unichar_isalpha(c)) {
      g_string_append_unichar(out, c);
    }
  }
  g_free(folded);
  return g_string_free(out, FALSE);
}

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

int rofi_scorer_tiered_alphabetic_evaluate(const char *pattern, glong plen,
                                           const char *str,
                                           G_GNUC_UNUSED glong slen,
                                           const int case_sensitive) {
  if (plen == 0) {
    return 0;
  }

  char *p = alpha_only(pattern, case_sensitive);
  char *s = alpha_only(str, case_sensitive);

  int tier;
  if (g_str_has_prefix(s, p)) {
    tier = 0;
  } else if (strstr(s, p) != NULL) {
    tier = 1;
  } else {
    const char *pit = p;
    const char *sit = s;
    while (*pit && *sit) {
      if (g_utf8_get_char(pit) == g_utf8_get_char(sit)) {
        pit = g_utf8_next_char(pit);
      }
      sit = g_utf8_next_char(sit);
    }
    tier = (*pit == '\0') ? 2 : 3;
  }

  g_free(p);
  g_free(s);

  // No intra-tier tiebreak: entries sharing a tier keep their original input
  // order (g_qsort_with_data is a stable merge sort).
  return tier;
}
