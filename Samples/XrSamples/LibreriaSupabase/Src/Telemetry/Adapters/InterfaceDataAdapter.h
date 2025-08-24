#pragma once

#include "../VRTypes.h"

namespace VRTelemetry {

    // Interfaz simple para convertir datos de cualquier SDK VR a nuestro formato genérico
    class InterfaceDataAdapter {
    public:
        virtual ~InterfaceDataAdapter() = default;

        // Método principal: convierte datos específicos del SDK a formato genérico
        // Cada SDK implementará este método de forma diferente
        virtual VRFrameData convertToGeneric(double timestamp) = 0;

        // Información del adapter
        virtual std::string getAdapterName() const = 0;
        virtual std::string getSDKVersion() const = 0;
    };

} // namespace VRTelemetry