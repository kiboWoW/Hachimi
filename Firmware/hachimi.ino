#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* AP_SSID = "hizru_dog";
const char* AP_PASSWORD = "12345678";

WebServer server(80);

#define I2C_SDA 8
#define I2C_SCL 9
#define PCA9685_ADDRESS 0x40
#define OLED_ADDRESS 0x3C

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_RESET -1

#define SERVOMIN 102
#define SERVOMAX 512
#define SERVO_FREQUENCY 50

#define FR_HIP 0
#define FR_FOOT 1
#define FL_HIP 2
#define FL_FOOT 3
#define BR_HIP 4
#define BR_FOOT 5
#define BL_HIP 6
#define BL_FOOT 7
#define NUM_SERVOS 8

#define HELLO_LED_PIN 2

Adafruit_PWMServoDriver pwm(PCA9685_ADDRESS);
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

bool oledReady = false;

const int zeroPulse[NUM_SERVOS] = {
  325, 325,
  325, 325,
  325, 325,
  325, 325
};

const int standPulse[NUM_SERVOS] = {
  230, 150,
  420, 500,
  230, 150,
  420, 500
};

const int servoTrim[NUM_SERVOS] = {
  0, 0,
  0, 0,
  0, 0,
  0, 0
};

const bool reverseServo[NUM_SERVOS] = {
  false, false,
  false, false,
  false, false,
  false, false
};

#define MOVE_STEPS 12
#define MOVE_DELAY 5
#define MICRO_SETTLE_DELAY 2

#define BODY_HEIGHT_OFFSET 30
#define FRONT_DOWN_OFFSET 18

#define HIP_FORWARD_SWING 48
#define HIP_BACK_SWING 48
#define FOOT_LIFT 82
#define FOOT_FORWARD 18

#define BR_FOOT_LIFT 100
#define BR_FOOT_FORWARD 25

#define RIGHT_HIP_TRIM 3
#define LEFT_HIP_TRIM -3
#define RIGHT_FOOT_TRIM 2
#define LEFT_FOOT_TRIM -2

#define STEP_PAUSE 30
#define PLANT_PAUSE 150
#define BR_PLANT_PAUSE 175
#define CYCLE_PAUSE 50

#define GREETING_FOOT_RAISE 155
#define GREETING_FOOT_LOW 100
#define GREETING_FOOT_HIGH 145
#define GREETING_HIP_SHIFT 45
#define GREETING_SUPPORT_HIP_SHIFT 18
#define GREETING_SUPPORT_FOOT_SHIFT 12
#define GREETING_PAUSE 110
#define GREETING_SETTLE 300

int currentPulse[NUM_SERVOS];

enum RobotMode {
  MODE_STAND,
  MODE_SIT,
  MODE_WALK,
  MODE_HANDSHAKE,
  MODE_HELLO
};

RobotMode requestedMode = MODE_STAND;
RobotMode runningMode = MODE_STAND;

volatile bool stopMotion = false;

void serviceWeb() {
  server.handleClient();
  delay(1);
}

int applyCalibration(int channel, int rawPulse) {
  int pulse = constrain(
    rawPulse + servoTrim[channel],
    SERVOMIN,
    SERVOMAX
  );

  if (reverseServo[channel]) {
    pulse = SERVOMIN + SERVOMAX - pulse;
  }

  return pulse;
}

bool shouldStop() {
  serviceWeb();
  return stopMotion;
}

bool moveServo(int channel, int targetPulse) {
  int target = applyCalibration(channel, targetPulse);
  int start = currentPulse[channel];

  for (int step = 1; step <= MOVE_STEPS; step++) {
    if (shouldStop()) {
      return false;
    }

    int value = start +
                ((target - start) * step) / MOVE_STEPS;

    currentPulse[channel] = value;
    pwm.setPWM(channel, 0, value);

    delay(MICRO_SETTLE_DELAY + MOVE_DELAY);
  }

  return true;
}

bool waitMotion(int durationMs) {
  unsigned long startTime = millis();

  while (millis() - startTime < (unsigned long)durationMs) {
    if (shouldStop()) {
      return false;
    }

    delay(2);
  }

  return true;
}

int standHip(int channel) {
  int value = standPulse[channel] + BODY_HEIGHT_OFFSET;

  if (channel == FR_HIP || channel == FL_HIP) {
    value += FRONT_DOWN_OFFSET;
  }

  return constrain(value, SERVOMIN, SERVOMAX);
}

void setHelloLED(bool state) {
  digitalWrite(HELLO_LED_PIN, state ? HIGH : LOW);
}

void oledMessage(const char* title, const char* subtitle) {
  if (!oledReady) {
    return;
  }

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);

  oled.setTextSize(2);
  oled.setCursor(0, 8);
  oled.println(title);

  oled.setTextSize(1);
  oled.setCursor(0, 40);
  oled.println(subtitle);

  oled.display();
}

