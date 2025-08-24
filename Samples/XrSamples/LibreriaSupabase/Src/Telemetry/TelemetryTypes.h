#pragma once

#include "VRTypes.h"
#include <string>
#include <vector>

namespace VRTelemetry {

    // Ahora FrameData es simplemente un alias de VRFrameData
    using FrameData = VRFrameData;

    // Configuración de telemetría
    struct TelemetryConfig {
        std::string supabaseUrl = "https://npgdluxvrigtrlexcwjj.supabase.co";
        std::string apiKey = "sb_publishable_6lmLqzJKoN3_AMLn--wZZg_ch1QWcaf";
        size_t maxFramesPerFile = 5400;  // CAMBIADO: int → size_t
        size_t maxFramesInMemory = 1800; // CAMBIADO: int → size_t
        bool enableLocalBackup = true;
        bool enableCloudUpload = true;
    };

    // Interface para uploaders (sin cambios)
    class ITelemetryUploader {
    public:
        virtual ~ITelemetryUploader() = default;
        virtual bool initialize(const TelemetryConfig& config) = 0;
        virtual bool createSession(const std::string& deviceInfo) = 0;
        virtual bool uploadFrameData(const std::vector<FrameData>& frames, const std::string& filename) = 0;
        virtual void shutdown() = 0;
        virtual std::string getSessionId() const = 0;
    };

} // namespace VRTelemetry