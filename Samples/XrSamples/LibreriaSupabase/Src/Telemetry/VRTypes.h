#pragma once

#include <string>
#include <sstream>
#include <iomanip>

namespace VRTelemetry {

    // Pose 3D genérica (posición + rotación)
    struct VRPose {
        // Posición
        float x, y, z;
        // Rotación (quaternion)
        float qx, qy, qz, qw;

        VRPose() : x(0), y(0), z(0), qx(0), qy(0), qz(0), qw(1) {}

        VRPose(float px, float py, float pz, float rx, float ry, float rz, float rw)
                : x(px), y(py), z(pz), qx(rx), qy(ry), qz(rz), qw(rw) {}
    };

    // Controlador VR genérico
    struct VRController {
        bool isTracked;
        VRPose pose;
        float triggerValue;

        VRController() : isTracked(false), triggerValue(0.0f) {}
    };

    // Estado de botones genérico
    struct VRInputState {
        bool buttonA;
        bool buttonB;
        bool menuButton;
        // Fácil agregar más botones en el futuro

        VRInputState() : buttonA(false), buttonB(false), menuButton(false) {}
    };

    // Datos completos de un frame VR (100% independiente de cualquier SDK)
    struct VRFrameData {
        double timestamp;
        VRPose headPose;
        VRController leftController;
        VRController rightController;
        VRInputState inputState;

        VRFrameData() : timestamp(0.0) {}

        // Convertir a CSV (igual que antes, pero con datos genéricos)
        std::string toCSV() const {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(6)
                << timestamp << ","
                << headPose.x << "," << headPose.y << "," << headPose.z << ","
                << headPose.qx << "," << headPose.qy << "," << headPose.qz << "," << headPose.qw << ","
                << (leftController.isTracked ? "1" : "0") << ","
                << leftController.pose.x << "," << leftController.pose.y << "," << leftController.pose.z << ","
                << leftController.pose.qx << "," << leftController.pose.qy << "," << leftController.pose.qz << "," << leftController.pose.qw << ","
                << leftController.triggerValue << ","
                << (rightController.isTracked ? "1" : "0") << ","
                << rightController.pose.x << "," << rightController.pose.y << "," << rightController.pose.z << ","
                << rightController.pose.qx << "," << rightController.pose.qy << "," << rightController.pose.qz << "," << rightController.pose.qw << ","
                << rightController.triggerValue << ","
                << (inputState.buttonA ? "1" : "0");
            return oss.str();
        }

        // JSON genérico
        std::string toJSON() const {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(6);
            oss << "{"
                << "\"timestamp\":" << timestamp << ","
                << "\"head\":{\"pos\":[" << headPose.x << "," << headPose.y << "," << headPose.z << "],"
                << "\"rot\":[" << headPose.qx << "," << headPose.qy << "," << headPose.qz << "," << headPose.qw << "]},"
                << "\"left\":{\"tracked\":" << (leftController.isTracked ? "true" : "false") << ","
                << "\"pos\":[" << leftController.pose.x << "," << leftController.pose.y << "," << leftController.pose.z << "],"
                << "\"rot\":[" << leftController.pose.qx << "," << leftController.pose.qy << "," << leftController.pose.qz << "," << leftController.pose.qw << "],"
                << "\"trigger\":" << leftController.triggerValue << "},"
                << "\"right\":{\"tracked\":" << (rightController.isTracked ? "true" : "false") << ","
                << "\"pos\":[" << rightController.pose.x << "," << rightController.pose.y << "," << rightController.pose.z << "],"
                << "\"rot\":[" << rightController.pose.qx << "," << rightController.pose.qy << "," << rightController.pose.qz << "," << rightController.pose.qw << "],"
                << "\"trigger\":" << rightController.triggerValue << "},"
                << "\"buttons\":{\"a\":" << (inputState.buttonA ? "true" : "false") << "}"
                << "}";
            return oss.str();
        }
    };

} // namespace VRTelemetry