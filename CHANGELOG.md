# Change log

All notable changes to the LaunchDarkly C SDK will be documented in this file. This project adheres to [Semantic Versioning](http://semver.org).

## [1.2.2] - 2019-04-16
### Fixed:
- Ensure `LDSetLogFunction` is properly exported in shared libraries

## [1.2.1] - 2019-04-12
### Fixed:
- Cleaned up exported symbols in shared libraries

## [1.2.0] - 2019-04-11
### Added:
- Added multiple environments support. It is now possible to evaluate flags in additional environments. You can add multiple secondary environments with `LDConfigAddSecondaryMobileKey`, and use an environment with `LDClientGetForMobileKey`.
- Added `LDNodeAppendHash`, and `LDNodeAppendArray`
- Exposed `LDConfigFree` (not needed in standard usage)
### Fixed:
- Switched to bitflag comparison routines as per cJSON doc
- Memory leak of background thread stack

## [1.1.1] - 2019-03-05
### Added:
- Improved Windows build documentation
- Added logging on authorization failure
### Fixed:
- Fixed a deadlock manifesting on Windows systems

## [1.1.0] - 2019-02-01
### Added:
- It is now possible to obtain information about how each feature flag value was determined (did the user match a target list or a rule, was the flag turned off, etc.). To do this, add `LDConfigSetUseEvaluationReasons(config, true)` to your configuration, and then call `LD____VariationDetail` instea of `LD____Variation` when evaluating a flag. The "Detail" functions will fill in an `LDVariationDetails` struct whose `reason` field is a JSON data structure. See [`DOCS.md`](DOCS.md) and the SDK reference guide on ["Evaluation reasons"](https://docs.launchdarkly.com/v2.0/docs/evaluation-reasons).

## [1.0.0] - 2019-01-10
Initial release.
