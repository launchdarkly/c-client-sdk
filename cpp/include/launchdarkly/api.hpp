#pragma once

#include <launchdarkly/api.h>

class LD_EXPORT(LDClientCPP) {
    public:
        static LDClientCPP *Get(void);

        static LDClientCPP *Init(struct LDConfig *const client,
            struct LDUser *const user,
            const unsigned int maxwaitmilli);

        bool isInitialized(void);

        bool awaitInitialized(const unsigned int timeoutmilli);

        bool boolVariation(const std::string &flagKey, const bool fallback);

        int intVariation(const std::string &flagKey, const int fallback);

        double doubleVariation(const std::string &flagKey,
            const double fallback);

        std::string stringVariation(const std::string &flagKey,
            const std::string &fallback);

        char *stringVariation(const std::string &flagKey,
            const std::string &fallback, char *const resultBuffer,
            const size_t resultBufferSize);

        struct LDJSON *JSONVariation(const std::string &flagKey,
            const struct LDJSON *const fallback);

        bool boolVariationDetail(const std::string &flagKey,
            const bool fallback, LDVariationDetails *const details);

        int intVariationDetail(const std::string &flagKey, const int fallback,
            LDVariationDetails *const details);

        double doubleVariationDetail(const std::string &flagKey,
            const double fallback, LDVariationDetails *const details);

        std::string stringVariationDetail(const std::string &flagKey,
            const std::string &fallback, LDVariationDetails *const details);

        char *stringVariationDetail(const std::string &flagKey,
            const std::string &fallback, char *const resultBuffer,
            const size_t resultBufferSize, LDVariationDetails *const details);

        struct LDJSON *JSONVariationDetail(const std::string &flagKey,
            const struct LDJSON *const fallback,
            LDVariationDetails *const details);

        struct LDJSON *getAllFlags();

        void setOffline();

        void setOnline();

        bool isOffline();

        void setBackground(const bool background);

        std::string saveFlags();

        void restoreFlags(const std::string &flags);

        void identify(LDUser *);

        void track(const std::string &name);

        void track(const std::string &name, struct LDJSON *const data);

        void flush();

        void close();

        bool registerFeatureFlagListener(const std::string &name,
            LDlistenerfn fn);

        void unregisterFeatureFlagListener(const std::string &name,
            LDlistenerfn fn);

    private:
        struct LDClient *client;
};
