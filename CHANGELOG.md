# Change log

All notable changes to the LaunchDarkly C SDK will be documented in this file. This project adheres to [Semantic Versioning](http://semver.org).

## [1.1.1] - 2019-03-05
* Improved Windows build documentation
* Fixed a deadlock manifesting on Windows systems
* Added logging on authorization failure

## [1.1.0] - 2019-02-01
### Added:
- It is now possible to obtain information about how each feature flag value was determined (did the user match a target list or a rule, was the flag turned off, etc.). To do this, add `LDConfigSetUseEvaluationReasons(config, true)` to your configuration, and then call `LD____VariationDetail` instea of `LD____Variation` when evaluating a flag. The "Detail" functions will fill in an `LDVariationDetails` struct whose `reason` field is a JSON data structure. See [`DOCS.md`](DOCS.md) and the SDK reference guide on ["Evaluation reasons"](https://docs.launchdarkly.com/v2.0/docs/evaluation-reasons).

## [1.0.0] - 2019-01-10
Initial release.
