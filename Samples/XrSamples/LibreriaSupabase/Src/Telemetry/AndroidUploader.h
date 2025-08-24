#pragma once

#ifdef ANDROID

#include "TelemetryTypes.h"
#include <jni.h>
#include <string>

namespace VRTelemetry {

    class AndroidUploader : public ITelemetryUploader {
    private:
        TelemetryConfig config;
        std::string sessionId;
        JavaVM* javaVM;
        jobject activityObject;
        bool isInitialized;

        // Metodos privados
        std::string generateSessionId();
        std::string createSessionJson();
        std::string createFrameDataJson(const std::vector<FrameData>& frameData);
        std::string escapeJsonString(const std::string& input);
        bool makeHttpRequest(const std::string& url, const std::string& method, const std::string& jsonData);

    public:
        AndroidUploader();
        virtual ~AndroidUploader();

        // Configuracion del contexto Java
        void setJavaContext(JavaVM* vm, jobject activity);

        // Implementacion de ITelemetryUploader
        bool initialize(const TelemetryConfig& cfg) override;
        bool createSession(const std::string& deviceInfo) override;
        bool uploadFrameData(const std::vector<FrameData>& frames, const std::string& filename) override;
        void shutdown() override;
        std::string getSessionId() const override;
    };

} // namespace VRTelemetry

#endif // ANDROID