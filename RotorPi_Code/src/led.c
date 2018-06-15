/**
\file       led.c
\author     Eddy Ferre - ferree@seattleu.edu, Nassuel Valera Cuevas - valeran1@seattleu.edu, Eli Smith - smithe25@seattleu.edu
\date       01/13/2018

\brief      Main persistence of vision file for the rotor RPi controller.

This program will run the logic of the rotor RPi
*/

// Linux C libraries
#include <stdio.h>     //printf, fprintf, stderr, fflush, scanf, getchar
#include <string.h>    //strncpy, strerror
#include <errno.h>     //errno
#include <stdlib.h>    //exit, EXIT_SUCCESS, EXIT_FAILURE
#include <signal.h>    //signal, SIGINT, SIGQUIT, SIGTERM
#include <wiringPi.h>  //wiringPiSetup, pinMode, digitalWrite, delay, INPUT, OUTPUT, PWM_OUTPUT
#include <pthread.h>

// Local headers
#include "web_client.h" //initWebClient_new_port

// Local function declaration
/// Function controlling exit or interrupt signals
void control_event(int sig);

void detect_ISR(void);

// Communication Thread
void* communicationThrFunc (void* null_ptr);


const int pin_table[]={28, 27, 26, 24, 23, 22, 21, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

// Global Variables
int idx=0;
int value[120];
char my_ip[20];

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
	pthread_t communicationThr;

	// Parse 1st argument: server RPi IP
    if(argc < 2)
    {
        printf("Enter the server IP: > ");
        fflush(stdout);
        scanf(" %s", server_ip);
        getchar();
        fflush(stdin);
    }
    else
    {
        strncpy(server_ip, argv[1], sizeof server_ip - 1);
    }
	
	sprintf(my_ip, "%s", getMyIP("wlan0"));
	printf("  Server IP address: %s\n", server_ip);
    printf("  My wireless IP is: %s\n", my_ip);

    // Open communication socket with rotor Rpi
    initWebClient(server_ip);

    // Initialize the Wiring Pi facility
	printf("Initialize Wiring Pi facility... ");
    if (wiringPiSetup() != 0)
    {
        // Handles error Wiring Pi initialization
        fprintf(stderr, "Unable to setup wiringPi: %s\n", strerror(errno));
        fflush(stderr);
        exit(EXIT_FAILURE);
    }
    // Initialize GPIO pins
	for(int i=0; i<24; i++)
	{
		pinMode(pin_table[i], OUTPUT);
	}
	pinMode(29, INPUT);
	wiringPiISR(29, INT_EDGE_RISING, &detect_ISR);
    printf("Done\n");


    int ret = pthread_create( &(communicationThr), NULL, communicationThrFunc, NULL);
	if( ret )
	{
		fprintf(stderr,"Error creating communicationThr - pthread_create() return code: %d\n",ret);
		fflush(stderr);
		return ret;
	}	
	
    // Display controller (Main loop)
	for(int i=0; i<120; i++)
	{
		value[i] = 0;
	}

    printf("Start Main loop\n");
    while(1){
		
		
		for (int j = 0; j < 24; j++) {
			digitalWrite(pin_table[j],value[idx] & (1<<j)); // masking
		}
		idx++;
		idx %= 120;
		// 1700 rpm 200
		// 1500 rpm 250
		delayMicroseconds(250);
		
    }
    return 0;
}

void control_event(int sig)
{
    printf("\b\b  \nExiting led... ");

	for (int j = 0; j < 24; j++) {
		digitalWrite(pin_table[j], 0);
	}

	stopWebClient();
    delay(200);
    printf("Done\n");
    exit(EXIT_SUCCESS);
}

void detect_ISR(void)
{
	idx = 64;
}

void* communicationThrFunc (void* null_ptr)
{
	char message[2000], response[2000];
	char* token;
	char static_ip[20];
	char delim[] = ",\n";
	//int locked = 0;
	int lockVal;

	while(1)
	{
		sprintf(message, "%s", getMessage());
		token = strtok(message, delim); // static RPi IP
		// Decode the static_pi IP address here and store it in static_ip variable
		sprintf(static_ip, "%s", token);
		token = strtok(NULL, delim); // my IP (ignore)
		token = strtok(NULL, delim); // command
		
		if(!strcmp(token,"display")) // 
		{
			token = strtok(NULL, delim); // value index
			while(token != NULL)
			{
				int i = strtol(token, NULL, 10);
				token = strtok(NULL, delim); // value
				int val = strtol(token, NULL, 16);
				value[i] = val;
				token = strtok(NULL, delim); // next value index
			}
			// Encode static_ip before sending it
			sprintf(response,"%s,%s,response,display,ok\n", my_ip, static_ip);
			sendMessage(response);
		}
		else if(!strcmp(token,"lock"))
		{
			/**
			 * For this, send the hashed ip address and decoded it on here before you 
			 */
			token = strtok(NULL, delim);
			lockVal = strtol(token, NULL, 10);
			//locked = 1;
			for (int i = 0; i < 120; i++)
				value[i] = strtol("000000", NULL, 16);
			// Encode static_ip before sending it
			sprintf(response,"%s,%s,response,lock,LOCKED\n", my_ip, static_ip);
			sendMessage(response);
		}
		else if(!strcmp(token,"unlock"))		
		{
			int localLock;
			token = strtok(NULL, delim);
			localLock = strtol(token, NULL, 10);
			if (lockVal == localLock)
			{
				//locked = 0;
				// Encode static_ip before sending it
				sprintf(response,"%s,%s,response,unlock,UNLOCKED\n", my_ip, static_ip);
				sendMessage(response);
			} else {
				// Encode static_ip before sending it
				sprintf(response,"%s,%s,response,unlock,stillLOCKED\n", my_ip, static_ip);
				sendMessage(response);
			}
		}
		else if(!strcmp(token,"test"))
		{
			// Some tests
		}
		else
		{
			// There's an error, send an error response
		}
	}
}

