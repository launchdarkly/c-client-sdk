LaunchDarkly Client-Side SDK for C/C++
===================================

# Important note

This repository has been superseded by [C++ Client SDK with C bindings](https://github.com/launchdarkly/cpp-sdks/tree/main/libs/client-sdk).
All future releases will be made from the [new repository](https://github.com/launchdarkly/cpp-sdks/tree/main/libs/client-sdk). Please consider upgrading and filing potential requests in that repository's [issue tracker](https://github.com/launchdarkly/cpp-sdks/issues?q=is%3Aissue+is%3Aopen+label%3A%22package%3A+sdk%2Fclient%22+sort%3Aupdated-desc).

[![CircleCI](https://circleci.com/gh/launchdarkly/c-client-sdk.svg?style=svg)](https://circleci.com/gh/launchdarkly/c-client-sdk)

The LaunchDarkly Client-Side SDK for C/C++ is designed primarily for use in desktop and embedded systems applications. It follows the client-side LaunchDarkly model for single-user contexts (much like our mobile or JavaScript SDKs). It is not intended for use in multi-user systems such as web servers and applications.

For using LaunchDarkly in _server-side_ C/C++ applications, refer to our [server-side C/C++ SDK](https://github.com/launchdarkly/c-server-sdk).

LaunchDarkly overview
-------------------------
[LaunchDarkly](https://www.launchdarkly.com) is a feature management platform that serves over 100 billion feature flags daily to help teams build better software, faster. [Get started](https://docs.launchdarkly.com/docs/getting-started) using LaunchDarkly today!

[![Twitter Follow](https://img.shields.io/twitter/follow/launchdarkly.svg?style=social&label=Follow&maxAge=2592000)](https://twitter.com/intent/follow?screen_name=launchdarkly)

Compatibility
-------------------------

This version of the LaunchDarkly SDK is compatible with POSIX environments (Linux, OS X, BSD) and Windows.

Getting started
---------------

Download a release archive from the [GitHub Releases](https://github.com/launchdarkly/c-client-sdk/releases) for use in your project. Refer to the [SDK documentation](https://docs.launchdarkly.com/sdk/client-side/c-c--#configuring-the-sdk) for complete instructions on installing and using the SDK.

Learn more
-----------

Check out our [documentation](https://docs.launchdarkly.com) for in-depth instructions on configuring and using LaunchDarkly. You can also head straight to the [complete reference guide for this SDK](https://docs.launchdarkly.com/sdk/client-side/c-c--).

Testing
-------

We run integration tests for all our SDKs using a centralized test harness. This approach gives us the ability to test for consistency across SDKs, as well as test networking behavior in a long-running application. These tests cover each method in the SDK, and verify that event sending, flag evaluation, stream reconnection, and other aspects of the SDK all behave correctly.

Contributing
------------

We encourage pull requests and other contributions from the community. Check out our [contributing guidelines](CONTRIBUTING.md) for instructions on how to contribute to this SDK.

About LaunchDarkly
-----------

* LaunchDarkly is a continuous delivery platform that provides feature flags as a service and allows developers to iterate quickly and safely. We allow you to easily flag your features and manage them from the LaunchDarkly dashboard.  With LaunchDarkly, you can:
    * Roll out a new feature to a subset of your users (like a group of users who opt-in to a beta tester group), gathering feedback and bug reports from real-world use cases.
    * Gradually roll out a feature to an increasing percentage of users, and track the effect that the feature has on key metrics (for instance, how likely is a user to complete a purchase if they have feature A versus feature B?).
    * Turn off a feature that you realize is causing performance problems in production, without needing to re-deploy, or even restart the application with a changed configuration file.
    * Grant access to certain features based on user attributes, like payment plan (eg: users on the ‘gold’ plan get access to more features than users in the ‘silver’ plan). Disable parts of your application to facilitate maintenance, without taking everything offline.
* LaunchDarkly provides feature flag SDKs for a wide variety of languages and technologies. Check out [our documentation](https://docs.launchdarkly.com/docs) for a complete list.
* Explore LaunchDarkly
    * [launchdarkly.com](https://www.launchdarkly.com/ "LaunchDarkly Main Website") for more information
    * [docs.launchdarkly.com](https://docs.launchdarkly.com/  "LaunchDarkly Documentation") for our documentation and SDK reference guides
    * [apidocs.launchdarkly.com](https://apidocs.launchdarkly.com/  "LaunchDarkly API Documentation") for our API documentation
    * [blog.launchdarkly.com](https://blog.launchdarkly.com/  "LaunchDarkly Blog Documentation") for the latest product updates
