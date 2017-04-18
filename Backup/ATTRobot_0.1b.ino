/*
This version tests out a new structure in the followEdge() function.
Move around the moveEndTime in the loop to insure that the move() function gets
executed in a proper amount of micros()

ATtiny85, 2 motors, LED, two sensors
Sensor stuff:
	both white	:	no prior	:	random walk
				:	prior		:	turn in direction of last black

	one white	:	ideal situation, move straight

	both black	:	no prior	:	random walk
				:	prior		:	turn in direction of last white
This code was written by Eric Heisler (shlonkin)
It is in the public domain, so do what you will with it.
*/
#include "Arduino.h"
//#include "Filter.h"

// ExponentialFilter<long> ADCFilter(10, 0);
// Here are some parameters that you need to adjust for your setup
// Base speeds are the motor pwm values. Adjust them for desired motor speeds.
#define baseLeftSpeed 37 //22
#define baseRightSpeed 37 //22

// This determines sensitivity in detecting black and white
// measurment is considered white if it is > whiteValue*n/4
// where n is the value below. n should satisfy 0<n<4
// A reasonable value is 3. Fractions are acceptable.
#define leftSensitivity 3  //3
#define rightSensitivity 3 //3

// the pin definitions
#define lmotorpin 1 // PB1 pin 6
#define rmotorpin 0 // PB0 pin 5
#define lsensepin 3 // PB3 ADC3 pin 2
#define rsensepin 1 // PB2 ADC1 pin 7
#define ledpin 4 //PB4 pin 3

// some behavioral numbers
// these are millisecond values
#define steplength 300//300
#define smallturn 200 //200
#define bigturn 500  //500
#define memtime 1000  //1000

#define left 0
#define right 1
#define both 2
#define straight 2

#define edge 0
#define white 1
#define black 2

#define PWM 64  //64 Use 1 when the wiring.h has not been altered.

// variables
uint8_t lspd, rspd; // motor speeds
uint16_t lsenseval, rsenseval, lwhiteval, rwhiteval; // sensor values
unsigned long moveEndTime = 0;

// functions
void followEdge();
void move(uint8_t lspeed, uint8_t rspeed);
void stop();
void senseInit();
void flashLED(uint8_t flashes);
void senseRead();

// just for convenience and simplicity (HIGH is off)
#define ledoff PORTB  &= ~(1 << 4)
#define ledon PORTB |= (1 << 4)



void setup(){

	/*For Fast PWM of 62.500 kHz (prescale factor of 1)
	Use these two lines in the setup function:

	TCCR0A = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
	TCCR0B = _BV(CS00);

	And modify the line in the wiring.c function in the Arduino program files
	hardware\arduino\cores\arduino\wiring.c :
	#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(1 * 256))

	For Phase-correct PWM of 31.250 kHz (prescale factor of 1)
	Use these two lines in the setup function:*/

	TCCR0A = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM00);
	TCCR0B = _BV(CS00);

	/*And modify the line in the wiring.c function in the Arduino program files
	hardware\arduino\cores\arduino\wiring.c :
	#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(1 * 510))
	*/

	// setup pins

	pinMode(lmotorpin, OUTPUT);
	pinMode(rmotorpin, OUTPUT);
	pinMode(2, INPUT); // these are the sensor pins, but the analog
	pinMode(3, INPUT); // and digital functions use different numbers
	ledoff;
	pinMode(ledpin, OUTPUT);
	analogWrite(lmotorpin, 0);
	analogWrite(rmotorpin, 0);

	lspd = baseLeftSpeed;
	rspd = baseRightSpeed;

	// give a few second pause to set the thing on a white surface
	// the LED will flash during this time


	flashLED(3);
	delay(PWM*100);
	senseInit();

	ledon;
}

void loop(){
	// followEdge() contains an infinite loop, so this loop really isn't necessary
	followEdge();
}

