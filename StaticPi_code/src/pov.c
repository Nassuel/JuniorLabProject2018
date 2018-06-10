/**
\file       pov.c
\author     Eddy Ferre - ferree@seattleu.edu, Nassuel Valera Cuevas - valeran1@seattleu.edu, Eli Smith - smithe25@seattleu.edu
\date       01/13/2018

\brief      Main persistence of vision file for the static RPi controllers.

This program will run the logic of the static RPi
*/

// Linux C libraries
#include <stdio.h>     //printf, fprintf, stderr, fflush, scanf, getchar
#include <string.h>    //strncpy, strerror
#include <errno.h>     //errno
#include <stdlib.h>    //exit, EXIT_SUCCESS, EXIT_FAILURE
#include <signal.h>    //signal, SIGINT, SIGQUIT, SIGTERM
#include <wiringPi.h>  //wiringPiSetup, pinMode, digitalWrite, delay, INPUT, OUTPUT, PWM_OUTPUT
#include <time.h>

// Local headers
#include "static_pinout.h"
#include "motor.h"      //initMotor
#include "web_client.h" //initWebClient_new_port

// Local function declaration
/// Function controlling exit or interrupt signals
void control_event(int sig);

// Communication Thread
void* communicationThrFunc (void* null_ptr);

const int clock_face[120]={	0x800400,	0xA70400,	0xC90000,	0xB10000,	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	
	0xC00000,	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	0xC00000,	0x800000,	
	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	0x840000,	0x8A0000,	0x880400,	0xEC0400,	0x880400,	0x8A0000,	0x840000,	
	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	0xC00000,	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	
	0x800000,	0x800000,	0x800000,	0x800000,	0xC00000,	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	0xF00000,	0x800000,	
	0x800000,	0x800400,	0x800400,	0x800400,	0x800000,	0x800000,	0xF00000,	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	
	0xC00000,	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	0xC00000,	0x800000,	
	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	0x840000,	0x8A0000,	0x820400,	0xE60400,	0x8A0400,	0x8A0000,	0x840000,	
	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	0xC00000,	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	
	0x800000,	0x800000,	0x800000,	0x800000,	0xC00000,	0x800000,	0x800000,	0x800000,	0x800000,	0x800000,	0xA10000,	0xFF0000,	
	0x810400,	0x800400	};

char my_ip[20];
char my_ip2[20];
int hashIP = 0;
char lockMess[200];
char rotor_ip[20]; 

