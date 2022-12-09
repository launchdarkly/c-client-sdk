# C SDK Common

Shared components for `c-server-sdk`, and `c-client-sdk`. This repository is *private* because it is intended to be used as a git subtree.

## Updating

A git subtree is not automatically updated. From either the `c-client-sdk` or `c-server-sdk` repository run: `git subtree pull --prefix c-sdk-common git@github.com:launchdarkly/c-sdk-common.git main --squash`.

## Headers

All headers live in the `include` directory. Experimental APIs that aren't intended for semver should
live in the `include/launchdarkly/experimental` subdirectory.
