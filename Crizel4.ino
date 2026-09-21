#define ENABLE_USER_AUTH
#define ENABLE_DATABASE


#include <Arduino.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <FirebaseClient.h>

#include <LittleFS.h>

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

#include <DHT.h>

#include <time.h>


// =====================================================
// DHT11
// =====================================================

#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(
    DHTPIN,
    DHTTYPE
);


// =====================================================
// FIREBASE
// =====================================================

#define API_KEY \
    "AIzaSyBAQ2Bk6LkPd0fwuRqgUcE_TeFAp0d-vGs"

#define DATABASE_URL \
    "https://crizel-e1e5c-default-rtdb.europe-west1.firebasedatabase.app"


// =====================================================
// FIREBASE AUTH
// =====================================================

// Use the Firebase Authentication account
// authorized for this ESP32.
//
// IMPORTANT:
// Replace these two values with your own credentials.

#define USER_EMAIL \
    "carismacrizeljean83@gmail.com"

#define USER_PASSWORD \
    "bustillo011226"


// =====================================================
// FIREBASE OBJECTS
// =====================================================

UserAuth user_auth(
    API_KEY,
    USER_EMAIL,
    USER_PASSWORD,
    3000
);

FirebaseApp app;

WiFiClientSecure ssl_client;

AsyncClientClass aClient(
    ssl_client
);

RealtimeDatabase Database;


// =====================================================
// WEB SERVER
// =====================================================

AsyncWebServer server(80);


// =====================================================
// WIFI MANAGER
// =====================================================

const char *AP_SSID =
    "ESP-WIFI-MANAGER";

bool wifiManagerMode =
    false;


// =====================================================
// SENSOR TIMER
// =====================================================

unsigned long lastSensorRead =
    0;

const unsigned long SENSOR_INTERVAL =
    10000;


// =====================================================
// FUNCTION DECLARATIONS
// =====================================================

void processFirebase(
    AsyncResult &aResult
);

bool connectToSavedWiFi();

void startWiFiManager();

void startMainWebServer();

void setupFirebase();

void sendSensorData();

String readFile(
    const char *path
);

bool writeFile(
    const char *path,
    const String &data
);

void deleteWiFiFiles();

String getDateString();

String getTimeString();


// =====================================================
// CRIZEL WIFI MANAGER HTML
// =====================================================

const char WIFI_MANAGER_HTML[] PROGMEM = R"rawliteral(

<!DOCTYPE html>

<html lang="en">

<head>

<meta charset="UTF-8">

<meta
    name="viewport"
    content="width=device-width, initial-scale=1.0"
>

<title>Crizel WiFi Manager</title>


<style>

* {
    box-sizing: border-box;
}


body {

    margin: 0;

    min-height: 100vh;

    display: flex;

    justify-content: center;

    align-items: center;

    padding: 20px;

    font-family:
        "Segoe UI",
        Arial,
        sans-serif;

    background: #eeeeee;

    color: #493842;
}


.container {

    width: 100%;

    max-width: 430px;

    background: #ffffff;

    padding: 30px;

    border-radius: 20px;

    border:
        1px solid #f0dce5;

    box-shadow:
        0 15px 40px
        rgba(128, 74, 100, 0.12);
}


.brand {

    text-align: center;

    margin-bottom: 25px;
}


.brand-icon {

    width: 55px;

    height: 55px;

    margin:
        0 auto 12px;

    border-radius: 17px;

    background: #f8d9e7;

    color: #b34d78;

    display: flex;

    align-items: center;

    justify-content: center;

    font-size: 28px;
}


h1 {

    margin: 0;

    font-size: 25px;

    color: #4c3943;
}


.subtitle {

    margin-top: 7px;

    font-size: 13px;

    color: #9b818d;

    line-height: 1.5;
}


label {

    display: block;

    margin-top: 17px;

    margin-bottom: 7px;

    font-size: 12px;

    font-weight: 700;

    color: #6d515e;
}


input {

    width: 100%;

    height: 44px;

    padding:
        0 13px;

    border:
        1px solid #e8cbd8;

    border-radius: 11px;

    font-size: 14px;

    color: #513d47;

    background: #fffafd;

    outline: none;
}


