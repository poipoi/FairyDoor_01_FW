#include <ArduinoOSC.h>
#include <Adafruit_NeoPixel.h>

WiFiUDP udp;


// ===== NeoPixel ====
#define NEOPIXEL_PIN    (5)
#define BRIGHT_LED_NUM  (4)
#define LAMP_LED_NUM    (1)

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(BRIGHT_LED_NUM + LAMP_LED_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void initNeopixel() {
  pixels.begin();
  pixels.setBrightness(255);
}

void setBrightColor(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < BRIGHT_LED_NUM; i++) {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }
  pixels.show();
}

void setLampColor(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < LAMP_LED_NUM; i++) {
    pixels.setPixelColor(BRIGHT_LED_NUM + i, pixels.Color(r, g, b));
  }
  pixels.show();
}

// ===== Device =====
#define SOLENOID_1_PIN  (14)
#define SOLENOID_2_PIN  (15)

void setSolenoid1Ctrl(int isON) {
  Serial.print("SOLENOID_1 ");
  Serial.println(isON ? "ON" : "OFF");
  digitalWrite(SOLENOID_1_PIN, isON ? HIGH : LOW);
}

void setSolenoid2Ctrl(int isON) {
  Serial.print("SOLENOID_2 ");
  Serial.println(isON ? "ON" : "OFF");
  digitalWrite(SOLENOID_2_PIN, isON ? HIGH : LOW);
}


// ===== Timer =====
#define MSEC2CLOCK(ms)    (ms * 80000L)

typedef void(*TIMER_CALLBACK)(int);

TIMER_CALLBACK timer0_callback = NULL;
int timer_callback_id = 0;

void timer0_ISR (void) {
  timer0_write(ESP.getCycleCount() + MSEC2CLOCK(10) ); // 10msec

  if (timer0_callback) timer0_callback(timer_callback_id);
}

void setTimer0Int(int ms, TIMER_CALLBACK callback, int id) {
  timer0_callback = callback;
  timer_callback_id = id;
  noInterrupts();
  timer0_isr_init();
  timer0_attachInterrupt(timer0_ISR);
  timer0_write(ESP.getCycleCount() + MSEC2CLOCK(ms));
  interrupts();
}


// ===== OSC =====
ArduinoOSCWiFi osc;

// const char* ssid = "NuAns NEO [Reloaded]";
// const char* pwd = "123456ab";
const char* ssid = "Buffalo-G-4F90";
const char* pwd = "54kh53h8mk7kk";

const IPAddress ip(192, 168, 43, 100);
const IPAddress gateway(192, 168, 43, 1);
const IPAddress subnet(255, 255, 255, 0);
const int recv_port = 12345;

void callback_solenoid(OSCMessage& m) {
  Serial.println("OSC: Solenoid");
  
  int id = m.getArgAsInt32(0);
  int isON = m.getArgAsInt32(1);
  if (id == 0) {
    setSolenoid1Ctrl(isON);
  } else if (id == 1) {
    setSolenoid2Ctrl(isON);
  }
}

void callback_bright(OSCMessage& m) {
  Serial.println("OSC: Bright");
  
  int r = m.getArgAsInt32(0);
  int g = m.getArgAsInt32(1);
  int b = m.getArgAsInt32(2);
  setBrightColor(r, g, b);
}

void callback_lamp(OSCMessage& m) {
  Serial.println("OSC: Lamp");

  int r = m.getArgAsInt32(0);
  int g = m.getArgAsInt32(1);
  int b = m.getArgAsInt32(2);
  setLampColor(r, g, b);  
}


// ===== setup =====
void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("");
  Serial.println("===== Wake up! =====");

  pinMode(SOLENOID_1_PIN, OUTPUT);
  pinMode(SOLENOID_2_PIN, OUTPUT);

  digitalWrite(SOLENOID_1_PIN, LOW);
  digitalWrite(SOLENOID_2_PIN, LOW);

  initNeopixel();

//  WiFi.disconnect(true);
  WiFi.begin(ssid, pwd);
  //WiFi.config(ip, gateway, subnet);
  while (WiFi.status() != WL_CONNECTED)
  {
      delay(500);
      Serial.print(".");
  }
  Serial.println("Connection Success!");
  Serial.println(WiFi.localIP());

  osc.begin(udp, recv_port);
  osc.addCallback("/fairydoor/sole", &callback_solenoid);
  osc.addCallback("/fairydoor/bright", &callback_bright);
  osc.addCallback("/fairydoor/lamp", &callback_lamp);
}

char buff[128];
int indx = 0;
void loop() {
  osc.parse();

  if (Serial.available()) {
    char c = Serial.read();
    switch(c) {
      case '\r':
        break;

      case '\n':
        if (indx == 3) {
          if (buff[0] == '0') {
            int id = buff[1] - '0';
            int isON = buff[2] - '0';
            if (id == 0) {
              setSolenoid1Ctrl(isON);            
            } else if (id == 1) {
              setSolenoid2Ctrl(isON);
            }
          } else if (buff[1] == '1') {

          }
        }
        indx = 0;
        break;

      default:
        buff[indx] = c;
        indx++;
        break;
    }
  }
}

