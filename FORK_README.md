# Fork notes

This is a personal fork of [davatorium/rofi](https://github.com/davatorium/rofi).
All custom work lives on the `reed` branch, based on the upstream `2.0.0` release.

## Why this fork exists

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

## How to use

```bash
rofi -dmenu -matching fuzzy -sort -sorting-method tiered
```

In this dotfiles repo it is wired into the project picker at
`rlocal/app/rofi-vscode/utils/interface.py`.

## Packaging

Built and installed as `rd-rofi` via
`rlocal/lib/pacman/pkgbuilds/rd-rofi/PKGBUILD` (`rpkgbuild rd-rofi`).