void moveToZeroPosition() {
  stopMotion = false;
  setHelloLED(false);
  oledMessage("SIT", "Zero position");

  for (int channel = 0; channel < NUM_SERVOS; channel++) {
    if (!moveServo(channel, zeroPulse[channel])) {
      return;
    }

    if (!waitMotion(70)) {
      return;
    }
  }

  waitMotion(400);
}

void moveToStandPosition() {
  stopMotion = false;
  setHelloLED(false);
  oledMessage("STAND", "Ready");

  for (int leg = 0; leg < 4; leg++) {
    int hip = leg * 2;
    int foot = hip + 1;

    if (!moveServo(hip, standHip(hip))) {
      return;
    }

    if (!moveServo(foot, standPulse[foot])) {
      return;
    }

    if (!waitMotion(90)) {
      return;
    }
  }

  waitMotion(500);
}

bool stepLeg(
  int hipChannel,
  int footChannel,
  int hipDirection,
  int footDirection,
  bool rightBack,
  bool rightSide
) {
  int hipStand = standHip(hipChannel);
  int footStand = standPulse[footChannel];

  int hipTrim = rightSide
                ? RIGHT_HIP_TRIM
                : LEFT_HIP_TRIM;

  int footTrim = rightSide
                 ? RIGHT_FOOT_TRIM
                 : LEFT_FOOT_TRIM;

  int liftAmount = rightBack
                   ? BR_FOOT_LIFT
                   : FOOT_LIFT;

  int forwardAmount = rightBack
                      ? BR_FOOT_FORWARD
                      : FOOT_FORWARD;

  int plantPause = rightBack
                   ? BR_PLANT_PAUSE
                   : PLANT_PAUSE;

  int footLift = footStand +
                 footDirection * liftAmount;

  int hipForward = hipStand +
                   hipDirection *
                   (HIP_FORWARD_SWING + hipTrim);

  int footForward = footStand +
                    footDirection *
                    (forwardAmount + footTrim);

  int hipPush = hipStand -
                hipDirection *
                (HIP_BACK_SWING + hipTrim);

  if (!moveServo(footChannel, footLift)) {
    return false;
  }

  if (!waitMotion(STEP_PAUSE)) {
    return false;
  }

  if (!moveServo(hipChannel, hipForward)) {
    return false;
  }

  if (!waitMotion(STEP_PAUSE)) {
    return false;
  }

  if (!moveServo(footChannel, footForward)) {
    return false;
  }

  if (!waitMotion(plantPause)) {
    return false;
  }

  if (!moveServo(hipChannel, hipPush)) {
    return false;
  }

  if (!waitMotion(STEP_PAUSE)) {
    return false;
  }

  return moveServo(hipChannel, hipStand);
}

void walkCycle() {
  stopMotion = false;
  setHelloLED(false);
  oledMessage("WALK", "Moving");

  if (!stepLeg(FR_HIP, FR_FOOT, 1, 1, false, true)) {
    return;
  }

  if (!stepLeg(BL_HIP, BL_FOOT, -1, -1, false, false)) {
    return;
  }

  if (!stepLeg(FL_HIP, FL_FOOT, -1, -1, false, false)) {
    return;
  }

  if (!stepLeg(BR_HIP, BR_FOOT, 1, 1, true, true)) {
    return;
  }

  waitMotion(CYCLE_PAUSE);
}

void prepareGreetingBalance() {
  oledMessage("READY", "Greeting");

  moveServo(
    FL_HIP,
    standHip(FL_HIP) - GREETING_SUPPORT_HIP_SHIFT
  );

  moveServo(
    FL_FOOT,
    standPulse[FL_FOOT] + GREETING_SUPPORT_FOOT_SHIFT
  );

  moveServo(
    BR_HIP,
    standHip(BR_HIP) + GREETING_SUPPORT_HIP_SHIFT
  );

  moveServo(
    BR_FOOT,
    standPulse[BR_FOOT] - GREETING_SUPPORT_FOOT_SHIFT
  );

  moveServo(
    BL_HIP,
    standHip(BL_HIP) - GREETING_SUPPORT_HIP_SHIFT
  );

  moveServo(
    BL_FOOT,
    standPulse[BL_FOOT] + GREETING_SUPPORT_FOOT_SHIFT
  );

  waitMotion(GREETING_SETTLE);

  moveServo(
    FR_FOOT,
    standPulse[FR_FOOT] + GREETING_FOOT_RAISE
  );

  moveServo(
    FR_HIP,
    standHip(FR_HIP) + GREETING_HIP_SHIFT
  );

  waitMotion(GREETING_SETTLE);
}

void restoreGreetingBalance() {
  moveServo(FR_FOOT, standPulse[FR_FOOT]);
  moveServo(FR_HIP, standHip(FR_HIP));

  waitMotion(150);
  moveToStandPosition();
}

