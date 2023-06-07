#include "Arduino.h"
#include <EEPROM.h>
#include "bsp.h"

// Globals
uint8_t global_state; // address 0x00
bool run_interrupt = false;

int write_state(int state) {
    if (state != 0 || state != 1) {
        Serial.println("State write in EEPROM with not allowd value");
        return 1;
    }
    EEPROM.update(0, state);
    return 0;
}

uint8_t read_state() {
    return EEPROM.read(0);
}

void motor_stop() {
    digitalWrite(MOTOR_OUT_1, LOW);
    digitalWrite(MOTOR_OUT_2, LOW);
}

void motor_drive(float speed) {
    unsigned int abs_speed;
    // if speed == 0, stop motor
    if (speed == 0) {
        motor_stop();
        return;
    }

    // if speed is not 0, drive motor
    if abs(speed > 100) {
        speed = 100;
    } else if (speed < -100)
    {
        speed = -100;
    }
    
    abs_speed = (unsigned int) (abs(speed) * 255./100.);
    
    if (speed > 0) {
        digitalWrite(MOTOR_OUT_1, 0);
        analogWrite(MOTOR_OUT_2, abs_speed);
    }
    else
    {
        digitalWrite(MOTOR_OUT_2, 0);
        analogWrite(MOTOR_OUT_1, abs_speed);
    }
}

void trig_interrupt(){
    run_interrupt = true;
}

void interrupt_routine() {
    run_interrupt = false;
    if (read_state() == 0) {
        motor_drive(100);
        write_state(1);
    } else
    {
        motor_drive(-100);
        write_state(0);
    }
}

void setup() {
    Serial.begin(9600);
    // Set pin modes
    pinMode(MOTOR_OUT_1, OUTPUT);
    pinMode(MOTOR_OUT_2, OUTPUT);
    pinMode(PUSH_BUTTON_IN, INPUT_PULLUP);

    // stop motor
    motor_drive(0);

    // Attach interrupt
    attachInterrupt(0, trig_interrupt, FALLING);
}

void loop() {
    if (run_interrupt) {
        interrupt_routine();
    }
}