input:focus {

    border-color: #c9799b;

    box-shadow:
        0 0 0 3px
        rgba(201,121,155,.10);
}


button {

    width: 100%;

    height: 45px;

    margin-top: 25px;

    border: none;

    border-radius: 11px;

    background: #b85f83;

    color: white;

    font-size: 14px;

    font-weight: 700;

    cursor: pointer;
}


button:hover {

    background: #a94f75;
}


.note {

    margin-top: 18px;

    text-align: center;

    font-size: 12px;

    color: #9b8790;

    line-height: 1.6;
}

</style>

</head>


<body>


<div class="container">


    <div class="brand">

        <div class="brand-icon">
            ♡
        </div>

        <h1>
            Crizel WiFi Manager
        </h1>

        <div class="subtitle">
            Configure the WiFi connection
            for your ESP32
        </div>

    </div>


    <form
        method="POST"
        action="/"
    >


        <label for="ssid">
            WiFi Name (SSID)
        </label>

        <input
            type="text"
            id="ssid"
            name="ssid"
            placeholder="Enter WiFi name"
            required
        >


        <label for="pass">
            WiFi Password
        </label>

        <input
            type="password"
            id="pass"
            name="pass"
            placeholder="Enter WiFi password"
        >


        <label for="ip">
            Static IP
            <span style="font-weight:normal;">
                (optional)
            </span>
        </label>

        <input
            type="text"
            id="ip"
            name="ip"
            placeholder="Example: 192.168.1.50"
        >


        <label for="gateway">
            Gateway
            <span style="font-weight:normal;">
                (optional)
            </span>
        </label>

        <input
            type="text"
            id="gateway"
            name="gateway"
            placeholder="Example: 192.168.1.1"
        >


        <button type="submit">
            Save WiFi Settings
        </button>


    </form>


    <div class="note">

        After saving, the ESP32 will restart
        and automatically connect to the
        selected WiFi network.

    </div>


</div>


</body>

</html>

)rawliteral";


// =====================================================
// READ FILE
// =====================================================

String readFile(
    const char *path
)
{
    if (!LittleFS.exists(path))
    {
        return "";
    }


    File file =
        LittleFS.open(
            path,
            "r"
        );


    if (!file)
    {
        return "";
    }


    String data =
        file.readString();


    file.close();

    data.trim();

    return data;
}


// =====================================================
// WRITE FILE
// =====================================================

bool writeFile(
    const char *path,
    const String &data
)
{
    File file =
        LittleFS.open(
            path,
            "w"
        );


    if (!file)
    {
        Serial.print(
            "Failed to open file: "
        );

        Serial.println(
            path
        );

        return false;
    }


    file.print(data);

    file.close();

    return true;
}


// =====================================================
// DELETE WIFI FILES
// =====================================================

void deleteWiFiFiles()
{
    LittleFS.remove(
        "/ssid.txt"
    );

    LittleFS.remove(
        "/pass.txt"
    );

    LittleFS.remove(
        "/ip.txt"
    );

    LittleFS.remove(
        "/gateway.txt"
    );


    Serial.println(
        "WiFi settings deleted."
    );
}


// =====================================================
// CONNECT SAVED WIFI
// =====================================================

