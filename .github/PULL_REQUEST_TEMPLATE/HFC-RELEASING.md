---
name: [HFC] Release
about: Checklist to make a release
title: "[HFC][RELEASE] v0.0."
labels: ''
assignees: ''

---

# How to make a release ?
HFC is a source package in that regard we don't want to have a special workflow transforming the sources or have the sources be different when released than when edited and tested.

In that regard we will make every merged-commit releasable.

- [ ] Check that every commit in the main branch should read as follow : 

```
HermeticFetchContent v1.0.X : <TITLE>
<OR> CONFIG : <TITLE>
<OR> DOC : <TITLE>
<OR> TEST : <TITLE>

<summary...>

CHANGELOG

- User Facing Change Description

Change-Id: <gerrit-compatible-change-id>
```

## Pre-Release
- [ ] Create a `release/v1.0.X` integration branch
  - [ ] Target all PRs that should be in the release to this `release/v1.0.X` branch
  - [ ] Rebase features PRs into `release/v1.0.X` with GH
    - Each feature should be one commit following template above
    - Each feature should have CI status passing

## Release scope : Rebase into current PR
- [ ] #<PR-NUMBER>
- [ ] ...

## Full-Release
- [ ] Test with Canary projects
  - [ ] canary-big
  - [ ] canary-medium
- [ ] Take the `release/v1.0.X` and **fast-forward** it in main
    - [ ] `git checkout main && git pull origin main --ff-only && git merge release/v1.0.X --ff-only` 