/* Code for Techno-Heart
 *  Author: Gowri Somanath (http://thepositronicbrain.blogspot.com/)
 *  October 2014.
 *
 *  The code is for creating the techno-heart using the Intel Galileo and the IoT dev kit.
 */

//C,C++ stuff
#include <iostream>
#include <cmath>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

//Intel IoT Dev kit stuff
#include "mraa/gpio.hpp"
#include "mraa/aio.hpp"
#include "mraa/pwm.hpp"
#include "buzzer.h"
//my library for LED pixels from Total Control Lighting
#include "gsTCLLib.cpp"

//Pin numbers and other details
#define NUM_LEDS 12
#define pulse_apin 0 //Connect Pulse heart rate to analog A0
#define echoPin 1 //Connect Ultrasound echo pins from analog pin 1
#define numEcho 5 //Number of ultra-sound connected. [I used 5 and connected from A1 to A5]
#define trigPin 5 //Connect all trigger lines of ultrasound to Digital pin 5
#define speaker_basepin 6 //Connect a 8ohm speaker to digital pwm pin 6 and 3
#define speaker_pin 3

gsTCLLib tcl;
mraa::Aio* pulse_a0;
mraa::Pwm * pwm_speaker;
mraa::Pwm * pwm_speaker_base;
mraa::Gpio *gpio_trig;
mraa::Aio *gpio_echo[numEcho];

struct timespec startt, endt;
float echo_time[numEcho];
int freq;

/*Trigger and process the ultra-sound.
 * Adjust speaker frequency accordingly
 * */
void getUltraSound(int s) {
	freq = 0;
	int cnt;
	int got = 0;
	for (int i = 0; i < numEcho; i++) {
		got = 0;
		//do each ultra-sound atleast 2 times to avoid noise
		for (int trial = 0; trial < 2; trial++) {
			gpio_trig->write(1); //pulse the trigger ping
			gpio_trig->write(0);
			clock_gettime(CLOCK_REALTIME, &startt);
			cnt = 0;
			//wait for the return ping
			while ((gpio_echo[i]->read() > 512) && cnt < 500) {
				cnt++;
			}
			clock_gettime(CLOCK_REALTIME, &endt);
			echo_time[i] = (endt.tv_nsec - startt.tv_nsec) * 0.001f
					+ (endt.tv_sec - startt.tv_sec) * 1000000.0f;
			echo_time[i] = echo_time[i] / 58.0f;
			// 7cm is my threshold. Adjust based on the size of your instrument.
			got += (echo_time[i] < 7.0f);
		}
		//If I got a stable ready then adjust frequency for this sensor
		if (got == 2) {

			freq += (i + 1);

		}
	}

	//play the sounds
	pwm_speaker->config_ms(freq, 0.5f);
	pwm_speaker_base->config_ms(freq / 2, 0.5f);

}

/* Read the pulse sensor and do the lights accordingly
 * */
void doLights(int sig) {

	uint16_t adc_value = pulse_a0->read();
	int r = adc_value >> 1;
	int g = 0; //r>>2;
	int b = r >> 3;
	tcl.sendEmpty();
	for (int j = 0; j < NUM_LEDS; j++) {
		tcl.sendColor(r, g, b);
	}
	tcl.sendEmpty();

}

int main() {

	std::cout << "hey!" << std::endl;

	//setup the different pins
	pulse_a0 = new mraa::Aio(pulse_apin);
	gpio_trig = new mraa::Gpio(trigPin);
	gpio_trig->dir(mraa::DIR_OUT);
	for (int i = 0; i < numEcho; i++) {
		gpio_echo[i] = new mraa::Aio(echoPin + i);

	}

	pwm_speaker = new mraa::Pwm(speaker_pin);
	pwm_speaker_base = new mraa::Pwm(speaker_basepin);
	pwm_speaker->enable(1);
	pwm_speaker_base->enable(1);

	//do a continuous loop for the two functions
	for (;;) {
		getUltraSound(0);
		doLights(0);
	}

	std::cout << "bye!" << std::endl;

	return 0;
}
