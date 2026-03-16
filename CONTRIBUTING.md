# Contributing to This Fork

Welcome, and thanks for your interest in contributing.

## The One Unbreakable Rule

**Do NOT take any code, tests, ideas, or other material from this repository and
submit it -- in any form, AI-generated or otherwise -- to the upstream UEFITool
repository (LongSoft/UEFITool).**

This includes:
- Opening pull requests on the upstream repo that contain or are derived from work done here
- Copying code from this fork into upstream contributions
- Using this fork's test suite, analyses, or documentation as a basis for upstream PRs
- Paraphrasing, rewriting, or "laundering" AI-assisted work from this fork to make it
  appear human-written for upstream submission

The upstream maintainer has explicitly banned AI-assisted contributions. We respect that
boundary absolutely. **Anyone found to have violated this rule will be permanently banned
from this repository with no appeal.**

This is not a suggestion. This is not flexible. This is the price of admission.

## AI-Assisted Contributions Are Welcome Here

This fork was built primarily using AI-assisted development. Contributions that use AI
tools (LLMs, code generators, copilots, etc.) are fully welcome here, as long as they
meet the same quality bar as any other contribution.

## How to Contribute

1. Fork this repository (not the upstream one)
2. Create a feature branch
3. Write tests first -- this is a test-driven project
4. Make sure both CMake and Meson builds pass
5. Open a PR against this fork

## Code Standards

- C++23
- Linux-first (Windows is nice-to-have, not a constraint)
- LF line endings
- Tests go in `common/tests/`
- Both CMake and Meson must build the test suite

## Questions?

Open an issue on this fork. Do NOT contact the upstream maintainers about anything
related to this fork.