bool connectToSavedWiFi()
{
    String ssid =
        readFile(
            "/ssid.txt"
        );

    String pass =
        readFile(
            "/pass.txt"
        );

    String ip =
        readFile(
            "/ip.txt"
        );

    String gateway =
        readFile(
            "/gateway.txt"
        );


    if (ssid.length() == 0)
    {
        Serial.println();
        Serial.println(
            "No saved WiFi credentials."
        );

        return false;
    }


    Serial.println();
    Serial.println(
        "================================="
    );

    Serial.println(
        "       SAVED WIFI FOUND"
    );

    Serial.println(
        "================================="
    );


    Serial.print(
        "SSID: "
    );

    Serial.println(
        ssid
    );


    WiFi.mode(
        WIFI_STA
    );


    delay(500);


    // -------------------------------------------------
    // OPTIONAL STATIC IP
    // -------------------------------------------------

    if (
        ip.length() > 0 &&
        gateway.length() > 0
    )
    {
        IPAddress local_IP;

        IPAddress gateway_IP;


        if (
            local_IP.fromString(ip) &&
            gateway_IP.fromString(gateway)
        )
        {
            IPAddress subnet(
                255,
                255,
                255,
                0
            );


            if (
                WiFi.config(
                    local_IP,
                    gateway_IP,
                    subnet
                )
            )
            {
                Serial.println(
                    "Static IP configured."
                );
            }
            else
            {
                Serial.println(
                    "Static IP configuration failed."
                );
            }
        }
    }


    WiFi.begin(
        ssid.c_str(),
        pass.c_str()
    );


    Serial.print(
        "Connecting to WiFi"
    );


    unsigned long startTime =
        millis();


    while (
        WiFi.status() != WL_CONNECTED &&
        millis() - startTime < 20000
    )
    {
        Serial.print(".");

        delay(500);
    }


    Serial.println();


    if (
        WiFi.status() ==
        WL_CONNECTED
    )
    {
        Serial.println();
        Serial.println(
            "================================="
        );

        Serial.println(
            "       WIFI CONNECTED"
        );

        Serial.println(
            "================================="
        );


        Serial.print(
            "SSID: "
        );

        Serial.println(
            WiFi.SSID()
        );


        Serial.print(
            "IP Address: "
        );

        Serial.println(
            WiFi.localIP()
        );


        Serial.print(
            "Gateway: "
        );

        Serial.println(
            WiFi.gatewayIP()
        );


        Serial.println();

        return true;
    }


    Serial.println(
        "Failed to connect to saved WiFi."
    );


    WiFi.disconnect(
        true
    );


    delay(1000);


    return false;
}


// =====================================================
// START WIFI MANAGER
// =====================================================

void startWiFiManager()
{
    wifiManagerMode =
        true;


    Serial.println();
    Serial.println(
        "================================="
    );

    Serial.println(
        "       WIFI MANAGER MODE"
    );

    Serial.println(
        "================================="
    );


    WiFi.mode(
        WIFI_AP
    );


    delay(500);


    bool apStarted =
        WiFi.softAP(
            AP_SSID
        );


    if (!apStarted)
    {
        Serial.println(
            "ERROR: Failed to start WiFi Manager AP!"
        );
    }


    delay(1000);


    Serial.print(
        "AP SSID: "
    );

    Serial.println(
        AP_SSID
    );


    Serial.print(
        "AP IP Address: "
    );

    Serial.println(
        WiFi.softAPIP()
    );


    // -------------------------------------------------
    // WIFI MANAGER PAGE
    // -------------------------------------------------

    server.on(
        "/",
        HTTP_GET,
        [](AsyncWebServerRequest *request)
        {
            request->send(
                200,
                "text/html",
                WIFI_MANAGER_HTML
            );
        }
    );


    // -------------------------------------------------
    // SAVE WIFI
    // -------------------------------------------------

    server.on(
        "/",
        HTTP_POST,
        [](AsyncWebServerRequest *request)
        {
            String ssid =
                "";

            String pass =
                "";

            String ip =
                "";

            String gateway =
                "";


            if (
                request->hasParam(
                    "ssid",
                    true
                )
            )
            {
                ssid =
                    request
                    ->getParam(
                        "ssid",
                        true
                    )
                    ->value();
            }


            if (
                request->hasParam(
                    "pass",
                    true
                )
            )
            {
                pass =
                    request
                    ->getParam(
                        "pass",
                        true
                    )
                    ->value();
            }


            if (
                request->hasParam(
                    "ip",
                    true
                )
            )
            {
                ip =
                    request
                    ->getParam(
                        "ip",
                        true
                    )
                    ->value();
            }


            if (
                request->hasParam(
                    "gateway",
                    true
                )
            )
            {
                gateway =
                    request
                    ->getParam(
                        "gateway",
                        true
                    )
                    ->value();
            }


            ssid.trim();

            pass.trim();

            ip.trim();

            gateway.trim();


            if (
                ssid.length() == 0 ||
                pass.length() == 0
            )
            {
                request->send(
                    400,
                    "text/plain",
                    "SSID and password are required."
                );

                return;
            }


            writeFile(
                "/ssid.txt",
                ssid
            );


            writeFile(
                "/pass.txt",
                pass
            );


            writeFile(
                "/ip.txt",
                ip
            );


            writeFile(
                "/gateway.txt",
                gateway
            );


            request->send(
                200,
                "text/html",

                "<!DOCTYPE html>"

                "<html>"

                "<head>"

                "<meta name='viewport' "
                "content='width=device-width, initial-scale=1'>"

                "</head>"

                "<body style='"
                "font-family:Segoe UI,Arial;"
                "text-align:center;"
                "padding:50px;"
                "background:#eeeeee;"
                "'>"

                "<div style='"
                "background:white;"
                "padding:30px;"
                "border-radius:20px;"
                "max-width:430px;"
                "margin:auto;"
                "border:1px solid #f0dce5;"
                "box-shadow:0 15px 40px "
                "rgba(128,74,100,.12);"
                "'>"

                "<h1 style='color:#b85f83;'>"
                "WiFi Saved!"
                "</h1>"

                "<p style='color:#9b8790;'>"
                "The ESP32 will restart and "
                "connect to the saved WiFi."
                "</p>"

                "<p style='color:#9b8790;'>"
                "Please wait..."
                "</p>"

                "</div>"

                "</body>"

                "</html>"
            );


            delay(1500);


            ESP.restart();
        }
    );


    server.begin();


    Serial.println();
    Serial.println(
        "================================="
    );

    Serial.println(
        "    CRIZEL WIFI MANAGER READY"
    );

    Serial.println(
        "================================="
    );


    Serial.print(
        "Connect to WiFi: "
    );

    Serial.println(
        AP_SSID
    );


    Serial.print(
        "Then open: http://"
    );

    Serial.println(
        WiFi.softAPIP()
    );


    Serial.println();
}


