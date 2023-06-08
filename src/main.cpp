#include "Arduino.h"
#include <EEPROM.h>
#include "bsp.h"
#include "LowPower.h"
#include "OneButton.h"

#define TIMER_SEC 15
#define DOUBLE_CLICK_MS 800
#define LONG_PRESS_MS 1000

// Globals
static volatile bool run_interrupt = 0;
static volatile unsigned int timer_count_sec;
unsigned long pressStartTime;

// Setup a new OneButton on pin PUSH_BUTTON_IN
// The 2. parameter activeLOW is true, because external wiring sets the button to LOW when pressed.
OneButton button(PUSH_BUTTON_IN, true);


void motor_stop() {
    digitalWrite(MOTOR_EN, LOW);
    digitalWrite(MOTOR_DIR, HIGH);
    digitalWrite(MOTOR_DRIVER_PW, LOW);
}

void motor_drive(float speed) {
    unsigned int abs_speed;
    // if speed == 0, stop motor
    if (speed == 0) {
        motor_stop();
        return;
    }

    // if speed is not 0, drive motor
    digitalWrite(MOTOR_DRIVER_PW, HIGH);
    if abs(speed > 100) {
        speed = 100;
    } else if (speed < -100)
    {
        speed = -100;
    }
    
    abs_speed = (unsigned int) (abs(speed) * 255./100.);
    
    if (speed > 0) {
        digitalWrite(MOTOR_DIR, HIGH);
    }
    else
    {
        digitalWrite(MOTOR_DIR, LOW);
    }
    analogWrite(MOTOR_EN, abs_speed);
}

void stop_timer() {
    TIMSK1 |= (0 << TOIE1);   // disable timer overflow interrupt
}

void start_timer(volatile unsigned int local_timer_count_sec = TIMER_SEC) {
    TCNT1 = 31250;   // preload timer
    timer_count_sec = local_timer_count_sec;
    TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
}

void trig_interrupt(){
    // run_interrupt = 1;
    button.tick(); // just call tick() to check the state.
}

// this function will be called when the button was pressed 1 time only.
void singleClick() {
    Serial.println("singleClick() detected.");
    start_timer(1);
    motor_drive(0);
} // singleClick

// this function will be called when the button was pressed 2 times in a short timeframe.
void doubleClick() {
    start_timer();
    Serial.println("doubleClick() detected.");
    Serial.println("extending");
    motor_drive(-100);
} // doubleClick

// this function will be called when the button was held down for 1 second or more.
void longPress() {
    start_timer();
    pressStartTime = millis() - LONG_PRESS_MS; // as set in setPressMs()
    // Serial.println("longPress()");
    Serial.println("retracting");
    motor_drive(100);
} // pressStart()


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

    // button mngmt
    // link the xxxclick functions to be called on xxxclick event.
    button.attachClick(singleClick);
    button.attachDoubleClick(doubleClick);

    button.setClickMs(DOUBLE_CLICK_MS);
    button.setPressMs(LONG_PRESS_MS); // that is the time when LongPressStart is called
    button.attachLongPressStart(longPress);
    // button.attachLongPressStop(pressStop);
}

void loop() {
    button.tick();
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