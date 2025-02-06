#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <Arduino.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#define CE_PIN 7
#define CSN_PIN 8

class AffineCipher {
public:
    AffineCipher(int a, int b) : a(a), b(b) {
        if (gcd(a, 10) != 1) {
            Serial.println("Error: 'a' must be coprime with 10.");
            while (1);
        }
    }

    char transform(char digit, bool encrypt) {
        if (isdigit(digit)) {
            int value = encrypt ? (digit - '0') : ((digit - '0' - b + 10) % 10);
            int result = (encrypt ? (a * value) : (modInverse(a, 10) * value)) % 10;
            if (result < 0) result += 10; 
            return result + '0';
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

int a = 3; 
int b = 2; 
AffineCipher cipher(a, b);
const byte receivingAddress[6] = "00021";
LiquidCrystal_I2C lcd(0x27,16,2);
RF24 radio(CE_PIN, CSN_PIN);

void setup() {
    Serial.begin(9600);
    radio.begin();
    radio.setChannel(69);
    radio.setDataRate(RF24_1MBPS);
    radio.setPALevel(RF24_PA_MAX);
    radio.openReadingPipe(1, receivingAddress);
    radio.startListening();
    lcd.init();                     
    lcd.backlight();
}

void loop() {
    if (radio.available()) {
        char dataReceived[32]; 
        radio.read(&dataReceived, sizeof(dataReceived));
        
        String receivedData = String(dataReceived); 
        int firstDelimiter = receivedData.indexOf('|');
        int secondDelimiter = receivedData.indexOf('|', firstDelimiter + 1);

        String encrypted[3] = {
            receivedData.substring(0, firstDelimiter),
            receivedData.substring(firstDelimiter + 1, secondDelimiter),
            receivedData.substring(secondDelimiter + 1)
        };

        for (int i = 0; i < 3; i++) {
            String decrypted = "";
            Serial.print(i);
            for (char digit : encrypted[i]) {
                decrypted += cipher.transform(digit, false);
            }
            Serial.print("Расшифрованный текст "); 
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.println(decrypted);
            if (i==0){
              lcd.setCursor(0, 0);
              lcd.print("Temreture: ");
              lcd.print(decrypted);
            }
            if (i==1){
              lcd.setCursor(0, 1);
              lcd.print("Humidity: ");
              if (decrypted.toInt()<50){
                lcd.print("NOT OK");
              }
              else{
                lcd.print("OK");
              }
              delay(1000);
              lcd.clear();
            }
            if (i==2){
              int decryptedInt = decrypted.toInt();
              lcd.setCursor(0,0);
              if (decryptedInt < 341) { // 0-340: Низкая освещенность
                lcd.print("Low Light");
              } else if (decryptedInt >= 341 && decryptedInt < 682) { // 341-681: Средняя освещенность
                lcd.print("Medium Light");
              } else { // 682-1023: Высокая освещенность
                lcd.print("High Light");
              }
              delay(1000);
              lcd.clear();
            }
        }
    }
}