/**
main function - Entry point function for pov

@param argc number of arguments passed
@param *argv IP address of the rotor RPi

@return number stdlib:EXIT_SUCCESS exit code when no error found.
*/
int main (int argc, char *argv[])
{
    // Inform OS that control_event() function will be handling kill signals
    (void)signal(SIGINT, control_event);
    (void)signal(SIGQUIT, control_event);
    (void)signal(SIGTERM, control_event);

    //Local variable definition
	char server_ip[20];
	int new_clock_face[120];
	char message[2000];
	// pthread_t communicationThr;

    // Parse 1st argument: rotor RPi IP
    if(argc < 2)
    {
        printf("Enter the rotor RPi IP: > ");
        fflush(stdout);
        scanf(" %s", rotor_ip);
        getchar();
        fflush(stdin);
    }
    else
    {
        strncpy(rotor_ip, argv[1], sizeof rotor_ip - 1);
    }
	// Parse 2nd argument: server RPi IP
    if(argc < 3)
    {
        printf("Enter the server IP: > ");
        fflush(stdout);
        scanf(" %s", server_ip);
        getchar();
        fflush(stdin);
    }
    else
    {
        strncpy(server_ip, argv[2], sizeof server_ip - 1);
    }
    printf("  Rotor IP address: %s\n", rotor_ip);
    printf("  Server IP address: %s\n", server_ip);
	sprintf(my_ip, "%s", getMyIP("wlan0"));
    printf("  My wireless IP is: %s\n", my_ip);
	
	sprintf(my_ip2, "%s", getMyIP("wlan0"));

    // Open communication socket with rotor Rpi
    initWebClient(server_ip);
	
	// Send lock command with hashed IP address (my_ip)
	char* lockChar;
	char delim[] = ".";
	
	lockChar = strtok(my_ip2, delim);
	lockChar = strtok(NULL, delim); // first number of ip address
	while (lockChar != NULL)
	{
		int i = strtol(lockChar, NULL, 10);
		lockChar = strtok(NULL, delim);
		hashIP += i; // add all of the ip digits
		lockChar = strtok(NULL, delim); // next number
	}
	hashIP %= 37;
	sprintf(lockMess, "%s,%s,lock,%d\n", my_ip, rotor_ip, hashIP);
	sendMessage(lockMess);
	
	// Time setup
	time_t ugly_time;
	struct tm *pretty_time;

    // Initialize the Wiring Pi facility
	printf("Initialize Wiring Pi facility... ");
    if (wiringPiSetup() != 0)
    {
        // Handles error Wiring Pi initialization
        fprintf(stderr, "Unable to setup wiringPi: %s\n", strerror(errno));
        fflush(stderr);
        exit(EXIT_FAILURE);
    }
	printf("Done\n");

    // Initialize GPIO pins
    printf("Initialize GPIO pins... ");
    pinMode(IN_SW1, INPUT);
    pinMode(IN_MT_ENC_A, INPUT);
    pinMode(IN_MT_ENC_B, INPUT);
    pinMode(OUT_MT_EN, OUTPUT);
    pinMode(OUT_PWM, OUTPUT);
    digitalWrite(OUT_PWM, 0);
    digitalWrite(OUT_MT_EN, 0);
	printf("Done\n");
	
	// Initialize Sound pins
	printf("Initialize Sound pins... ");
	pinMode(S_EN, OUTPUT);
	digitalWrite(S_EN, 1);
	system("gpio mode 23 ALT0");
    system("gpio mode 26 ALT0");
	printf("Done\n");

    // Start encoder counter ISRs, setting actual motor RPM
	printf("Initialize Motor... ");
    initMotor();
	printf("Done\n");
	
	int ret = pthread_create( &(communicationThr), NULL, communicationThrFunc, NULL);
    if( ret ) {
		fprintf(stderr,"Error creating communicationThr - pthread_create() return code: %d\n",ret);
		fflush(stderr);
		return ret;
    }

    // Display controller (Main loop)
    int min, prev_min = -1, hr, min_idx, hr_idx, played = 0;
    printf("Start Main loop\n");
    while(1) {
        time(&ugly_time);
		pretty_time = localtime(&ugly_time);
		
		min = pretty_time->tm_min;
		hr = pretty_time->tm_hour;
		
		if (min == 0 && !played) {
			system("omxplayer -o local --vol=-1500 res/hourlychimebeg.mp3");
			for (int i = 0;i < hr; i++)
				system("omxplayer -o local --vol=-1500 res/bigbenstrike.mp3");
			played = 1;
		}
		
		if(prev_min != min) // New time to display
		{
			//set the clock image table
			for(int i=0; i<120; i++) {
				new_clock_face[i] = clock_face[i];
			}
			
			// set the hour and min hands
			min_idx = min * 2;
			hr_idx = (hr * 10 + min / 6) % 120;
			
			//minute dial
			new_clock_face[min_idx + 1] |= 0xFFFFF;
			new_clock_face[min_idx] |= 0x1FFFFF;
			new_clock_face[min_idx - 1] |= 0xFFFFF;
			//hour dial
			new_clock_face[hr_idx + 1] |= 0xFFF;
			new_clock_face[hr_idx] |= 0x1FFF;
			new_clock_face[hr_idx - 1] |= 0xFFF;
			
			//send display command
			sprintf(message, "%s,%s,display", my_ip, rotor_ip);
			for(int i = 0; i < 120 ; i++) {
				if(new_clock_face[i] != 0)
				{
					sprintf(message, "%s,%d,%X", message, i, new_clock_face[i]);
				}
			}
			
			sprintf(message, "%s\n", message);
			
			sendMessage(message);
			
			prev_min = min;
		}
        delay(500);  //Always keep a sleep or delay in infinite loop
    }
    return 0;
}

// Checks if the connection with the Rotor Pi was established
void* communicationThrFunc (void* null_ptr) {
	char* tokenR;
	char* token2;
	delimR = ",\n"
	
	char responseM[2000];
	
	sprintf(responseM, "%s", getMessage());
	token = strtok(responseM, delim2); // my ip
	token = strtok(NULL, delim2); // rotor ip
	
	if (!strcmp(token, ))
}

void control_event(int sig)
{
    printf("\b\b  \nExiting pov... ");
	
	sprintf(lockMess, "%s,%s,unlock,%d\n", my_ip, rotor_ip, hashIP);
	sendMessage(lockMess);
	
    //stop the motor
    stopMotor();

    delay(200);
    printf("Done\n");
    exit(EXIT_SUCCESS);
}
