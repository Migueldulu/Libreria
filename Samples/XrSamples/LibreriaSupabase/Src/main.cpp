
#include <string>
#include <string_view>
#include <fstream>
#include <chrono>
#include <vector>
#include <sstream>
#include <iomanip>

#include <openxr/openxr.h>
#include "GUI/VRMenuObject.h"
#include "Render/BitmapFont.h"
#include "XrApp.h"
#include "OVR_Math.h"
#include "Input/ControllerRenderer.h"
#include "Input/TinyUI.h"
#include "Render/SimpleBeamRenderer.h"

// NUEVO: Includes para telemetría genérica
#include "Telemetry/TelemetryManager.h"
#include "Telemetry/Adapters/OpenXRAdapter.h"
#ifdef ANDROID
#include "Telemetry/AndroidUploader.h"
#endif

class XrAppBaseApp : public OVRFW::XrApp {
private:
    OVRFW::VRMenuObject* holaMundoLabel;
    OVRFW::VRMenuObject* toggleButton;
    OVRFW::VRMenuObject* recordingStatusLabel;
    bool debeReposicionar = false;
    bool labelCreado = false;
    bool labelVisible = true;

    // NUEVO: Telemetría genérica + adapter para OpenXR
    VRTelemetry::TelemetryManager telemetryManager;
    VRTelemetry::OpenXRAdapter openXRAdapter;
    std::chrono::high_resolution_clock::time_point startTime;

public:
    XrAppBaseApp() : OVRFW::XrApp() {
        BackgroundColor = OVR::Vector4f(0.55f, 0.35f, 0.1f, 1.0f);
    }

    virtual std::vector<const char*> GetExtensions() override {
        std::vector<const char*> extensions = XrApp::GetExtensions();
        return extensions;
    }

    virtual bool AppInit(const xrJava* context) override {
        if (false == ui_.Init(context, GetFileSys())) {
            ALOG("TinyUI::Init FAILED.");
            return false;
        }

        // NUEVO: Inicializar telemetría genérica
#ifdef ANDROID
        auto androidUploader = std::make_unique<VRTelemetry::AndroidUploader>();

            if (context && context->Vm && context->ActivityObject) {
                androidUploader->setJavaContext(context->Vm, context->ActivityObject);
            }

            VRTelemetry::TelemetryConfig config;
            config.enableLocalBackup = true;
            config.enableCloudUpload = true;

            telemetryManager.initialize(std::move(androidUploader), config);
#endif

        // Inicializar tiempo de inicio para timestamps
        startTime = std::chrono::high_resolution_clock::now();

        ALOG("VR Motion Recording started with generic adapter pattern");
        return true;
    }

    virtual bool SessionInit() override {
        if (!controllerRenderL_.Init(true)) {
            ALOG("SessionInit::Init L controller renderer FAILED.");
            return false;
        }
        if (!controllerRenderR_.Init(false)) {
            ALOG("SessionInit::Init R controller renderer FAILED.");
            return false;
        }
        cursorBeamRenderer_.Init(GetFileSys(), nullptr, OVR::Vector4f(1.0f), 1.0f);
        return true;
    }