// =====================================================
// MAIN WEB SERVER
// =====================================================

void startMainWebServer()
{
    wifiManagerMode =
        false;


    // -------------------------------------------------
    // MAIN WEBSITE
    // -------------------------------------------------

    server.on(
        "/",
        HTTP_GET,
        [](AsyncWebServerRequest *request)
        {
            if (
                LittleFS.exists(
                    "/index.html"
                )
            )
            {
                request->send(
                    LittleFS,
                    "/index.html",
                    "text/html"
                );
            }
            else
            {
                request->send(
                    404,
                    "text/plain",
                    "index.html not found."
                );
            }
        }
    );


    // -------------------------------------------------
    // CHANGE WIFI
    // -------------------------------------------------

    server.on(
        "/change-wifi",
        HTTP_GET,
        [](AsyncWebServerRequest *request)
        {
            request->send(
                200,
                "text/html",

                "<!DOCTYPE html>"

                "<html>"

                "<head>"

                "<meta name='viewport' "
                "content='width=device-width, initial-scale=1'>"

                "</head>"

                "<body style='"
                "font-family:Segoe UI,Arial;"
                "text-align:center;"
                "padding:50px;"
                "background:#eeeeee;"
                "'>"

                "<div style='"
                "background:white;"
                "padding:30px;"
                "border-radius:20px;"
                "max-width:430px;"
                "margin:auto;"
                "border:1px solid #f0dce5;"
                "'>"

                "<h1 style='color:#b85f83;'>"
                "Changing WiFi..."
                "</h1>"

                "<p style='color:#9b8790;'>"
                "WiFi settings will be cleared."
                "</p>"

                "<p style='color:#9b8790;'>"
                "The ESP32 will restart."
                "</p>"

                "</div>"

                "</body>"

                "</html>"
            );


            delay(1000);


            deleteWiFiFiles();


            ESP.restart();
        }
    );


    // -------------------------------------------------
    // STATIC FILES
    // -------------------------------------------------

    server.serveStatic(
        "/",
        LittleFS,
        "/"
    );


    server.begin();


    Serial.println();
    Serial.println(
        "================================="
    );

    Serial.println(
        "   CRIZEL ACTIVITY 4 WEB SERVER"
    );

    Serial.println(
        "================================="
    );


    Serial.print(
        "Website: http://"
    );

    Serial.println(
        WiFi.localIP()
    );


    Serial.println();
}


// =====================================================
// FIREBASE CALLBACK
// =====================================================

