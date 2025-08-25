package io.github.migueldulu.LibreriaSupabase;

import android.util.Log;
import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class HttpHelper {
    private static final String TAG = "HttpHelper";
    private static final ExecutorService executor = Executors.newCachedThreadPool();

    public static boolean makeRequest(String urlString, String method, String jsonData, String apiKey) {
        try {
            // Ejecutar en hilo separado para evitar NetworkOnMainThreadException
            CompletableFuture<Boolean> future = CompletableFuture.supplyAsync(() -> {
                return makeRequestSync(urlString, method, jsonData, apiKey);
            }, executor);

            // Esperar máximo 30 segundos
            return future.get(30, java.util.concurrent.TimeUnit.SECONDS);

        } catch (Exception e) {
            Log.e(TAG, "Error in async request: " + e.getMessage(), e);
            return false;
        }
    }

    private static boolean makeRequestSync(String urlString, String method, String jsonData, String apiKey) {
        HttpURLConnection connection = null;
        try {
            Log.d(TAG, "Making " + method + " request to: " + urlString);

            URL url = new URL(urlString);
            connection = (HttpURLConnection) url.openConnection();

            // Configurar la conexión
            connection.setRequestMethod(method);
            connection.setRequestProperty("Content-Type", "application/json");

            // CORREGIDO: Supabase necesita AMBOS headers
            connection.setRequestProperty("Authorization", "Bearer " + apiKey);
            connection.setRequestProperty("apikey", apiKey);  // ← NUEVO: Header adicional requerido

            connection.setRequestProperty("Prefer", "return=representation");
            connection.setConnectTimeout(15000); // 15 segundos
            connection.setReadTimeout(30000);    // 30 segundos
            connection.setDoOutput(true);
            connection.setDoInput(true);

            // NUEVO: Log de headers para debug
            Log.d(TAG, "Headers set:");
            Log.d(TAG, "  Content-Type: application/json");
            Log.d(TAG, "  Authorization: Bearer " + apiKey.substring(0, Math.min(10, apiKey.length())) + "...");
            Log.d(TAG, "  apikey: " + apiKey.substring(0, Math.min(10, apiKey.length())) + "...");

            // Enviar datos JSON
            if (jsonData != null && !jsonData.isEmpty()) {
                Log.d(TAG, "Sending JSON data (length: " + jsonData.length() + ")");
                Log.d(TAG, "JSON preview: " + jsonData.substring(0, Math.min(200, jsonData.length())) + "...");

                try (DataOutputStream outputStream = new DataOutputStream(connection.getOutputStream())) {
                    outputStream.writeBytes(jsonData);
                    outputStream.flush();
                }
            }

            // Obtener respuesta
            int responseCode = connection.getResponseCode();
            Log.d(TAG, "Response code: " + responseCode);

            // Leer respuesta
            BufferedReader reader;
            if (responseCode >= 200 && responseCode < 300) {
                reader = new BufferedReader(new InputStreamReader(connection.getInputStream()));
            } else {
                reader = new BufferedReader(new InputStreamReader(connection.getErrorStream()));
            }

            StringBuilder response = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) {
                response.append(line);
            }
            reader.close();

            String responseBody = response.toString();
            Log.d(TAG, "Response: " + responseBody);

            // Considerar exitoso si el código está en el rango 200-299
            boolean success = responseCode >= 200 && responseCode < 300;

            if (success) {
                Log.i(TAG, "Request successful: " + responseCode);
            } else {
                Log.w(TAG, "Request failed: " + responseCode + " - " + responseBody);

                // NUEVO: Análisis específico de errores de Supabase
                if (responseCode == 401) {
                    if (responseBody.contains("No API key found")) {
                        Log.e(TAG, "SUPABASE ERROR: API key not found in headers");
                    } else if (responseBody.contains("Invalid API key")) {
                        Log.e(TAG, "SUPABASE ERROR: API key is invalid");
                    }
                } else if (responseCode == 400) {
                    Log.e(TAG, "SUPABASE ERROR: Bad request - check JSON format");
                } else if (responseCode == 404) {
                    Log.e(TAG, "SUPABASE ERROR: Table or endpoint not found");
                }
            }

            return success;

        } catch (Exception e) {
            Log.e(TAG, "Exception in HTTP request: " + e.getMessage(), e);
            return false;
        } finally {
            if (connection != null) {
                connection.disconnect();
            }
        }
    }

    // Método para limpiar recursos cuando la app se cierre
    public static void shutdown() {
        executor.shutdown();
    }
}