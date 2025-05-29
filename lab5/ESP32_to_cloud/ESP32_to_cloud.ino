// ESP32 Board MAC Address: b0:81:84:04:9c:78
#include <techin515_inferencing.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// MPU6050 sensor
Adafruit_MPU6050 mpu;

// Sampling and capture variables
#define SAMPLE_RATE_MS 10  // 100Hz sampling rate (10ms between samples)
#define CAPTURE_DURATION_MS 1000  // 1 second capture
#define FEATURE_SIZE EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE  // Size of the feature array

#define BUTTON_PIN 3
#define RED_PIN 4
#define GREEN_PIN 5
#define BLUE_PIN 21

// Confidence threshold for local vs server inference
#define CONFIDENCE_THRESHOLD 80.0

// Capture state variables
bool capturing = false;
unsigned long last_sample_time = 0;
unsigned long capture_start_time = 0;
int sample_count = 0;
unsigned long led_on_time = 0;
bool led_active = false;

// WiFi credentials 
const char* ssid = "UW MPSK";
const char* password = "RKA?A3_a-N"; // use your password here
// Server details - CHANGE THIS TO YOUR WEB URL
const char* serverUrl = "http://10.18.145.154:8080/predict"; // Fill in with your server URL; Please keep /predict

// Student identifier - set this to your UWNetID
const char* studentId = "pengt906";

// Feature array to store accelerometer data
float features[FEATURE_SIZE];

/**
 * @brief      Copy raw feature data in out_ptr
 *             Function called by inference library
 *
 * @param[in]  offset   The offset
 * @param[in]  length   The length
 * @param      out_ptr  The out pointer
 *
 * @return     0
 */
int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

void print_inference_result(ei_impulse_result_t result);
void sendRawDataToServer();

/**
 * Setup WiFi connection
*/
 void setupWiFi() {
     Serial.println("Connecting to WiFi...");
     WiFi.begin(ssid, password);
     
     // Wait for connection
     while (WiFi.status() != WL_CONNECTED) {
         delay(500);
         Serial.print(".");
     }
     
     Serial.println("");
     Serial.print("Connected to ");
     Serial.println(ssid);
     Serial.print("IP address: ");
     Serial.println(WiFi.localIP());
 }

/**
 * @brief      Arduino setup function
 */
void setup()
{
    // Initialize serial
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP); // Button, active LOW
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);

    // Turn off LED initially
    setColor(0, 0, 0);

    // Initialize MPU6050
    Serial.println("Initializing MPU6050...");
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) {
            delay(10);
        }
    }
    
    // Configure MPU6050 - match settings with gesture_capture.ino
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    
    Serial.println("MPU6050 initialized successfully");
    Serial.println("Send 'o' to start gesture capture");
    setupWiFi();
}

/**
* @brief      Send result to server (original function for high confidence)
*/
void sendGestureToServer(const char* gesture, float confidence) {
   // Create JSON payload
   String jsonPayload = "{";
   jsonPayload += "\"student_id\":";
   jsonPayload += "\"";
   jsonPayload += studentId;
   jsonPayload += "\",";
   jsonPayload += "\"gesture\":";
   jsonPayload += "\"";
   jsonPayload += gesture;
   jsonPayload += "\",";
   jsonPayload += "\"confidence\":";
   jsonPayload += confidence;
   jsonPayload += "}";
   
   Serial.println("\n--- Sending Prediction to Server ---");
   Serial.println("URL: " + String(serverUrl));
   Serial.println("Payload: " + jsonPayload);
   
   HTTPClient http;
   http.begin(serverUrl);
   http.addHeader("Content-Type", "application/json");
   
   // Send POST request
   int httpResponseCode = http.POST(jsonPayload);
   
   Serial.print("HTTP Response code: ");
   Serial.println(httpResponseCode);
   
   if (httpResponseCode > 0) {
       String response = http.getString();
       Serial.println("Server response: " + response);
   } else {
       Serial.printf("Error sending POST: %s\n", http.errorToString(httpResponseCode).c_str());
   }
   
   http.end();
   Serial.println("--- End of Request ---\n");
}

/**
 * @brief      Send raw sensor data to server for remote inference
 */
