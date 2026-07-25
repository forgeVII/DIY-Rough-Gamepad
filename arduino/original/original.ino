#include <Wire.h>
#include <USBComposite.h>

const int MPU_addr = 0x68;
int16_t AcX, AcY, AcZ, GyX, GyY, GyZ;
float accAngleX, accAngleY;
float roll = 0, pitch = 0, yaw = 0;
float elapsedTime, currentTime, previousTime;
float GyXoffset = 0, GyYoffset = 0, GyZoffset = 0;

const int analogPins[6] = {PA0, PA1, PA2, PA3, PA4, PA5};
const int btnPins[12] = {PB0, PB1, PB3, PB4, PB5, PB8, PB9, PB10, PB11, PB12, PB13, PB14};
const int NUM_BUTTONS = 12;

#pragma pack(push, 1)
struct GamepadReport_t {
    uint8_t  reportID;
    uint16_t buttons;
    uint8_t  hat;
    uint8_t  x, y, z, rx, ry, rz;
    uint8_t  sliderLeft, sliderRight;
};
#pragma pack(pop)

static const uint8_t customReportDesc[] = {
    0x05, 0x01,
    0x09, 0x05,
    0xA1, 0x01,
    0x85, 0x01,

    0x05, 0x09,
    0x19, 0x01,
    0x29, 0x0C,
    0x15, 0x00,
    0x25, 0x01,
    0x75, 0x01,
    0x95, 0x0C,
    0x81, 0x02,
    0x75, 0x01,
    0x95, 0x04,
    0x81, 0x03,

    0x05, 0x01,
    0x09, 0x39,
    0x15, 0x00,
    0x25, 0x07,
    0x35, 0x00,
    0x46, 0x3B, 0x01,
    0x65, 0x14,
    0x75, 0x04,
    0x95, 0x01,
    0x81, 0x42,
    0x75, 0x01,
    0x95, 0x04,
    0x81, 0x03,

    0x05, 0x01,
    0x09, 0x30,
    0x09, 0x31,
    0x09, 0x32,
    0x09, 0x33,
    0x09, 0x34,
    0x09, 0x35,
    0x09, 0x36,
    0x09, 0x36,
    0x15, 0x00,
    0x26, 0xFF, 0x00,
    0x75, 0x08,
    0x95, 0x08,
    0x81, 0x02,

    0xC0
};

static const HIDReportDescriptor customHIDReportDesc = {
    customReportDesc,
    sizeof(customReportDesc)
};

static GamepadReport_t gamepadReport;
USBHID HID;

class CustomGamepad : public HIDReporter {
public:
    CustomGamepad(USBHID& h) : HIDReporter(h, &customHIDReportDesc, (uint8_t*)&gamepadReport, sizeof(GamepadReport_t), 1) {}
    void send() { sendReport(); }
};

CustomGamepad gamepad(HID);

void readAccel() {
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_addr, 6, true);
    AcX = Wire.read() << 8 | Wire.read();
    AcY = Wire.read() << 8 | Wire.read();
    AcZ = Wire.read() << 8 | Wire.read();
}

void readGyro() {
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x43);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_addr, 6, true);
    GyX = Wire.read() << 8 | Wire.read();
    GyY = Wire.read() << 8 | Wire.read();
    GyZ = Wire.read() << 8 | Wire.read();
}

void setup() {
    Wire.begin();
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x6B);
    Wire.write(0);
    Wire.endTransmission(true);

    *(volatile uint32_t *)0x40010004 = (*(volatile uint32_t *)0x40010004 & ~(0x7 << 24)) | (0x2 << 24);

    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(btnPins[i], INPUT_PULLUP);
    }

    HID.begin(&customHIDReportDesc);

    memset(&gamepadReport, 0, sizeof(gamepadReport));
    gamepadReport.reportID = 1;
    gamepadReport.hat = 15;

    long sumGyX = 0, sumGyY = 0, sumGyZ = 0;
    for (int i = 0; i < 2000; i++) {
        readGyro();
        sumGyX += GyX;
        sumGyY += GyY;
        sumGyZ += GyZ;
        delay(1);
    }
    GyXoffset = (float)sumGyX / 2000;
    GyYoffset = (float)sumGyY / 2000;
    GyZoffset = (float)sumGyZ / 2000;

    currentTime = millis();
}

void loop() {
    previousTime = currentTime;
    currentTime = millis();
    elapsedTime = (currentTime - previousTime) / 1000.0;

    readAccel();
    readGyro();
    GyX -= GyXoffset;
    GyY -= GyYoffset;
    GyZ -= GyZoffset;

    accAngleX = (atan(AcY / sqrt(pow(AcX, 2) + pow(AcZ, 2))) * 180 / PI);
    accAngleY = (atan(-1 * AcX / sqrt(pow(AcY, 2) + pow(AcZ, 2))) * 180 / PI);

    float accMag = sqrt(pow(AcX, 2) + pow(AcY, 2) + pow(AcZ, 2)) / 16384.0;
    float alpha = (abs(accMag - 1.0) > 0.1) ? 0.99 : 0.96;

    roll  = alpha * (roll + (GyX / 131.0) * elapsedTime) + (1.0 - alpha) * accAngleX;
    pitch = alpha * (pitch + (GyY / 131.0) * elapsedTime) + (1.0 - alpha) * accAngleY;
    yaw   += (GyZ / 131.0) * elapsedTime;

    while (yaw > 180) yaw -= 360;
    while (yaw < -180) yaw += 360;

    gamepadReport.x           = 255 - constrain((int)((yaw + 180.0) * 255.0 / 360.0), 0, 255);
    gamepadReport.y           = constrain((int)((-pitch + 90.0) * 255.0 / 180.0), 0, 255);
    gamepadReport.z           = constrain((int)((-roll + 90.0) * 255.0 / 180.0), 0, 255);
    gamepadReport.rx          = 255 - (analogRead(analogPins[0]) >> 4);
    gamepadReport.ry          = 255 - (analogRead(analogPins[1]) >> 4);
    gamepadReport.rz          = analogRead(analogPins[3]) >> 4;
    gamepadReport.sliderLeft  = constrain(128 + ((analogRead(analogPins[4]) - 2048) >> 4) - ((analogRead(analogPins[5]) - 2048) >> 4), 0, 255);
    gamepadReport.sliderRight = analogRead(analogPins[2]) >> 4;

    uint16_t btnMask = 0;
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (!digitalRead(btnPins[i])) {
            btnMask |= (1 << i);
        }
    }
    gamepadReport.buttons = btnMask;

    if ((btnMask & 0x0E00) == 0x0E00) {
        yaw = 0;
        roll = 0;
        pitch = 0;
    }

    gamepad.send();

    delay(10);
}
