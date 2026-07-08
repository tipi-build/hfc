# Landing a change

HFC is a source package: we don't transform sources at release time, so every commit that lands on `main` must be releasable on its own. Please make sure this PR meets the following before it lands.

## Every commit stands on its own
- [ ] The PR may contain multiple commits, but each one is self-contained and could be released as-is (their order matters).
- [ ] Every commit builds and passes all checks on its own.
- [ ] Every commit has a descriptive first line.
- [ ] A `Change-Id:` line is present in each commit's trailer so it stays compatible with Gerrit.

## Linear history
- [ ] The commits are rebased on top of the latest `main` (no merge commits).
- [ ] All checks pass on the rebased commits.

## Landing
- [ ] Land the change using **Rebase and merge** to keep history linear.