void processFirebase(
    AsyncResult &aResult
)
{
    if (!aResult.isResult())
    {
        return;
    }


    if (aResult.isEvent())
    {
        Firebase.printf(
            "Firebase Event - task: %s, msg: %s, code: %d\n",
            aResult.uid().c_str(),
            aResult.eventLog().message().c_str(),
            aResult.eventLog().code()
        );
    }


    if (aResult.isDebug())
    {
        Firebase.printf(
            "Firebase Debug - task: %s, msg: %s\n",
            aResult.uid().c_str(),
            aResult.debug().c_str()
        );
    }


    if (aResult.isError())
    {
        Firebase.printf(
            "Firebase Error - task: %s, msg: %s, code: %d\n",
            aResult.uid().c_str(),
            aResult.error().message().c_str(),
            aResult.error().code()
        );
    }


    if (aResult.available())
    {
        Firebase.printf(
            "Firebase Payload - task: %s, payload: %s\n",
            aResult.uid().c_str(),
            aResult.c_str()
        );
    }
}


// =====================================================
// FIREBASE SETUP
// =====================================================

void setupFirebase()
{
    Serial.println();
    Serial.println(
        "================================="
    );

    Serial.println(
        "       FIREBASE SETUP"
    );

    Serial.println(
        "================================="
    );


    ssl_client.setInsecure();


    Serial.println(
        "Initializing Firebase..."
    );


    initializeApp(
        aClient,
        app,
        getAuth(user_auth),
        processFirebase,
        "authTask"
    );


    app.getApp<RealtimeDatabase>(
        Database
    );


    Database.url(
        DATABASE_URL
    );


    Serial.println(
        "Firebase initialization started."
    );


    Serial.println();
}


// =====================================================
// GET DATE
// =====================================================

String getDateString()
{
    struct tm timeinfo;


    if (
        !getLocalTime(
            &timeinfo
        )
    )
    {
        return "1970-01-01";
    }


    char buffer[20];


    strftime(
        buffer,
        sizeof(buffer),
        "%Y-%m-%d",
        &timeinfo
    );


    return String(
        buffer
    );
}


// =====================================================
// GET TIME
// =====================================================

String getTimeString()
{
    struct tm timeinfo;


    if (
        !getLocalTime(
            &timeinfo
        )
    )
    {
        return "00:00:00";
    }


    char buffer[20];


    strftime(
        buffer,
        sizeof(buffer),
        "%H:%M:%S",
        &timeinfo
    );


    return String(
        buffer
    );
}


// =====================================================
// SEND SENSOR DATA
// =====================================================

void sendSensorData()
{
    // -------------------------------------------------
    // CHECK FIREBASE
    // -------------------------------------------------

    if (!app.ready())
    {
        Serial.println(
            "Firebase not ready yet..."
        );

        return;
    }


    // -------------------------------------------------
    // READ DHT11
    // -------------------------------------------------

    float humidity =
        dht.readHumidity();


    float temperature =
        dht.readTemperature();


    // -------------------------------------------------
    // CHECK SENSOR
    // -------------------------------------------------

    if (
        isnan(humidity) ||
        isnan(temperature)
    )
    {
        Serial.println();
        Serial.println(
            "================================="
        );

        Serial.println(
            "ERROR: Failed to read DHT11"
        );

        Serial.println(
            "================================="
        );

        Serial.println();

        return;
    }


    // -------------------------------------------------
    // DATE & TIME
    // -------------------------------------------------

    String date =
        getDateString();


    String time =
        getTimeString();


    // -------------------------------------------------
    // BASE PATH
    // -------------------------------------------------

    String basePath =
        "/ESP32_Data/" +
        date +
        "/" +
        time;


    // -------------------------------------------------
    // SENSOR PATHS
    // -------------------------------------------------

    String temperaturePath =
        basePath +
        "/temperature";


    String humidityPath =
        basePath +
        "/humidity";


    // -------------------------------------------------
    // SERIAL
    // -------------------------------------------------

    Serial.println();
    Serial.println(
        "================================="
    );

    Serial.println(
        "       DHT11 SENSOR READING"
    );

    Serial.println(
        "================================="
    );


    Serial.print(
        "Temperature: "
    );

    Serial.print(
        temperature,
        1
    );

    Serial.println(
        " °C"
    );


    Serial.print(
        "Humidity: "
    );

    Serial.print(
        humidity,
        1
    );

    Serial.println(
        " %"
    );


    Serial.print(
        "Date: "
    );

    Serial.println(
        date
    );


    Serial.print(
        "Time: "
    );

    Serial.println(
        time
    );


    Serial.print(
        "Firebase: "
    );

    Serial.println(
        basePath
    );


    // -------------------------------------------------
    // FIREBASE WRITE
    // -------------------------------------------------

    Database.set<float>(
        aClient,
        temperaturePath,
        temperature,
        processFirebase,
        "temperatureTask"
    );


    Database.set<float>(
        aClient,
        humidityPath,
        humidity,
        processFirebase,
        "humidityTask"
    );


    Serial.println();

    Serial.println(
        "Temperature write task sent."
    );

    Serial.println(
        "Humidity write task sent."
    );

    Serial.println(
        "================================="
    );

    Serial.println();
}


