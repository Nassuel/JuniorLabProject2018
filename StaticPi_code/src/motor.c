#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>
#include <pthread.h>
#include <softPwm.h>

#include "motor.h"
#include "static_pinout.h"
#include "utils.h"

#define PULSE_SAMPLING_MS  250
#define MAX_PWM           1024
#define KP                   1


// Local function declaration
void* speedControlThrFunc (void* null_ptr);

int pulseCounter;
pthread_t speedControlThr;
static int init_motors_done = 0;
int count;


// ISR function for the encoder counter
void counterA_ISR (void)
{
    ++pulseCounter;
}

// Thread function to calculate RPM
void* speedControlThrFunc (void* null_ptr)
{
	const int desired_rpm = 1500;
	const int expected_cnt = (int)(desired_rpm * 211.2 * (float)PULSE_SAMPLING_MS / (4000.0 * 60.0));
	int value;

    while(1) {
        pulseCounter = 0;
        delay(PULSE_SAMPLING_MS);
        count = pulseCounter;
        value += (int)((float)(expected_cnt - count) * KP);
        value = value > MAX_PWM ? MAX_PWM : (value < 0 ? 0 : value);  // Bounds 0 <= value <= MAX_PWM
        softPwmWrite(OUT_PWM, value);
    }
}


// init_motor: initializes motor pin (GPIO allocation and initial values of output)
// and initialize the elements of all motor control data structure
int initMotor(void)
{
    pulseCounter = 0;
    count = 0;

    // Set all GPIO pin modes and states
    pinMode(OUT_PWM, OUTPUT);
    pinMode(OUT_MT_EN, OUTPUT);
    pinMode(DIR_OUT, OUTPUT);
    pinMode(IN_MT_ENC_A, INPUT);
    digitalWrite(OUT_MT_EN, 0);  // Set disable pin to low
    digitalWrite(DIR_OUT, 1);  // set direction, 1: spin clockwise, 0: spin counter-clockwise
    if( softPwmCreate(OUT_PWM, 0, MAX_PWM) )  //Create a SW PWM, value from 0 to MAX_PWM (=100% duty cycle)
    {
        fprintf(stderr,"Error creating software PWM: %s\n", strerror(errno));
        fflush(stderr);
        return -1;
    }

    if(!init_motors_done)
    {
        // Start counter ISR and RPM calculator
        wiringPiISR(IN_MT_ENC_A, INT_EDGE_FALLING, &counterA_ISR);
        int ret = pthread_create( &(speedControlThr), NULL, speedControlThrFunc, NULL);
        if( ret )
        {
            fprintf(stderr,"Error creating speedControlThr - pthread_create() return code: %d\n",ret);
            fflush(stderr);
            return ret;
        }
    }
    init_motors_done = 1;
    return 0;
}

// stopMotor: stop the motor
void stopMotor(void)
{
	pinMode(OUT_PWM, OUTPUT);
    digitalWrite(OUT_PWM, 0);
}


// getCount: accessor funtion of a motor encoder counter
int getCount(void)
{
    return count;
}


// getRPM: accessor funtion of a motor encoder counter
int getRPM(void)
{
    return (int)((float)count * 4000.0 * 60.0 / (211.2 * (float)PULSE_SAMPLING_MS));
}
