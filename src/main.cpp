#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>
#include <esp_sleep.h> // Deep sleep library

// WiFi & Firebase Credentials
#define WIFI_SSID "UW MPSK"
#define WIFI_PASSWORD "s_rMSGM.w7"
#define DATABASE_SECRET "HI0DdDYLVadO3nGIf4hN28KTpRQY4cC3bX7gThOi"
#define DATABASE_URL "https://jaz-demo-default-rtdb.firebaseio.com/"

// Ultrasonic Sensor Pins (XIAO ESP32C3)
const int trigPin = D0;  
const int echoPin = D1; 

// Power-saving configuration
#define DISTANCE_THRESHOLD 50   // cm (Trigger sensor if within the distance)
#define NO_MOVEMENT_THRESHOLD 30000  // 30s no movement_start deepsleep
#define SHORT_SLEEP_DURATION 30000000  // 30s deepsleep(microseconds)
#define DATA_SEND_INTERVAL 2000 // Transmit data every 2 seconds

WiFiClientSecure ssl;
DefaultNetwork network;
AsyncClientClass client(ssl, getNetwork(network));
FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result;
LegacyToken dbSecret(DATABASE_SECRET);

unsigned long lastMovementTime = 0;
unsigned long lastUploadTime = 0;
float lastMeasuredDistance = -1;
bool objectDetected = false;

// Function Prototypes
float measureDistance();
void connectToWiFi();
void initFirebase();
void sendDataToFirebase(float distance);
void enterShortSleep();

void setup() {
    Serial.begin(115200);
    delay(500);
    
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    Serial.println("Device waking up...");

    float distance = measureDistance();

    if (distance < DISTANCE_THRESHOLD) {
        objectDetected = true;
        lastMovementTime = millis();
        lastMeasuredDistance = distance;
        Serial.println("Object detected! Initializing WiFi and Firebase...");
        connectToWiFi();
        initFirebase();
    } else {
        Serial.println("No object detected. Entering short sleep...");
        enterShortSleep();
    }
}

void loop() {
    float distance = measureDistance();
    Serial.print("Measured Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    if (distance < DISTANCE_THRESHOLD) {
        if (abs(distance - lastMeasuredDistance) > 2) { // movement detected
            lastMovementTime = millis();  
            lastMeasuredDistance = distance;
            Serial.println("Object moved! Resetting sleep timers.");
        }

        // update data every 2 seconds
        if (millis() - lastUploadTime > DATA_SEND_INTERVAL) {
            sendDataToFirebase(distance);
            lastUploadTime = millis();
        }
    } 
    else {
        Serial.println("Object moved out of range (> 50cm). Entering short sleep...");
        enterShortSleep();
    }

    // if no movement for 30 seconds, enter deepsleep mode
    if (millis() - lastMovementTime > NO_MOVEMENT_THRESHOLD) {
        Serial.println("No movement for 30 seconds. Entering short sleep...");
        enterShortSleep();
    }

    app.loop(); // keep Firebase running
}

// Function: Measure Distance Using HC-SR04
float measureDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH, 30000); // Timeout at 30ms
    float distance = duration * 0.0343 / 2;
    return distance;
}

// Function: Connect to WiFi
void connectToWiFi() {
    Serial.println(WiFi.macAddress());
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    ssl.setInsecure();

    Serial.println("Connecting to WiFi...");
    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 10) {
        delay(1000);
        Serial.print(".");
        attempt++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nWiFi Connection Failed. Restarting...");
        ESP.restart();
    } else {
        Serial.println("\nConnected to WiFi!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    }
}

// Function: Initialize Firebase
void initFirebase() {
    Serial.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);
    ssl.setInsecure();
    initializeApp(client, app, getAuth(dbSecret));
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);
    client.setAsyncResult(result);
}

// Function: Send Data to Firebase
void sendDataToFirebase(float distance) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("No WiFi. Cannot send data.");
        return;
    }

    Serial.print("Sending data to Firebase... ");
    String path = "/test/distance";
    String pushID = Database.push<number_t>(client, path.c_str(), number_t(distance));

    if (client.lastError().code() == 0) {
        Serial.printf("Data sent successfully, ID: %s\n", pushID.c_str());
    } else {
        Serial.printf("Firebase Error: %s (Code %d)\n", client.lastError().message().c_str(), client.lastError().code());
    }
}

// Function: Enter Short Sleep
void enterShortSleep() {
    Serial.println("Entering short sleep for 30 seconds...");
    delay(1000); 
    WiFi.disconnect();
    esp_sleep_enable_timer_wakeup(SHORT_SLEEP_DURATION);
    esp_deep_sleep_start();
}