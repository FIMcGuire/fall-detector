// This #include statement was automatically added by the Particle IDE.
#include <MPU6050.h>
#include "TinyGPS.h"
#include <google-maps-device-locator.h>

// MPU variables:
MPU6050 accelgyro;
int16_t ax, ay, az, gx, gy, gz;

//GP20u7 variables:
TinyGPS gps;
char gpsData[64];

//WiFi-GPS ariables
GoogleMapsDeviceLocator locator;

//Alert type string
String alert = "";

//Button pin and variables
int BUTTON_PIN = D2;
int LED_PIN = D7;
int button_state = 0;
int oldbutton_state = 0;

void setup() {
    //declare buttonpin as output
    pinMode(BUTTON_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);
    //setup serial communications
    Wire.begin();
    Serial.begin(9600);
    
    //initialize MPU module
    accelgyro.initialize();

    // Cerify the connection:
    Particle.publish("MPU Working", (accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed"));
}

void loop() {
    
    button_state = digitalRead(BUTTON_PIN);
    accelgyro.initialize();
    //call fallDetect() method
    fallDetect();
    Serial.print(button_state);
    if (button_state == 1 & oldbutton_state == 0)
    {    
        alert = "Button Alarm";
        Particle.publish("Alarm", alert);
        getGPS();
    }
    
    oldbutton_state = button_state;
}

//method to detect fall
void fallDetect() {
    //call method ot get MPU data
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    Serial.println(String(ax) + "," + String(ay) + "," + String(az) + ",");
    
    //if X, Y and Z values exceed 30000
    if(abs(ax) > 30000 || abs(ay) > 30000 || abs(az) > 30000)
    {
        //set alert string to fall detected
        alert = "Fall Alarm";
        digitalWrite(LED_PIN, HIGH);
        
        for (unsigned long start = millis(); millis() - start < 5000;)
        {
            button_state = digitalRead(BUTTON_PIN);
            
            if (button_state == 1 & oldbutton_state == 0)
            {    
                alert = "False Alarm";
                Particle.publish("Alarm", alert);
                button_state = 0;
                delay(5000);
                break;
            }
        }
        digitalWrite(LED_PIN, LOW);
        //call getGPS() method
        if(alert == "Fall Alarm")
        {
            Particle.publish("Alarm", alert);
            getGPS();
        }
    }
}

//method to get GPS data
void getGPS() {
    //boolean for GPS validity
    bool isValidGPS = false;
    
    //for X seconds
    for (unsigned long start = millis(); millis() - start < 10000;)
    {
        digitalWrite(LED_PIN, HIGH);
        //while connected to MPU module through Serial1
        while (Serial1.available())
        {
            //char c equal to value read through Serial1
            char c = Serial1.read();
            
            //if gps.encode() method returns true
            if (gps.encode(c))
            {
                //set boolean to true
                isValidGPS = true;
            }
        }
        
        digitalWrite(LED_PIN, LOW);
    }
    
    //if boolean is true
    if (isValidGPS)
    {
        //create float variables for lat and lon and long variable for age
        float lat, lon;
        unsigned long age;
        
        //method to get gps data
        gps.f_get_position(&lat, &lon, &age);
        
        //store string in character bugger
        sprintf(gpsData, "%.6f,%.6f", (lat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : lat), (lon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : lon));
        
        Particle.publish("gpsloc", gpsData);
    }
    else
    {
        //WiFi + Google GPS Location
        locator.withSubscribe(locationCallback).publishLocation();
    }
}

void locationCallback(float lan, float lon, float accuracy) {
    Particle.publish("coordinates", String(lan) + "," + String(lon));
}