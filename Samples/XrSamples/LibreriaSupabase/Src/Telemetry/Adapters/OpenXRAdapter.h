#pragma once

#include "InterfaceDataAdapter.h"
#include "XrApp.h" // Solo aquí importamos OpenXR, no en el resto de telemetría

namespace VRTelemetry {

    // Adapter específico para OpenXR - convierte de OpenXR a nuestro formato genérico
    class OpenXRAdapter : public InterfaceDataAdapter {
    private:
        const OVRFW::ovrApplFrameIn* currentFrame;

    public:
        OpenXRAdapter() : currentFrame(nullptr) {}

        // Actualizar con el frame actual de OpenXR
        void updateFrame(const OVRFW::ovrApplFrameIn& frame) {
            currentFrame = &frame;
        }

        // Implementación del método virtual: convertir OpenXR → genérico
        VRFrameData convertToGeneric(double timestamp) override {
            VRFrameData data;
            data.timestamp = timestamp;

            if (!currentFrame) {
                return data; // Retorna datos vacíos si no hay frame
            }

            // Convertir headset
            data.headPose.x = currentFrame->HeadPose.Translation.x;
            data.headPose.y = currentFrame->HeadPose.Translation.y;
            data.headPose.z = currentFrame->HeadPose.Translation.z;
            data.headPose.qx = currentFrame->HeadPose.Rotation.x;
            data.headPose.qy = currentFrame->HeadPose.Rotation.y;
            data.headPose.qz = currentFrame->HeadPose.Rotation.z;
            data.headPose.qw = currentFrame->HeadPose.Rotation.w;

            // Convertir controlador izquierdo
            data.leftController.isTracked = currentFrame->LeftRemoteTracked;
            if (data.leftController.isTracked) {
                data.leftController.pose.x = currentFrame->LeftRemotePose.Translation.x;
                data.leftController.pose.y = currentFrame->LeftRemotePose.Translation.y;
                data.leftController.pose.z = currentFrame->LeftRemotePose.Translation.z;
                data.leftController.pose.qx = currentFrame->LeftRemotePose.Rotation.x;
                data.leftController.pose.qy = currentFrame->LeftRemotePose.Rotation.y;
                data.leftController.pose.qz = currentFrame->LeftRemotePose.Rotation.z;
                data.leftController.pose.qw = currentFrame->LeftRemotePose.Rotation.w;
                data.leftController.triggerValue = currentFrame->LeftRemoteIndexTrigger;
            }

            // Convertir controlador derecho
            data.rightController.isTracked = currentFrame->RightRemoteTracked;
            if (data.rightController.isTracked) {
                data.rightController.pose.x = currentFrame->RightRemotePose.Translation.x;
                data.rightController.pose.y = currentFrame->RightRemotePose.Translation.y;
                data.rightController.pose.z = currentFrame->RightRemotePose.Translation.z;
                data.rightController.pose.qx = currentFrame->RightRemotePose.Rotation.x;
                data.rightController.pose.qy = currentFrame->RightRemotePose.Rotation.y;
                data.rightController.pose.qz = currentFrame->RightRemotePose.Rotation.z;
                data.rightController.pose.qw = currentFrame->RightRemotePose.Rotation.w;
                data.rightController.triggerValue = currentFrame->RightRemoteIndexTrigger;
            }

            // Convertir botones
            data.inputState.buttonA = currentFrame->Clicked(OVRFW::ovrApplFrameIn::kButtonA);
            // Fácil agregar más botones aquí en el futuro

            return data;
        }

        // Información del adapter
        std::string getAdapterName() const override {
            return "OpenXR Adapter";
        }

        std::string getSDKVersion() const override {
            return "OpenXR 1.0 + Meta SDK";
        }
    };

} // namespace VRTelemetry