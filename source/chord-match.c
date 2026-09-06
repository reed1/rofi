#include "chord-match.h"

#include <glib.h>
#include <pango/pango.h>
#include <string.h>

// Entries reach us as raw text, which with -markup-rows still carries pango
// markup. Falls back to a plain copy when there is no markup or it fails to
// parse.
static char *strip_markup(const char *s) {
  char *out = NULL;
  if (strchr(s, '<') != NULL &&
      pango_parse_markup(s, -1, 0, NULL, &out, NULL, NULL)) {
    return out;
  }
  return g_strdup(s);
}

// First character of each '-' separated segment. `spans`, when non-NULL,
// collects the byte range of every initial within `entry`.
static char *initials(const char *entry, GArray *spans) {
  GString *out = g_string_new(NULL);
  gboolean at_segment_start = TRUE;

  for (const char *it = entry; *it; it = g_utf8_next_char(it)) {
    if (*it == '-') {
      at_segment_start = TRUE;
      continue;
    }
    if (at_segment_start) {
      const char *next = g_utf8_next_char(it);
      g_string_append_len(out, it, next - it);
      if (spans != NULL) {
        rofi_range_pair span = {.start = (int)(it - entry),
                                .stop = (int)(next - entry)};
        g_array_append_val(spans, span);
      }
      at_segment_start = FALSE;
    }
  }

  return g_string_free(out, FALSE);
}

static char *fold(char *s, int case_sensitive) {
  if (case_sensitive) {
    return s;
  }
  char *folded = g_utf8_casefold(s, -1);
  g_free(s);
  return folded;
}

// The chord of `entry`, ready to compare against a typed one.
static char *entry_chord(const char *entry, int case_sensitive) {
  char *plain = strip_markup(entry);
  char *chord = initials(plain, NULL);
  g_free(plain);
  return fold(chord, case_sensitive);
}

gboolean rofi_chord_match(const char *chord, const char *entry,
                          int case_sensitive) {
  if (chord == NULL || *chord == '\0') {
    return TRUE;
  }

  char *typed = fold(g_strdup(chord), case_sensitive);
  char *candidate = entry_chord(entry, case_sensitive);
  gboolean match = g_str_has_prefix(candidate, typed);

  g_free(typed);
  g_free(candidate);
  return match;
}

gboolean rofi_chord_is_complete(const char *chord, const char *entry,
                                int case_sensitive) {
  if (chord == NULL || *chord == '\0' || entry == NULL) {
    return FALSE;
  }

  char *typed = fold(g_strdup(chord), case_sensitive);
  char *candidate = entry_chord(entry, case_sensitive);
  gboolean complete = g_strcmp0(typed, candidate) == 0;

  g_free(typed);
  g_free(candidate);
  return complete;
}

GArray *rofi_chord_initial_spans(const char *chord, const char *entry) {
  GArray *spans = g_array_new(FALSE, FALSE, sizeof(rofi_range_pair));

  char *plain = strip_markup(entry);
  char *candidate = initials(plain, spans);
  g_free(candidate);
  g_free(plain);

  // Only the initials typed so far are lit; the rest of the entry stays plain.
  guint typed = (guint)g_utf8_strlen(chord, -1);
  if (spans->len > typed) {
    g_array_set_size(spans, typed);
  }

  return spans;
}
