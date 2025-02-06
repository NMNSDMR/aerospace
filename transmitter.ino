#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <DHT.h>
#define CE_PIN 7
#define CSN_PIN 8
#define DHTPIN 2
#define DHTTYPE DHT11 // Или DHT22
#include <Arduino.h>

class AffineCipher {
public:
    AffineCipher(int a, int b) : a(a), b(b) {
        if (gcd(a, 10) != 1) {
            Serial.println("Error: 'a' must be coprime with 10.");
            while (1); 
        }
    }

    char encrypt(char digit) {
        if (isdigit(digit)) {
            return (a * (digit - '0') + b) % 10 + '0';
        }
        return digit; 
    }

    char decrypt(char digit) {
        if (isdigit(digit)) {
            int a_inverse = modInverse(a, 10);
            int x = (a_inverse * ((digit - '0' - b + 10) % 10)) % 10; 
            if (x < 0) {
                x += 10; 
            }
            return x + '0';
        }
        return digit; 
    }

private:
    int a, b;

    int gcd(int a, int b) {
        while (b != 0) {
            int temp = b;
            b = a % b;
            a = temp;
        }
        return a;
    }

    int modInverse(int a, int m) {
        a = a % m;
        for (int x = 1; x < m; x++) {
            if ((a * x) % m == 1) {
                return x;
            }
        }
        return 1;
    }
};

RF24 radio(CE_PIN, CSN_PIN);
DHT dht(DHTPIN, DHTTYPE);
const byte forwardingAddress[6] = "00021";
const int photoresistorPin = A0;

AffineCipher cipher(3, 2);



void setup() {
    Serial.begin(9600);
    pinMode(photoresistorPin, INPUT);
    dht.begin();
    radio.begin();
    radio.setChannel(69);
    radio.setDataRate(RF24_1MBPS);
    radio.setPALevel(RF24_PA_MAX);
    radio.openWritingPipe(forwardingAddress);
}

void loop() {
  int sv = analogRead(photoresistorPin);
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  String strT = String(t);
  String strH = String(h);
  String strSV = String(sv);
    String encrypted1 = "";
    String encrypted2 = "";
    String encrypted3 = "";

    for (char digit : strT) {
        encrypted1 += cipher.encrypt(digit);
    }

    for (char digit : strH) {
        encrypted2 += cipher.encrypt(digit);
    }

    for (char digit : strSV) {
        encrypted3 += cipher.encrypt(digit);
    }

    Serial.print("Зашифрованный текст 1: ");
    Serial.println(encrypted1);
    Serial.print("Зашифрованный текст 2: ");
    Serial.println(encrypted2);
    Serial.print("Зашифрованный текст 3: ");
    Serial.println(encrypted3);


    String dataToSend = encrypted1 + "|" + encrypted2 + "|" + encrypted3; 
    radio.write(dataToSend.c_str(), dataToSend.length() + 1); 

    delay(2000);
}
