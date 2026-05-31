# AI Contribution Policy

Porytiles is a small project with a single maintainer.
This document briefly describes how AI-assisted contributions are handled.

## Two tracks

This repository contains two codebases, governed by different rules.

**`Legacy/`** is the original Porytiles compiler, preserved as the `porytiles-legacy` binary.
The Legacy codebase is closed to AI-assisted contributions of any kind, including those from the maintainer.
PRs that touch files under `Legacy/` must be human-authored.
This rule applies to all contributors and is not negotiable.
AI-drafted patches to `Legacy/` will be closed without review.

**`Porytiles/`** is the active codebase, producing the `porytiles` binary.
The active codebase accepts AI-assisted contributions, subject to the same quality bar as any other PR.
There is no required disclosure of AI use nor any automated detection mechanisms. Maintainer judgment is the only filter.

## Review Criteria

The quality bar is [`STYLE.md`](./STYLE.md), [`CONTRIBUTING.md`](./CONTRIBUTING.md), and maintainer discretion.
Code that meets the style guide and the contribution conventions is welcome regardless of how it was produced.
Code that does not is rejected regardless of how it was produced.

Contributions that read as low-effort "slop" or are out of step with the rest of the codebase will be asked to rework or be closed.
There is no formal taxonomy of disqualifying tells: if the maintainer flags quality concerns, you must rework or your contribution will be closed.

Contributors must be able to discuss **any** aspect of their contribution intelligently and understand how it fits into the codebase.
Inability to discuss one's own contribution is disqualifying, as is the response of "the bot wrote it, I don't know how/why it works."

## No disclosure

Contributors are not required to disclose AI use.
The policy is the same whether the patch was hand-written, AI-drafted, or AI-assisted: it must meet the quality bar.

This is deliberate. A required disclosure would either:
- need to be enforced (which requires unreliable detection methods and additional maintenance burden)
- or go unenforced (which reduces it to a purely performative ritual)

Neither helps the maintainer or the project.
