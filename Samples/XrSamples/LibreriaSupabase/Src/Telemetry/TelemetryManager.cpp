#include "TelemetryManager.h"
#include <android/log.h>

#define ALOG(...) __android_log_print(ANDROID_LOG_INFO, "VRTelemetry", __VA_ARGS__)

namespace VRTelemetry {

    TelemetryManager::TelemetryManager()
            : currentFileIndex(0), frameCount(0), isInitialized(false) {
        frameBuffer.reserve(5400); // Reservar memoria para eficiencia
    }

    TelemetryManager::~TelemetryManager() {
        shutdown();
    }

    bool TelemetryManager::initialize(std::unique_ptr<ITelemetryUploader> uploaderImpl,
                                      const TelemetryConfig& cfg) {
        if (isInitialized) {
            ALOG("TelemetryManager already initialized");
            return true;
        }

        config = cfg;
        uploader = std::move(uploaderImpl);

        if (!uploader) {
            ALOG("Error: No uploader provided");
            return false;
        }

        // Inicializar uploader
        if (!uploader->initialize(config)) {
            ALOG("Error: Failed to initialize uploader");
            return false;
        }

        // Generar nombre base y crear sesión
        baseFilename = generateBaseFilename();
        startTime = std::chrono::high_resolution_clock::now();

        if (config.enableCloudUpload) {
            if (!uploader->createSession("Meta Quest - LibreriaSupabase App")) {
                ALOG("Warning: Failed to create cloud session, continuing with local only");
            }
        }

        isInitialized = true;
        ALOG("TelemetryManager initialized successfully. Session: %s",
             getSessionId().c_str());

        return true;
    }

    void TelemetryManager::shutdown() {
        if (!isInitialized) return;

        ALOG("Shutting down TelemetryManager...");

        // Guardar datos restantes
        if (!frameBuffer.empty()) {
            saveBufferToFile();
            if (config.enableCloudUpload) {
                uploadBufferToCloud();
            }
        }

        // Limpiar uploader
        if (uploader) {
            uploader->shutdown();
            uploader.reset();
        }

        ALOG("TelemetryManager shutdown complete. Total frames: %d, Files: %d",
             frameCount, currentFileIndex + 1);

        isInitialized = false;
    }

    void TelemetryManager::recordFrame(const VRFrameData& frameData) {
        if (!isInitialized) return;

        frameBuffer.push_back(frameData);
        frameCount++;

        // Si el buffer está lleno, procesarlo
        if (frameBuffer.size() >= config.maxFramesPerFile) {
            saveBufferToFile();

            if (config.enableCloudUpload) {
                uploadBufferToCloud();
            }

            frameBuffer.clear();
            currentFileIndex++;
        }
    }

    void TelemetryManager::forceUpload() {
        if (!isInitialized || frameBuffer.empty()) return;

        saveBufferToFile();
        if (config.enableCloudUpload) {
            uploadBufferToCloud();
        }
        frameBuffer.clear();
        currentFileIndex++;
    }

    std::string TelemetryManager::getSessionId() const {
        if (uploader) {
            return uploader->getSessionId();
        }
        return "no_session";
    }

    std::string TelemetryManager::generateBaseFilename() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);

        std::ostringstream oss;
        oss << "vr_motion_" << std::put_time(&tm, "%Y%m%d_%H%M%S");
        return oss.str();
    }

    std::string TelemetryManager::getCurrentFilename() const {
        std::ostringstream oss;
        oss << baseFilename << "_part" << std::setfill('0') << std::setw(3) << currentFileIndex << ".csv";
        return oss.str();
    }

    void TelemetryManager::saveBufferToFile() {
        if (frameBuffer.empty() || !config.enableLocalBackup) return;

        std::string filename = getCurrentFilename();
        std::ofstream file(filename);

        if (file.is_open()) {
            // Escribir cabecera CSV
            file << "timestamp,head_pos_x,head_pos_y,head_pos_z,"
                 << "head_rot_x,head_rot_y,head_rot_z,head_rot_w,"
                 << "left_tracked,left_pos_x,left_pos_y,left_pos_z,"
                 << "left_rot_x,left_rot_y,left_rot_z,left_rot_w,left_trigger,"
                 << "right_tracked,right_pos_x,right_pos_y,right_pos_z,"
                 << "right_rot_x,right_rot_y,right_rot_z,right_rot_w,right_trigger,"
                 << "button_a\n";

            // Escribir datos
            for (const auto& frame : frameBuffer) {
                file << frame.toCSV() << "\n";
            }

            file.close();
            ALOG("Saved %zu frames to %s", frameBuffer.size(), filename.c_str());
        } else {
            ALOG("Error: Could not open file %s for writing", filename.c_str());
        }
    }

    void TelemetryManager::uploadBufferToCloud() {
        if (frameBuffer.empty() || !uploader) return;

        std::string filename = getCurrentFilename();
        bool success = uploader->uploadFrameData(frameBuffer, filename);

        if (success) {
            ALOG("Successfully uploaded %s to cloud", filename.c_str());
        } else {
            ALOG("Failed to upload %s to cloud", filename.c_str());
        }
    }

} // namespace VRTelemetry