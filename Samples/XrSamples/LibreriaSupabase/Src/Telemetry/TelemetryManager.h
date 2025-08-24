#pragma once

#include "TelemetryTypes.h"
#include <vector>
#include <memory>
#include <fstream>
#include <chrono>

namespace VRTelemetry {

    class TelemetryManager {
    private:
        std::unique_ptr<ITelemetryUploader> uploader;
        std::vector<FrameData> frameBuffer;
        TelemetryConfig config;

        std::chrono::high_resolution_clock::time_point startTime;
        int currentFileIndex;
        int frameCount;
        std::string baseFilename;
        bool isInitialized;

        // Métodos privados
        std::string generateBaseFilename();
        std::string getCurrentFilename() const;
        void saveBufferToFile();
        void uploadBufferToCloud();

    public:
        TelemetryManager();
        ~TelemetryManager();

        // Configuración
        bool initialize(std::unique_ptr<ITelemetryUploader> uploaderImpl,
                        const TelemetryConfig& cfg = TelemetryConfig{});
        void shutdown();

        // NUEVO: Grabación con datos genéricos (independiente de OpenXR)
        void recordFrame(const VRFrameData& frameData);
        void forceUpload();  // Para envío inmediato

        // Información del estado
        int getTotalFrames() const { return frameCount; }
        int getCurrentFileIndex() const { return currentFileIndex; }
        std::string getSessionId() const;
        bool isReady() const { return isInitialized && uploader != nullptr; }

        // Configuración dinámica
        void setConfig(const TelemetryConfig& newConfig) { config = newConfig; }
        const TelemetryConfig& getConfig() const { return config; }
    };

} // namespace VRTelemetry