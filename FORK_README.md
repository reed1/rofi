# Fork notes

This is a personal fork of [davatorium/rofi](https://github.com/davatorium/rofi).
All custom work lives on the `reed` branch, based on the upstream `2.0.0` release.

## Why this fork exists

### Tiered sorting

The built-in sorting methods (`normal`/levenshtein and `fzf`) do not order
results the way I want for project pickers. Typing `mex` against a list of
project names should surface entries by how directly they match:

```
mexico    # exact prefix match
amexico   # exact substring match (not at the start)
medixr    # spread / subsequence match (m … e … x)
```

The existing methods interleave substring and subsequence matches
(`mexico medixr amexico`), which buries the more relevant `amexico`.

## What was changed

A new `-sorting-method tiered` option that ranks matches in tiers
(lower sorts higher):

- **Tier 0** — `str` starts with the input (exact prefix)
- **Tier 1** — `str` contains the input as a contiguous substring
- **Tier 2** — the input is a (non-contiguous) subsequence of `str` (spread)

Within a tier, shorter entries and earlier / tighter matches rank first.

A second `-sorting-method tiered-alphabetic` option uses the same tiers but
first strips **all non-alphabetic characters** from both the input and the
entry, so a leading `.` (or any punctuation) does not demote an entry. Searching
`do` then treats `.dotfiles` as a prefix match, same tier as `dooit`. It applies
**no intra-tier tiebreak**, so entries sharing a tier keep their original input
order (rofi sorts with `g_qsort_with_data`, a stable merge sort).

Both scorers strip pango markup before ranking (mirroring the matcher, which
already does this in `dmenu.c`). rofi otherwise scores against the raw entry,
so with `-markup-rows` every label starts with `<span …>` — the prefix tier
would never fire and the substring tiebreak would be skewed by the (variable)
markup-prefix length, burying entries that carry extra tags like `<b>`.

Pair either with `-matching fuzzy`, which admits all three kinds of match as
candidates so the tiered sorter has something to rank.

The implementation is kept in its own files to minimize the diff against
upstream and keep `git pull upstream` conflict-free. Original files carry only
the few lines of integration glue needed to reach the new code.

New files (all fork-only):

- `include/tiered-sort.h` — declares the scorers
- `source/tiered-sort.c` — implements `rofi_scorer_tiered_evaluate` and
  `rofi_scorer_tiered_alphabetic_evaluate`

Original files touched (integration hooks only):

- `include/settings.h` — add `SORT_TIERED` / `SORT_TIERED_ALPHABETIC` to the
  `SortingMethod` enum
- `source/helper.c` — parse `"tiered"` / `"tiered-alphabetic"` in
  `config_sanity_check`
- `source/view.c` — `#include "tiered-sort.h"` and dispatch the tiered methods
  to their scorers in `filter_elements`
- `meson.build` — add `source/tiered-sort.c` to the `rofi` sources

The man page (`doc/rofi.1.markdown`) is intentionally left untouched to avoid
conflicts; the `tiered` method is documented here instead.

### Chord matching

A `-matching chord` method plus a `-chord-select` flag, which together give
rofi the behaviour of my `chord` script launcher: type the *initials* of an
entry and it runs.

The chord of an entry is the first character of each `-` separated segment:
`rofi-path-dotfiles` is `rpd`. `-matching chord` keeps an entry whose chord
*starts with* the input.

Unlike every other matching method this one is **not a regex**.
`create_regex()` stores the typed chord on the matcher and leaves `regex`
NULL; `helper_token_match` compares it against the entry's initials directly.
Initials are not something a regex expresses well, and the two things the
feature needs beyond a yes/no answer — where the initials sit, so they can be
highlighted, and whether the input has spelled *all* of them — are a scan of
the string either way.

`-chord-select` runs the lone remaining candidate the moment the input spells
out its **complete** chord. A chord that merely narrows to one candidate is not
enough: otherwise the trailing keystrokes of a longer chord would leak into
whatever window comes up next. It sits beside the existing `-auto-select` check
in `filter_elements`.

New files (all fork-only):

- `include/chord-match.h` — declares the matcher, the completeness test and the
  highlight spans
- `source/chord-match.c` — implements `rofi_chord_match`,
  `rofi_chord_is_complete` and `rofi_chord_initial_spans`

Original files touched (integration hooks only):

- `include/settings.h` — add `MM_CHORD` to `MatchingMethod` (bumping
  `MM_NUM_MATCHERS`) and a `chord_select` field
- `include/rofi-types.h` — carry the typed chord on `rofi_int_matcher`, for the
  tokens that have no regex
- `source/helper.c` — add `"Chord"` to `MatchingMethodStr` (which is what
  `config_sanity_check` parses `-matching` against, so no separate parser), a
  `case MM_CHORD` in `create_regex`, and the chord branches in
  `helper_token_match`, `helper_token_match_get_pango_attr` and
  `helper_tokenize_free`
- `source/view.c` — `#include "chord-match.h"` and the `-chord-select` check
  next to `-auto-select` in `filter_elements`
- `source/xrmoptions.c` — register `-chord-select`
- `config/config.c` — default `chord_select` to `FALSE`
- `meson.build` — add `source/chord-match.c` to the `rofi` sources

Nothing outside `source/helper.c` touches `rofi_int_matcher->regex`, so a
matcher without one stays contained.

A plugin (`.so`) mode could not do this: `include/mode-private.h` gives a
plugin custom matching and display, but no way to say "accept this entry now",
so the auto-fire has to come from the view. Doing it as a matching method is
also more general — it works on any dmenu list, not just one mode's entries.

## How to use

```bash
rofi -dmenu -matching fuzzy -sort -sorting-method tiered
rofi -dmenu -i -matching chord -chord-select
```

In this dotfiles repo `tiered-alphabetic` is wired into the project picker at
`rlocal/app/rofi-vscode/shared/interface.py`, and `chord` into the script
launcher at `rlocal/app/chord/main.py`.

## Packaging

Built and installed as `rd-rofi` via
`rlocal/lib/pacman/pkgbuilds/rd-rofi/PKGBUILD` (`rpkgbuild rd-rofi`).