void followEdge(){
	// now look for an edge

	//unsigned long moveEndTime = 0; // the millis at which to stop
	unsigned long randomBits = 0; // for a random walk
	unsigned long prior = 0; // after edge encounter set to millis + memtime
	uint8_t priorDir = left; //0=left, 1=right, 2=both
	uint8_t lastSense = white; //0=edge, 1=both white, 2=both black
	uint8_t lastMove = left; //0=left, 1=right, 2=straight
	//uint16_t motionVall, motionValr, lastmotionVall, lastmotionValr;

	senseRead();

	while(true){

			/*motionVall = lsenseval;
			motionValr = rsenseval;

			if (motionVall <= lastmotionVall / 0.99 && motionVall >= lastmotionVall * 0.99){
				if (motionValr <= lastmotionValr / 0.99 && motionValr >= lastmotionValr * 0.99){
							ledon;
							delay(700);
							ledoff;
							delay(700);
							ledon;
							//lspd ++;
							//rspd ++;
					}
				}

			lastmotionVall = motionVall;
			lastmotionValr = motionValr;*/

	while(moveEndTime >= micros() || moveEndTime == 0){

		if(randomBits == 0){ randomBits = micros(); }   // refill the random bits if needed
		senseRead();

		// Here is the important part
		// There are four possible states: both white, both black, one of each
		// The behavior will depend on current and previous states


		if((lsenseval > lwhiteval) && (rsenseval > rwhiteval)){
			// both white - if prior turn toward black, else random walk
			if(lastSense == black || millis() < prior){
				// turn toward last black or left
				switch (priorDir) {
					case left:
						moveEndTime = millis()+smallturn;
						move(0, rspd); // turn left
						lastMove = left;
						break;
					case right:
						moveEndTime = millis()+smallturn;
						move(lspd, 0); // turn right
						lastMove = right;
						break;
					case both:
						moveEndTime = millis()+bigturn;
						move(0, rspd); // turn left a lot
						lastMove = left;
						break;
				}

			}else{
				// random walk
				if(millis() < moveEndTime){
					// just continue moving
				}else{
					if(lastMove == straight){ //0=straight, 1=left, 2=right
						moveEndTime = millis()+steplength;
						move(lspd, rspd); // go straight
						lastMove = straight;
					}else{
						if(randomBits & 1){
							moveEndTime = millis()+smallturn;
							move(0, rspd); // turn left
							lastMove = left;
						}else{
							moveEndTime = millis()+smallturn;
							move(lspd, 0); // turn right
							lastMove = right;
						}
						randomBits >>= 1;
					}
				}
			}
			lastSense = white;

		}else if((lsenseval > lwhiteval) || (rsenseval > rwhiteval)){
			// one white, one black - this is the edge
			// just go straight
			moveEndTime = millis()+steplength;
			move(lspd, rspd); // go straight
			lastMove = straight; //0=straight, 1=left, 2=right
			lastSense = edge;
			prior = millis()+memtime;
			if(lsenseval > lwhiteval*leftSensitivity){
				// the right one is black
				priorDir = right;
			}else{
				// the left one is black
				priorDir = left;
			}

		}else{
			// both black - if prior turn toward white, else random walk
			if(lastSense == white || millis() < prior){
				// turn toward last white or left
				switch (priorDir) {
					case left:
						moveEndTime = millis()+smallturn;
						move(lspd, 0);  // turn left
						lastMove = right;
						break;
					case right:
						moveEndTime = millis()+smallturn;
						move(0, rspd); // turn left
						lastMove = left;
						break;
					case both:
						moveEndTime = millis()+bigturn;
						move(lspd, 0); // turn right a lot
						lastMove = right;
						break;
					}
			}else{
				// random walk
				if(millis() < moveEndTime){
					// just continue moving
				}else{
					if(lastMove == straight){
						moveEndTime = millis()+steplength;
						move(lspd, rspd); // go straight
						lastMove = straight;
					}else{
						if(randomBits & 1){
							moveEndTime = millis()+smallturn;
							move(0, rspd); // turn left
							lastMove = left;
						}else{
							moveEndTime = millis()+smallturn;
							move(lspd, 0); // turn right
							lastMove = right;
							}
							randomBits >>= 1;
						}
					}
				}
				lastSense = black;
			}
		}
	}
}

void move(uint8_t lspeed, uint8_t rspeed){
	analogWrite(lmotorpin, lspeed);
	analogWrite(rmotorpin, rspeed);
}

void stop(){
	analogWrite(lmotorpin, 0);
	analogWrite(rmotorpin, 0);
}

// stores the average of 16 readings as a white value
void senseInit(){
	lwhiteval = 0;
	rwhiteval = 0;

	ledon;
	delay(PWM*10);

		for(uint8_t i=0; i<16; i++){
			lwhiteval += analogRead(lsensepin);
			delay(PWM*1);
			rwhiteval += analogRead(rsensepin);
			delay(PWM*9);
		}

	lwhiteval >>= 4;
	rwhiteval >>= 4;
	lwhiteval = lwhiteval*leftSensitivity;
	rwhiteval = rwhiteval*rightSensitivity;  //maybe try mapping this instead?

	ledoff;
}

void senseRead(){

		lsenseval = 0;
		rsenseval = 0;

		for(uint8_t i=0; i<8; i++){
			lsenseval += analogRead(lsensepin);
			delay(1);//what if I added delays to let analogRead stablize?
			rsenseval += analogRead(rsensepin);
			delay(9);
			}
}

void flashLED(uint8_t flashes){
	while(flashes){
		flashes--;
		ledon;
		delay(PWM*200);
		ledoff;
		if(flashes){ delay(PWM*500); }
	}
	while(flashes){
		flashes--;
		ledon;
		delay(PWM*100);
		ledoff;
		if(flashes){ delay(PWM*200); }
	}
}