void sendRawDataToServer() {
   HTTPClient http;
   http.begin(serverUrl);
   http.addHeader("Content-Type", "application/json");

   // Build JSON array from features[]
   String jsonPayload = "{";
   jsonPayload += "\"student_id\":\"" + String(studentId) + "\",";
   jsonPayload += "\"raw_data\":[";
   
   for (int i = 0; i < FEATURE_SIZE; i++) {
       jsonPayload += String(features[i], 6); // 6 decimal places for precision
       if (i < FEATURE_SIZE - 1) {
           jsonPayload += ",";
       }
   }
   
   jsonPayload += "]}";

   Serial.println("\n--- Sending Raw Data to Server ---");
   Serial.println("URL: " + String(serverUrl));
   Serial.println("Payload length: " + String(jsonPayload.length()));

   int httpResponseCode = http.POST(jsonPayload);
   Serial.print("HTTP Response code: ");
   Serial.println(httpResponseCode);

   if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Server response: " + response);

      // Parse the JSON response
      DynamicJsonDocument doc(256);
      DeserializationError error = deserializeJson(doc, response);
      if (!error) {
            const char* gesture = doc["gesture"];
            float confidence = doc["confidence"];

            Serial.println("Server Inference Result:");
            Serial.print("Gesture: ");
            Serial.println(gesture);
            Serial.print("Confidence: ");
            Serial.print(confidence);
            Serial.println("%");
            
            // Actuate LED based on server response
            String label = String(gesture);
            if (label == "Z") setColor(255, 0, 0);        // Red
            else if (label == "O") setColor(0, 255, 0);    // Green
            else if (label == "V") setColor(0, 0, 255);   // Blue
            else setColor(255, 255, 255); // Unknown → White
            
            led_on_time = millis();
            led_active = true;
            
      } else {
            Serial.print("Failed to parse server response: ");
            Serial.println(error.c_str());
            // Set LED to white for error
            setColor(255, 255, 255);
            led_on_time = millis();
            led_active = true;
      }

   } else {
      Serial.printf("Error sending POST: %s\n", http.errorToString(httpResponseCode).c_str());
      // Set LED to white for error
      setColor(255, 255, 255);
      led_on_time = millis();
      led_active = true;
   }

   http.end();
   Serial.println("--- End of Raw Data Request ---\n");
}

/**
 * @brief      Capture accelerometer data for inference
 */
void capture_accelerometer_data() {
    if (millis() - last_sample_time >= SAMPLE_RATE_MS) {
        last_sample_time = millis();
        
        // Get accelerometer data
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);
        
        // Store data in features array (x, y, z, x, y, z, ...)
        if (sample_count < FEATURE_SIZE / 3) {
            int idx = sample_count * 3;
            features[idx] = a.acceleration.x;
            features[idx + 1] = a.acceleration.y;
            features[idx + 2] = a.acceleration.z;
            sample_count++;
        }
        
        // Check if capture duration has elapsed
        if (millis() - capture_start_time >= CAPTURE_DURATION_MS) {
            capturing = false;
            Serial.println("Capture complete");
            
            // Run inference on captured data
            run_inference();
        }
    }
}

/**
 * @brief      Run inference on the captured data
 */
void run_inference() {
    // Check if we have enough data
    if (sample_count * 3 < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        Serial.println("ERROR: Not enough data for inference");
        return;
    }
    
    ei_impulse_result_t result = { 0 };

    // Create signal from features array
    signal_t features_signal;
    features_signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    features_signal.get_data = &raw_feature_get_data;

    // Run the classifier
    EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false /* debug */);
    if (res != EI_IMPULSE_OK) {
        Serial.print("ERR: Failed to run classifier (");
        Serial.print(res);
        Serial.println(")");
        return;
    }

    // Print inference result
    print_inference_result(result);
}

/**
 * @brief      Arduino main function
 */
void loop() {
  // If button is pressed (LOW)
  if (digitalRead(BUTTON_PIN) == LOW && !capturing) {
    delay(50); // debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Button pressed: Starting gesture capture...");
      sample_count = 0;
      capturing = true;
      capture_start_time = millis();
      last_sample_time = millis();
    }
  }

  // Turn off LED after 3 seconds
  if (led_active && (millis() - led_on_time >= 3000)) {
    setColor(0, 0, 0);  // Turn off LED
    led_active = false;
  }

  // Capture data if in capturing mode
  if (capturing) {
    capture_accelerometer_data();
  }
}

void print_inference_result(ei_impulse_result_t result) {
  float max_value = 0;
  int max_index = -1;

  for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    if (result.classification[i].value > max_value) {
      max_value = result.classification[i].value;
      max_index = i;
    }
  }

  if (max_index != -1) {
    Serial.print("Local Prediction: ");
    Serial.print(ei_classifier_inferencing_categories[max_index]);
    Serial.print(" (");
    Serial.print(max_value * 100);
    Serial.println("%)");

    float confidence = max_value * 100;
    String label = ei_classifier_inferencing_categories[max_index];

    // Check confidence threshold
    if (confidence < CONFIDENCE_THRESHOLD) {
        Serial.println("Low confidence - sending raw data to server...");
        sendRawDataToServer();
    } else {
        Serial.println("High confidence - using local inference result");
        
        // Set color based on gesture label (local inference)
        if (label == "Z") setColor(255, 0, 0);        // Red
        else if (label == "O") setColor(0, 255, 0);    // Green
        else if (label == "V") setColor(0, 0, 255);   // Blue  
        else setColor(255, 255, 255); // Unknown → White

        // Send result to server for logging (optional)
        sendGestureToServer(label.c_str(), max_value);
        
        led_on_time = millis();
        led_active = true;
    }
  }
}

void setColor(int r, int g, int b) {
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
}