    virtual void Update(const OVRFW::ovrApplFrameIn& in) override {
        // NUEVO: Conversión OpenXR → Genérico → Telemetría (¡3 líneas!)
        openXRAdapter.updateFrame(in);
        auto now = std::chrono::high_resolution_clock::now();
        double timestamp = std::chrono::duration<double>(now - startTime).count();
        VRTelemetry::VRFrameData genericData = openXRAdapter.convertToGeneric(timestamp);
        telemetryManager.recordFrame(genericData);

        // Resto del código se mantiene exactamente igual...
        if(!labelCreado){
            OVR::Matrix4f matrizInicial = OVR::Matrix4f(in.HeadPose);
            OVR::Vector3f posiInicial = matrizInicial.Transform({0.0f, -0.35f, -2.0f});
            holaMundoLabel = ui_.AddLabel("Super Hola Mundo", posiInicial, {400.0f, 100.0f});
            holaMundoLabel->SetLocalRotation(in.HeadPose.Rotation);

            OVR::Vector3f posicionBoton = matrizInicial.Transform({-0.75f, -0.1f, -2.0f});
            toggleButton = ui_.AddButton(
                    "Ocultar Texto",
                    posicionBoton,
                    {200.0f, 75.0f},
                    [this]() {
                        this->ToggleTextoVisibilidad();
                    }
            );
            toggleButton->SetLocalRotation(in.HeadPose.Rotation);

            OVR::Vector3f posicionEstado = matrizInicial.Transform({0.0f, 0.15f, -2.0f});
            recordingStatusLabel = ui_.AddLabel("GRABANDO (GENÉRICO)", posicionEstado, {350.0f, 60.0f});
            recordingStatusLabel->SetLocalRotation(in.HeadPose.Rotation);
            recordingStatusLabel->SetTextColor(OVR::Vector4f(0.2f, 1.0f, 0.2f, 1.0f)); // Verde para genérico

            labelCreado = true;
        }

        if (recordingStatusLabel && telemetryManager.isReady()) {
            std::ostringstream statusText;
            statusText << "GENÉRICO: " << telemetryManager.getTotalFrames()
                       << " | " << openXRAdapter.getAdapterName()
                       << " | Sesión: " << telemetryManager.getSessionId().substr(0, 8) << "...";
            recordingStatusLabel->SetText(statusText.str().c_str());
        }

        if (in.Clicked(OVRFW::ovrApplFrameIn::kButtonA)) {
            debeReposicionar = true;
            holaMundoLabel->SetTextColor(OVR::Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
        } else {
            holaMundoLabel->SetTextColor(OVR::Vector4f(1.0f, 1.0f, 1.0f, 1.0f));
        }

        if (debeReposicionar) {
            ReposicionarElementos(in.HeadPose);
            debeReposicionar = false;
        }

        ui_.HitTestDevices().clear();

        if (in.LeftRemoteTracked) {
            controllerRenderL_.Update(in.LeftRemotePose);
            const bool didTrigger = in.LeftRemoteIndexTrigger > 0.5f;
            ui_.AddHitTestRay(in.LeftRemotePointPose, didTrigger);
        }

        if (in.RightRemoteTracked) {
            controllerRenderR_.Update(in.RightRemotePose);
            const bool didTrigger = in.RightRemoteIndexTrigger > 0.5f;
            ui_.AddHitTestRay(in.RightRemotePointPose, didTrigger);
        }

        ui_.Update(in);
        cursorBeamRenderer_.Update(in, ui_.HitTestDevices());
    }

    virtual void Render(const OVRFW::ovrApplFrameIn& in, OVRFW::ovrRendererOutput& out) override {
        ui_.Render(in, out);

        if (in.LeftRemoteTracked) {
            controllerRenderL_.Render(out.Surfaces);
        }
        if (in.RightRemoteTracked) {
            controllerRenderR_.Render(out.Surfaces);
        }

        cursorBeamRenderer_.Render(in, out);
    }

    virtual void SessionEnd() override {
        controllerRenderL_.Shutdown();
        controllerRenderR_.Shutdown();
        cursorBeamRenderer_.Shutdown();
    }

    virtual void AppShutdown(const xrJava* context) override {
        telemetryManager.shutdown();
        OVRFW::XrApp::AppShutdown(context);
        ui_.Shutdown();
    }

private:
    OVRFW::ControllerRenderer controllerRenderL_;
    OVRFW::ControllerRenderer controllerRenderR_;
    OVRFW::TinyUI ui_;
    OVRFW::SimpleBeamRenderer cursorBeamRenderer_;

    void ToggleTextoVisibilidad() {
        if (holaMundoLabel != nullptr) {
            labelVisible = !labelVisible;
            holaMundoLabel->SetVisible(labelVisible);

            if (labelVisible) {
                toggleButton->SetText("Ocultar Texto");
            } else {
                toggleButton->SetText("Mostrar Texto");
            }
        }
        ALOG("ToggleTextoVisibilidad llamado - labelVisible: %s",
             labelVisible ? "true" : "false");
    }

    void ReposicionarElementos(const OVR::Posef& headPose) {
        if (holaMundoLabel && toggleButton && recordingStatusLabel) {
            ui_.RemoveParentMenu(recordingStatusLabel);
            ui_.RemoveParentMenu(toggleButton);
            ui_.RemoveParentMenu(holaMundoLabel);
            recordingStatusLabel = nullptr;
            toggleButton = nullptr;
            holaMundoLabel = nullptr;

            OVR::Matrix4f matrizNuevaCabeza = OVR::Matrix4f(headPose);

            OVR::Vector3f nuevaPosi = matrizNuevaCabeza.Transform({0.0f, -0.35f, -2.0f});
            holaMundoLabel = ui_.AddLabel("Super Hola Mundo", nuevaPosi, {400.0f, 100.0f});
            holaMundoLabel->SetLocalRotation(headPose.Rotation);
            holaMundoLabel->SetVisible(labelVisible);

            OVR::Vector3f posicionBoton = matrizNuevaCabeza.Transform({-0.75f, -0.1f, -2.0f});
            toggleButton = ui_.AddButton(
                    labelVisible ? "Ocultar Texto" : "Mostrar Texto",
                    posicionBoton,
                    {200.0f, 75.0f},
                    [this]() {
                        this->ToggleTextoVisibilidad();
                    }
            );
            toggleButton->SetLocalRotation(headPose.Rotation);

            OVR::Vector3f posicionEstado = matrizNuevaCabeza.Transform({0.0f, 0.15f, -2.0f});
            recordingStatusLabel = ui_.AddLabel("GRABANDO (GENÉRICO)", posicionEstado, {350.0f, 60.0f});
            recordingStatusLabel->SetLocalRotation(headPose.Rotation);
            recordingStatusLabel->SetTextColor(OVR::Vector4f(0.2f, 1.0f, 0.2f, 1.0f)); // Verde para genérico
        }
    }
};

ENTRY_POINT(XrAppBaseApp)