void performHandshake() {
  stopMotion = false;
  setHelloLED(false);
  oledMessage("HANDSHAKE", "Right leg");

  prepareGreetingBalance();

  for (int i = 0; i < 3; i++) {
    if (!moveServo(
          FR_FOOT,
          standPulse[FR_FOOT] + GREETING_FOOT_LOW
        )) {
      return;
    }

    if (!waitMotion(GREETING_PAUSE)) {
      return;
    }

    if (!moveServo(
          FR_FOOT,
          standPulse[FR_FOOT] + GREETING_FOOT_HIGH
        )) {
      return;
    }

    if (!waitMotion(GREETING_PAUSE)) {
      return;
    }
  }

  restoreGreetingBalance();
}

void performHello() {
  stopMotion = false;
  setHelloLED(true);
  oledMessage("HELLO", "Right leg waving");

  prepareGreetingBalance();

  for (int i = 0; i < 4; i++) {
    if (!moveServo(
          FR_FOOT,
          standPulse[FR_FOOT] + GREETING_FOOT_LOW
        )) {
      return;
    }

    if (!waitMotion(GREETING_PAUSE)) {
      return;
    }

    if (!moveServo(
          FR_FOOT,
          standPulse[FR_FOOT] + GREETING_FOOT_HIGH
        )) {
      return;
    }

    if (!waitMotion(GREETING_PAUSE)) {
      return;
    }
  }

  restoreGreetingBalance();
  setHelloLED(false);
  oledMessage("HELLO", "Done");
  waitMotion(1500);
}

const char PAGE[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Robot Dog</title>
<style>
body {
  font-family: Arial;
  text-align: center;
  background: #f2f2f2;
  padding: 24px;
}

button {
  display: block;
  width: 90%;
  max-width: 360px;
  margin: 12px auto;
  padding: 18px;
  border: 0;
  border-radius: 12px;
  font-size: 22px;
  font-weight: bold;
  color: white;
}

#stand { background: #1976d2; }
#sit { background: #555; }
#walk { background: #16833b; }
#handshake { background: #b36b00; }
#hello { background: #7b2cbf; }
</style>
</head>
<body>
<h1>Robot Dog</h1>

<button id="stand" onclick="go('/stand')">STAND</button>
<button id="sit" onclick="go('/sit')">SIT</button>
<button id="walk" onclick="go('/walk')">WALK</button>
<button id="handshake" onclick="go('/handshake')">
  HANDSHAKE
</button>
<button id="hello" onclick="go('/hello')">HELLO</button>

<p id="status">Ready</p>

<script>
function go(url) {
  fetch(url)
    .then(response => response.text())
    .then(text => {
      document.getElementById("status").textContent = text;
    })
    .catch(() => {
      document.getElementById("status").textContent =
        "Command failed";
    });
}
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", PAGE);
}

void selectMode(RobotMode mode, const char* message) {
  stopMotion = true;
  requestedMode = mode;
  server.send(200, "text/plain", message);
}

void handleStand() {
  selectMode(MODE_STAND, "Stand selected");
}

void handleSit() {
  selectMode(MODE_SIT, "Sit selected");
}

void handleWalk() {
  selectMode(MODE_WALK, "Walk selected");
}

void handleHandshake() {
  selectMode(MODE_HANDSHAKE, "Handshake selected");
}

void handleHello() {
  selectMode(MODE_HELLO, "Hello selected");
}

void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  delay(500);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  Serial.print("Open: http://");
  Serial.println(WiFi.softAPIP());
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(HELLO_LED_PIN, OUTPUT);
  setHelloLED(false);

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  oledReady = oled.begin(
    SSD1306_SWITCHCAPVCC,
    0x3C
  );

  if (!oledReady) {
    oledReady = oled.begin(
      SSD1306_SWITCHCAPVCC,
      0x3D
    );
  }

  if (oledReady) {
    oledMessage("Robot Dog", "OLED ready");
  }

  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQUENCY);
  delay(350);

  for (int channel = 0; channel < NUM_SERVOS; channel++) {
    currentPulse[channel] =
      applyCalibration(channel, zeroPulse[channel]);

    pwm.setPWM(
      channel,
      0,
      currentPulse[channel]
    );

    delay(60);
  }

  startAccessPoint();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/stand", HTTP_GET, handleStand);
  server.on("/sit", HTTP_GET, handleSit);
  server.on("/walk", HTTP_GET, handleWalk);
  server.on("/handshake", HTTP_GET, handleHandshake);
  server.on("/hello", HTTP_GET, handleHello);
  server.begin();

  moveToStandPosition();
  runningMode = MODE_STAND;
}

void loop() {
  serviceWeb();

  if (requestedMode != runningMode) {
    stopMotion = true;
    runningMode = requestedMode;
    stopMotion = false;

    if (runningMode == MODE_STAND) {
      moveToStandPosition();
    }
    else if (runningMode == MODE_SIT) {
      moveToZeroPosition();
    }
    else if (runningMode == MODE_HANDSHAKE) {
      performHandshake();
      runningMode = MODE_STAND;
      requestedMode = MODE_STAND;
    }
    else if (runningMode == MODE_HELLO) {
      performHello();
      runningMode = MODE_STAND;
      requestedMode = MODE_STAND;
    }
  }
  else if (runningMode == MODE_WALK) {
    walkCycle();
  }
  else {
    delay(10);
  }
}
