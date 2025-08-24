#ifdef ANDROID

#include "AndroidUploader.h"
#include <android/log.h>
#include <sstream>
#include <iomanip>
#include <chrono>

#define ALOG(...) __android_log_print(ANDROID_LOG_INFO, "AndroidUploader", __VA_ARGS__)

namespace VRTelemetry {

    AndroidUploader::AndroidUploader()
        : javaVM(nullptr), activityObject(nullptr), isInitialized(false) {
    }

    AndroidUploader::~AndroidUploader() {
        shutdown();
    }

    void AndroidUploader::setJavaContext(JavaVM* vm, jobject activity) {
        javaVM = vm;
        activityObject = activity;
        ALOG("Java context configured");
    }

    bool AndroidUploader::initialize(const TelemetryConfig& cfg) {
        if (isInitialized) {
            ALOG("AndroidUploader already initialized");
            return true;
        }

        config = cfg;
        sessionId = generateSessionId();
        isInitialized = true;

        ALOG("AndroidUploader initialized. Session ID: %s", sessionId.c_str());
        return true;
    }

    bool AndroidUploader::createSession(const std::string& deviceInfo) {
        if (!isInitialized) {
            ALOG("AndroidUploader not initialized");
            return false;
        }

        std::string jsonData = createSessionJson();
        std::string url = config.supabaseUrl + "/rest/v1/vr_sessions";

        bool success = makeHttpRequest(url, "POST", jsonData);

        if (success) {
            ALOG("Session created successfully in Supabase");
        } else {
            ALOG("Failed to create session in Supabase");
        }

        return success;
    }

    bool AndroidUploader::uploadFrameData(const std::vector<FrameData>& frames, const std::string& filename) {
        if (!isInitialized || frames.empty()) {
            return false;
        }

        std::string jsonData = createFrameDataJson(frames);
        std::string url = config.supabaseUrl + "/rest/v1/vr_movement_data";

        bool success = makeHttpRequest(url, "POST", jsonData);

        if (success) {
            ALOG("Successfully uploaded %zu frames for %s", frames.size(), filename.c_str());
        } else {
            ALOG("Failed to upload frames for %s", filename.c_str());
        }

        return success;
    }

    void AndroidUploader::shutdown() {
        if (isInitialized) {
            ALOG("AndroidUploader shutdown");
            isInitialized = false;
        }
    }

    std::string AndroidUploader::getSessionId() const {
        return sessionId;
    }

