#include "Arduino.h"
#include <EEPROM.h>
#include "bsp.h"
#include "LowPower.h"

#define TIMER_SEC 15

// Globals
static volatile bool run_interrupt = 0;
static volatile unsigned int timer_count_sec;


int write_state(uint8_t state) {
    // if (state != 0 || state != 1) {
    //     Serial.println(state);
    //     Serial.println("State write in EEPROM with not allowed value");
    //     return 1;
    // }
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

void stop_timer() {
    TIMSK1 |= (0 << TOIE1);   // disable timer overflow interrupt
}

void start_timer() {
    TCNT1 = 31250;   // preload timer
    timer_count_sec = TIMER_SEC;
    TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
}

void trig_interrupt(){
    run_interrupt = 1;
}

void interrupt_routine() {
    detachInterrupt(digitalPinToInterrupt(PUSH_BUTTON_IN));
    run_interrupt = 0;
    Serial.println(read_state());
    start_timer();
    if (read_state() == 0) {
        Serial.println("retracting");
        motor_drive(100);
        write_state(1);
    } else
    {
        Serial.println("extending");
        motor_drive(-100);
        write_state(0);
    }
    delay(500);
    EIFR |= 1 << INTF0;
    attachInterrupt(digitalPinToInterrupt(PUSH_BUTTON_IN), trig_interrupt, FALLING);
    run_interrupt = 0;
}

void setup() {
    Serial.begin(9600);
    noInterrupts(); 
    // Set pin modes
    pinMode(MOTOR_OUT_1, OUTPUT);
    pinMode(MOTOR_OUT_2, OUTPUT);
    pinMode(PUSH_BUTTON_IN, INPUT_PULLUP);

    // setup Timer1
    TCCR1A = 0;
    TCCR1B = 0;

    TCNT1 = 31250;   // preload timer
    TCCR1B |= (1 << CS12);    // 256 prescaler
    TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
    stop_timer();

    interrupts();   

    // stop motor
    motor_drive(0);

    // Attach interrupt
    attachInterrupt(digitalPinToInterrupt(PUSH_BUTTON_IN), trig_interrupt, FALLING);
}

void loop() {
    if (run_interrupt) {
        interrupt_routine();
    }
}

ISR(TIMER1_OVF_vect)        // interrupt service routine every one second
{
    TCNT1 = 31250;   // preload timer
    timer_count_sec --;
    if (timer_count_sec == 0) {
        stop_timer();
        Serial.println("TIMER FINISHED, going to sleep");
        Serial.flush();
        LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    }
}