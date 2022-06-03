/*!
 * @file api.hpp
 * @brief Public C++ bindings for the LaunchDarkly Client Side C/C++ SDK.
 */

#pragma once

#include <launchdarkly/api.h>

class LD_EXPORT(LDClientCPP) {
    public:
        /**
         * @brief Obtain pointer to LDClientCPP singleton. To initialize the singleton,
         * see `LDClientCPP::Init`.
         * @return Pointer to LDClientCPP singleton.
         */
        static LDClientCPP *Get();

        /**
         * @brief Initialize the client.
         * After this call, the `config` and `user` must not be modified.

         * Only a single initialized client may exist at one time.
         * To initialize another instance you must first cleanup the previous client
         * with `LDClientCPP::close`.
         *
         * Should you initialize while another client exists `abort` will be called.
         *
         * Neither `LDClientCPP::Init` nor `LDClientCPP::close` are thread safe.
         *
         * @param config Configuration for the client.
         * @param user Initial user for the client.
         * @param maxwaitmilli The max amount of time the client will wait to be fully initialized.
         * If the timeout is hit, the client will be available for feature flag evaluation, but the results
         * will be fallbacks.
         * The client will continue attempts to connect to LaunchDarkly in the background.
         * If `maxwaitmilli` is set to `0`, then `LDClientCPP::Init` will wait indefinitely.
         * @return Pointer to LDClientCPP singleton.
         */
        static LDClientCPP *Init(struct LDConfig *config,
            struct LDUser *user,
            unsigned int maxwaitmilli);

        /** @brief Returns true if the client has been initialized. */
        bool isInitialized();

        /** @brief Block until initialized up to timeout; returns true if initialized. */
        bool awaitInitialized(unsigned int timeoutmilli);

        /** @brief Evaluate Bool flag. */
        bool boolVariation(const std::string &flagKey, bool fallback);

        /** @brief Evaluate Int flag.
         * If the flag value is actually a float, it will be truncated. */
        int intVariation(const std::string &flagKey, int fallback);

        /** @brief Evaluate Double flag. */
        double doubleVariation(const std::string &flagKey, double fallback);

        /** @brief Evaluate String flag. */
        std::string stringVariation(const std::string &flagKey, const std::string &fallback);

        /** @brief Evaluate String flag, storing result in provided buffer. */
        char *stringVariation(const std::string &flagKey,
            const std::string &fallback, char *resultBuffer, size_t resultBufferSize);

        /** @brief Evaluate JSON flag.
         * @return LDJSON pointer which must be freed with `LDJSONFree`.
         */
        struct LDJSON *JSONVariation(const std::string &flagKey, const struct LDJSON *fallback);

        /** @brief Evaluate Bool flag and obtain additional details. */
        bool boolVariationDetail(const std::string &flagKey, bool fallback, LDVariationDetails *details);

        /** @brief Evaluate Int flag and obtain additional details. */
        int intVariationDetail(const std::string &flagKey, int fallback, LDVariationDetails *details);

        /** @brief Evaluate Double flag and obtain additional details. */
        double doubleVariationDetail(const std::string &flagKey, double fallback, LDVariationDetails *details);

        /** @brief Evaluate String flag and obtain additional details. */
        std::string stringVariationDetail(const std::string &flagKey,
            const std::string &fallback, LDVariationDetails *details);

        /** @brief Evaluate String flag, obtaining additional details.
         * The result is stored in the provided buffer. */
        char *stringVariationDetail(const std::string &flagKey, const std::string &fallback, char *resultBuffer,
            size_t resultBufferSize, LDVariationDetails *details);

        /** @brief Evaluate JSON flag and obtain additional details. */
        struct LDJSON *JSONVariationDetail(const std::string &flagKey, const struct LDJSON *fallback,
                LDVariationDetails *details);

        /** @brief Returns an object of all flags. This must be freed with `LDJSONFree`. */
        struct LDJSON *getAllFlags();

        /** @brief Make the client operate in offline mode. No network traffic. */
        void setOffline();

        /** @brief Return the client to online mode. */
        void setOnline();

        /** @brief Returns the offline status of the client. */
        bool isOffline();

        /** @brief Enable or disable polling mode. */
        void setBackground(bool background);

        /** @brief Get JSON string containing all flags. */
        std::string saveFlags();

        /** @brief Set flag store from JSON string. */
        void restoreFlags(const std::string &flags);

        /** @brief Update the client with a new user.
         *
         * The old user is freed. This will re-fetch feature flag settings from
         * LaunchDarkly. For performance reasons, user contexts should not be
         * changed frequently. */
        void identify(LDUser *user);

        /** @brief Record a custom event. */
        void track(const std::string &name);

        /** @brief Record a custom event and include custom data. */
        void track(const std::string &name, struct LDJSON *data);

        /** @brief  Send any pending events to the server.
         *
         * They will normally be flushed after a timeout, but may also be flushed manually.
         * This operation does not block. */
        void flush();

        /** @brief Close the client, free resources, and generally shut down.
         *
         * This will additionally close all secondary environments. Do not attempt to
         * manage secondary environments directly. */
        void close();

        /** @brief Register a callback for when a flag is updated. */
        bool registerFeatureFlagListener(const std::string &name, LDlistenerfn fn);

        /** @brief Unregister a callback registered with `LDClientRegisterFeatureFlagListener`. */
        void unregisterFeatureFlagListener(const std::string &name, LDlistenerfn fn);
    private:
        struct LDClient *client;
};