// =====================================================
// SETUP
// =====================================================

void setup()
{
    Serial.begin(
        115200
    );


    delay(1000);


    Serial.println();
    Serial.println();


    Serial.println(
        "================================="
    );

    Serial.println(
        " CRIZEL ESP32 DHT11 ACTIVITY 4"
    );

    Serial.println(
        "================================="
    );


    // =================================================
    // LITTLEFS
    // =================================================

    Serial.println();

    Serial.println(
        "Starting LittleFS..."
    );


    if (
        !LittleFS.begin(
            true
        )
    )
    {
        Serial.println(
            "LittleFS mount failed!"
        );


        while (true)
        {
            delay(1000);
        }
    }


    Serial.println(
        "LittleFS ready."
    );


    // =================================================
    // DHT11
    // =================================================

    dht.begin();


    Serial.println(
        "DHT11 initialized."
    );


    Serial.println(
        "DHT11 GPIO: 4"
    );


    // =================================================
    // WIFI
    // =================================================

    bool connected =
        connectToSavedWiFi();


    if (!connected)
    {
        startWiFiManager();

        return;
    }


    // =================================================
    // NTP
    // =================================================

    Serial.println(
        "Starting NTP time..."
    );


    // Philippines UTC+8

    configTime(
        8 * 3600,
        0,
        "pool.ntp.org",
        "time.nist.gov",
        "time.google.com"
    );


    Serial.print(
        "Waiting for time"
    );


    struct tm timeinfo;

    int retry =
        0;


    while (
        !getLocalTime(
            &timeinfo
        ) &&
        retry < 20
    )
    {
        Serial.print(".");

        delay(500);

        retry++;
    }


    Serial.println();


    if (
        getLocalTime(
            &timeinfo
        )
    )
    {
        Serial.println(
            "Time synchronized."
        );


        Serial.print(
            "Date: "
        );

        Serial.println(
            getDateString()
        );


        Serial.print(
            "Time: "
        );

        Serial.println(
            getTimeString()
        );
    }
    else
    {
        Serial.println(
            "WARNING: Time synchronization failed."
        );
    }


    // =================================================
    // FIREBASE
    // =================================================

    setupFirebase();


    // =================================================
    // WEB SERVER
    // =================================================

    startMainWebServer();


    // =================================================
    // READY
    // =================================================

    Serial.println();

    Serial.println(
        "================================="
    );

    Serial.println(
        "       SYSTEM READY"
    );

    Serial.println(
        "================================="
    );


    Serial.print(
        "Website: http://"
    );

    Serial.println(
        WiFi.localIP()
    );


    Serial.println();
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
    // -------------------------------------------------
    // FIREBASE ASYNC TASKS
    // -------------------------------------------------

    if (!wifiManagerMode)
    {
        app.loop();
    }


    // -------------------------------------------------
    // SENSOR EVERY 10 SECONDS
    // -------------------------------------------------

    if (
        !wifiManagerMode &&
        millis() - lastSensorRead >=
        SENSOR_INTERVAL
    )
    {
        lastSensorRead =
            millis();


        sendSensorData();
    }


    delay(10);
}