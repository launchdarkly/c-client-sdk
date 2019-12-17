# Change log

All notable changes to the LaunchDarkly C SDK will be documented in this file. This project adheres to [Semantic Versioning](http://semver.org).

## [1.7.4] - 2019-12-17
### Changed:
- Artifacts are now released as zip / tar files containing both binaries and headers. No SDK code changes associated with this version bump.

## [1.7.3] - 2019-12-16
### Fixed:
- Stopped using `clock_gettime` on OSX which is not supported for older OS versions.

## [1.7.2] - 2019-12-16
### Fixed:
- Updated cJSON dependency to 1.7.12 (to resolve several known security vulnerabilities)

## [1.7.1] - 2019-10-25
### Fixed
- Several race conditions in background threads causing lag when closing the client
- Increased validation of HTTP responses
- Simplified internal thread tracking logic
- Increased test coverage and documentation of variation methods

## [1.7.0] - 2019-09-26
### Added
- Added `LDClientTrackMetric` which is an extended version of `LDClientTrackData` but with an extra associated metric value.

## [1.6.0] - 2019-09-11
### Added
- Added the `LDConfigSetSSLCertificateAuthority` function. This can be used to specify an alternative certificate authority (such as your own private authority). (Thanks, Aditya Kulkarni!)

## [1.5.0] - 2019-07-26
### Added
- Added the `LDUserFree` function. This can be used to free a user object *before* it has been passed to the client
### Fixed
- A leak of HTTP headers set for requests to LaunchDarkly
- A leak of the in memory flag store when `LDClientIdentify` is called. This was introduced in `1.2.0`
- A leak of a structure when events are flushed but no events need to be sent

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
