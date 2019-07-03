# Change log

All notable changes to the LaunchDarkly C SDK will be documented in this file. This project adheres to [Semantic Versioning](http://semver.org).

## [1.4.0] - 2019-07-03
### Added
- Added the `LDConfigSetVerifyPeer` configuration option. This option allows disabling certificate verification, which may be useful for testing, or in unfortunate networking configurations. Note that certificate verification should not be disabled unless it is essential, as it makes the SDK vulnerable to man-in-the-middle attacks. (Thanks, [mstrater](https://github.com/mstrater)!)

## [1.3.2] - 2019-06-20
### Fixed
- Fixed a bug where reconnecting in streaming mode sometimes erroneously waited for a timeout when switching users

## [1.3.1] - 2019-05-10
### Changed:
- Renamed the build artifacts. They are now prefixed with `ldclientapi`.
- Changed repository references to use the new URL.

There are no other changes in this release. Substituting the build artifacts from version 1.3.0 with the build artifacts from version 1.3.1 will not affect functionality.

## Note on future releases
The LaunchDarkly SDK repositories are being renamed for consistency. This repository is now `c-client-sdk` rather than `c-client`. (Note that `c-server-sdk` also exists, which is the _server-side_ C/C++ SDK.)

The library name will also change. In the 1.3.0 release, it is still `ldapi`; in all future releases, it will be `ldclientapi`.

## [1.3.0] - 2019-04-18
### Added:
- Version string macro `LD_SDK_VERSION` in `ldapi.h`

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