    std::string AndroidUploader::generateSessionId() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << "session_" << time_t << "_" << ms.count();
        return oss.str();
    }

    std::string AndroidUploader::createSessionJson() {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

        std::ostringstream oss;
        oss << "{"
             << "\"session_id\":\"" << sessionId << "\","
             << "\"device_info\":\"Meta Quest - LibreriaSupabase\","
             << "\"start_time\":" << timestamp
             << "}";
        return oss.str();
    }

    std::string AndroidUploader::createFrameDataJson(const std::vector<FrameData>& frameData) {
        std::ostringstream json;
        json << "[";

        for (size_t i = 0; i < frameData.size(); ++i) {
            const auto& frame = frameData[i];
            json << "{"
                 << "\"session_id\":\"" << sessionId << "\","
                 << "\"timestamp\":" << frame.timestamp << ","
                 << "\"frame_data\":\"" << escapeJsonString(frame.toCSV()) << "\","
                 << "\"head_pos_x\":" << frame.headPose.x << ","
                 << "\"head_pos_y\":" << frame.headPose.y << ","
                 << "\"head_pos_z\":" << frame.headPose.z << ","
                 << "\"head_rot_x\":" << frame.headPose.qx << ","
                 << "\"head_rot_y\":" << frame.headPose.qy << ","
                 << "\"head_rot_z\":" << frame.headPose.qz << ","
                 << "\"head_rot_w\":" << frame.headPose.qw << ","
                 << "\"left_tracked\":" << (frame.leftController.isTracked ? "true" : "false") << ","
                 << "\"left_pos_x\":" << frame.leftController.pose.x << ","
                 << "\"left_pos_y\":" << frame.leftController.pose.y << ","
                 << "\"left_pos_z\":" << frame.leftController.pose.z << ","
                 << "\"left_rot_x\":" << frame.leftController.pose.qx << ","
                 << "\"left_rot_y\":" << frame.leftController.pose.qy << ","
                 << "\"left_rot_z\":" << frame.leftController.pose.qz << ","
                 << "\"left_rot_w\":" << frame.leftController.pose.qw << ","
                 << "\"left_trigger\":" << frame.leftController.triggerValue << ","
                 << "\"right_tracked\":" << (frame.rightController.isTracked ? "true" : "false") << ","
                 << "\"right_pos_x\":" << frame.rightController.pose.x << ","
                 << "\"right_pos_y\":" << frame.rightController.pose.y << ","
                 << "\"right_pos_z\":" << frame.rightController.pose.z << ","
                 << "\"right_rot_x\":" << frame.rightController.pose.qx << ","
                 << "\"right_rot_y\":" << frame.rightController.pose.qy << ","
                 << "\"right_rot_z\":" << frame.rightController.pose.qz << ","
                 << "\"right_rot_w\":" << frame.rightController.pose.qw << ","
                 << "\"right_trigger\":" << frame.rightController.triggerValue << ","
                 << "\"button_a\":" << (frame.inputState.buttonA ? "true" : "false")
                 << "}";

            if (i < frameData.size() - 1) {
                json << ",";
            }
        }

        json << "]";
        return json.str();
    }

    std::string AndroidUploader::escapeJsonString(const std::string& input) {
        std::string result;
        for (char c : input) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c; break;
            }
        }
        return result;
    }

    bool AndroidUploader::makeHttpRequest(const std::string& url, const std::string& method, const std::string& jsonData) {
        if (!javaVM || !activityObject) {
            ALOG("Java context not available");
            return false;
        }

        JNIEnv* env;
        bool detach = false;

        int status = javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
        if (status == JNI_EDETACHED) {
            if (javaVM->AttachCurrentThread(&env, nullptr) != 0) {
                ALOG("Failed to attach thread to JVM");
                return false;
            }
            detach = true;
        }

        bool result = false;

        try {
            // CORRECCIÓN: Package name correcto que coincide con HttpHelper.java
            jclass httpHelperClass = env->FindClass("io/github/migueldulu/LibreriaSupabase/HttpHelper");
            if (!httpHelperClass) {
                ALOG("HttpHelper class not found - checking if class exists...");

                // Intentar verificar si hay excepciones JNI pendientes
                if (env->ExceptionCheck()) {
                    env->ExceptionDescribe(); // Esto imprime la excepción detallada
                    env->ExceptionClear();
                }

                throw std::runtime_error("HttpHelper class not found");
            }

            // Obtener el método makeRequest
            jmethodID makeRequestMethod = env->GetStaticMethodID(
                httpHelperClass,
                "makeRequest",
                "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z"
            );

            if (!makeRequestMethod) {
                ALOG("makeRequest method not found");

                if (env->ExceptionCheck()) {
                    env->ExceptionDescribe();
                    env->ExceptionClear();
                }

                throw std::runtime_error("makeRequest method not found");
            }

            // Convertir strings a jstring
            jstring jUrl = env->NewStringUTF(url.c_str());
            jstring jMethod = env->NewStringUTF(method.c_str());
            jstring jData = env->NewStringUTF(jsonData.c_str());
            jstring jApiKey = env->NewStringUTF(config.apiKey.c_str());

            // Verificar que todas las strings se crearon correctamente
            if (!jUrl || !jMethod || !jData || !jApiKey) {
                ALOG("Failed to create one or more jstring objects");
                throw std::runtime_error("Failed to create jstring objects");
            }

            ALOG("Calling Java makeRequest method...");
            ALOG("URL: %s", url.c_str());
            ALOG("Method: %s", method.c_str());
            ALOG("JSON size: %zu", jsonData.length());

            // Llamar al método Java
            jboolean jResult = env->CallStaticBooleanMethod(
                httpHelperClass,
                makeRequestMethod,
                jUrl, jMethod, jData, jApiKey
            );

            // Verificar si hubo excepciones durante la llamada
            if (env->ExceptionCheck()) {
                ALOG("Exception occurred during Java method call");
                env->ExceptionDescribe();
                env->ExceptionClear();
                result = false;
            } else {
                result = (bool)jResult;
                ALOG("Java method returned: %s", result ? "true" : "false");
            }

            // Limpiar referencias locales
            env->DeleteLocalRef(jUrl);
            env->DeleteLocalRef(jMethod);
            env->DeleteLocalRef(jData);
            env->DeleteLocalRef(jApiKey);
            env->DeleteLocalRef(httpHelperClass);

        } catch (const std::exception& e) {
            ALOG("Exception in makeHttpRequest: %s", e.what());
            result = false;
        }

        if (detach) {
            javaVM->DetachCurrentThread();
        }

        return result;
    }

} // namespace VRTelemetry

#endif // ANDROID