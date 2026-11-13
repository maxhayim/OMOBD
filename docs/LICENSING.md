# Licensing and attribution

## Summary

| Material | Owner | Terms |
|---|---|---|
| preOBD code, docs, tools, tests (everything inherited from upstream) | Eric Nichols and the preOBD contributors | MIT + NonCommercial ([LICENSE](../LICENSE)) |
| OMOBD additions in this fork (`src/omobd/`, `dashboard/`, `engine-profiles/`, OMOBD docs/tests/tools, and edits to upstream files) | OMOBD contributors | Distributed under the same MIT + NonCommercial terms, because they are modifications of preOBD |
| EDMS-II concepts and documentation | Paul (PeachParts mach4 / Idahobenz) | Used as a foundation **with his permission**, credited throughout. No EDMS-II files or photos are included yet. |
| PeachParts thread content (posts, photos, drawings) | Each post's author | **Not included.** Linked only. |

## What the preOBD licence allows

The full text is in [LICENSE](../LICENSE) and must stay unchanged in every
copy. In short:

- **Allowed:** use, copy, modify, merge, publish and distribute for personal,
  educational or other non-commercial purposes, keeping the copyright notice,
  permission notice and NonCommercial clause.
- **Not allowed without written permission from the copyright holder:**
  selling it or licensing it, including it in anything sold, using it to
  generate revenue, using it inside a for-profit business, or any use for
  commercial advantage. Contact for commercial terms: `info@preobd.com`
  (from the upstream README).

So OMOBD must not be described as "open source" in the OSI sense, as
"free for any use", or as commercially usable. Donations, sponsorship
buttons or paid builds would need checking against clause 2 first; none are
set up for this repository.

## Upstream relationship

- `maxhayim/OMOBD` is a GitHub fork of `preobd/preOBD`; the full upstream Git
  history is preserved.
- The `upstream` remote points at `https://github.com/preobd/preOBD.git` so
  upstream fixes can be merged (`git fetch upstream && git merge upstream/main`).
- Fixes that help preOBD generally (the IDF 5 watchdog fix, for example) should
  be offered back upstream as pull requests.

## EDMS-II material: open permission questions

Paul's permission to use his previous work as a foundation is recorded in
[history/EDMS-II.md](history/EDMS-II.md). Before any of the following are
added to this repository, get explicit written permission and agreed terms
from the author, and record them here:

- [ ] Original source code (Arduino sketch, Processing app), if recovered
- [ ] Schematics and PCB files
- [ ] Photographs, gauge artwork and illustrations from the PeachParts thread
      (the history page paraphrases posts and links to them; nothing is copied)
- [ ] Quoted measurements or text beyond short attributed quotations
- [ ] Material from anyone else who posted in the thread

| Date | Who | What was permitted | Terms | Evidence |
|---|---|---|---|---|
| 2026-11 | Paul (mach4 / Idahobenz) | Use of his previous work as the foundation and starting point for OMOBD, with credit | Credit to Paul | Private correspondence, held by the repository owner |

## Trademarks

Mercedes-Benz, Jeep, Bosch, VDO and other names are trademarks of their
owners, used only to identify compatible equipment.
