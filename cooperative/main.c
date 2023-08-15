/*******************************************************************************
* (c) 2023 by Theo Borm
* see LICENSE file in the root directory of this repository
*
* Controls the LEDs on the "label"-badge. Synchronizes badges using IR pulses
*
* cooperative version - transmitting one 25ms pulse per cycle of LED patterns
* to allow peaceful co-existence
*
* Uses a timer T16 interrupt for delays.
* Uses timer T2 to generate 38Khz IR modulation signal
*/

#include <stdint.h>
#include <device.h>
#include <calibrate.h>

/*******************************************************************************
* configure/calibrate system clock source
*/
unsigned char _sdcc_external_startup(void)
{
	// use the IHRC oscillator, target 4MHz
	PDK_SET_SYSCLOCK(SYSCLOCK_IHRC_4MHZ);
	// calibrate for 4MHz operation @ 4000 mVolt
	EASY_PDK_CALIBRATE_IHRC(4000000,4000);
	return 0;   // keep SDCC happy
}


/*******************************************************************************
* provide arduino-like millis() functionality using T16
* T16 can be clocked from several sources, use a clock divider and can generate
* an interruput when a certain bit (8..15) changes.
* We assume the IHRC is calibrated to 16MHz, then dividing by 64 will produce
* a 250 KHz input clock to T16. After 256 clock pulses bit 8 will toggle, which
* is almost once a millisecond. To make it exactly once a millisecond, we should
* load T16 (which is an up-counter) with the value 6.
*/
volatile uint32_t elapsedmillis;  // overflows after 49.71 days

void setup_millis() {
	T16M = (uint8_t)(T16M_CLK_IHRC | T16M_CLK_DIV64 | T16M_INTSRC_8BIT);
	T16C=6;
	elapsedmillis=0;
	INTEN |= INTEN_T16;
}

/*******************************************************************************
* Interrupt dispatcher - only T16 is used
*/
void interrupt(void) __interrupt(0) {
	if (INTRQ & INTRQ_T16) {
		INTRQ &= ~INTRQ_T16; // Mark as processed
		elapsedmillis++;
		T16C=6;
	}
}

/*******************************************************************************
* 32 bit operations are non-atomic on this 8 bit microcontroller, so we must
* disable the T16 interrupt and then re-enable it after reading the value
*/
uint32_t millis() {
	INTEN &= ~INTEN_T16;
	uint32_t current = elapsedmillis;
	INTEN |= INTEN_T16;
	return(current);
}


/*******************************************************************************
* leds are connected in 2 charlieplexed arrays, left and right side of the PCB
* This makes it possible to light two leds at the same time without resorting
* to rapidly switching between LEDs - this keeps things simple. It is certainly
* possible to make it appear as if more than 2 leds are lit simultaneously as
* well as implement intermediate brightness levels, but one has to take into
* account a lot more details - individual pin drive strength, charlieplexing
* level (20x) and the speed (~20 frames/s) necessary to make persistence of
* vision work, and this is outside the (KISS) scope of this project
*
* When viewed on the PCB, in this file the LEDs are numbered as follows: 
*
*     L04 L03 L02 L01 L00 R00 R01 R02 R03 R04
*     L05                                 R05
*     L06                                 R06
*     L07                                 R07
*     L08                                 R08
*     L19                                 R09
*     L10                                 R10
*     L11                                 R11
*     L12                                 R12
*     L13                                 R13
*     L14                                 R14
*     L15 L16 L17 L18 L19 R19 R18 R17 R16 R15
*
* The LEDs are are connected between pin-pairs which must be set high, low or
* HiZ to activate them as follows:
*
* 	    PB4 PB5 PB6 PB7 PA7                     PA0 PB0 PB1 PB2 PB3
* L00 D37   +               -             R00 D04   -               +
* L01 D29   +           -                 R01 D12       -           +
* L02 D21   +       -                     R02 D20           -       +
* L03 D13   +   -                         R03 D28               -   +
* L04 D38       +           -             R04 D03   -           +
* L05 D30       +       -                 R05 D11       -       +
* L06 D22       +   -                     R06 D19           -   +
* L07 D05   -   +                         R07 D36               +   -
* L08 D39           +       -             R08 D02   -       +
* L09 D31           +   -                 R09 D10       -   +
* L10 D14       -   +                     R10 D27           +   -
* L11 D06    -      +                     R11 D35           +       -
* L12 D40               +   -             R12 D01   -   +
* L13 D23           -   +                 R13 D18       +   -
* L14 D15       -       +                 R14 D26       +       -
* L15 D07    -          +                 R15 D34       +           -
* L16 D32               -   +             R16 D09   +   -
* L17 D24           -       +             R17 D17   +       -
* L18 D15       -           +             R18 D25   +           -
* L19 D08    -              +             R19 D33   +               -
*
* D01..D39 refers to the diode numbers in the hardware schematic.
*
* The controller pins on the LEFT side drive the following LEDs:
* as minus pins		as plus pins
* PA7 => L00 L04 L08 L12	PB4 => L00 L01 L02 L03
* PB7 => L01 L05 L09 L16	PB5 => L04 L05 L06 L07
* PB6 => L02 L06 L13 L17	PB6 => L08 L09 L10 L11
* PB5 => L03 L10 L14 L18	PB7 => L12 L13 L14 L15
* PB4 => L07 L11 L15 L19	PA7 => L16 L17 L18 L19
*
* The controller pins on the RIGHT side drive the following LEDs:
* as minus pins		as plus pins
* PA0 => R00 R04 R08 R12	PB3 => R00 R01 R02 R03
* PB0 => R01 R05 R09 R16	PB2 => R04 R05 R06 R07
* PB1 => R02 R06 R13 R17	PB1 => R08 R09 R10 R11
* PB2 => R03 R10 R14 R18	PB0 => R12 R13 R14 R15
* PB3 => R07 R11 R15 R19	PA0 => R16 R17 R18 R19
*/
 
 
/*******************************************************************************
* this function takes the number of the LED on the left and right sides that
* should be switched ON as arguments
* if an argument is 20, then LEDs on the corresponding side are switched OFF
* if an argument is >=21, then the corresponding side is left unchanged
*/
void setled(int8_t left,int8_t right)
{
	if ( left < 21 )
	{
		PAC &= 0x7f; PBC &= 0x0f; // disable all left outputs
		switch (left)
		{
			case 0: PA &= 0x7f; PAC |= 0x80; PB |= 0x10; PBC |= 0x10; break;
			case 1: PB &= 0x7f; PBC |= 0x80; PB |= 0x10; PBC |= 0x10; break;
			case 2: PB &= 0xbf; PBC |= 0x40; PB |= 0x10; PBC |= 0x10; break;
			case 3: PB &= 0xdf; PBC |= 0x20; PB |= 0x10; PBC |= 0x10; break;
			case 4: PA &= 0x7f; PAC |= 0x80; PB |= 0x20; PBC |= 0x20; break;
			case 5: PB &= 0x7f; PBC |= 0x80; PB |= 0x20; PBC |= 0x20; break;
			case 6: PB &= 0xbf; PBC |= 0x40; PB |= 0x20; PBC |= 0x20; break;
			case 7: PB &= 0xef; PBC |= 0x10; PB |= 0x20; PBC |= 0x20; break;
			case 8: PA &= 0x7f; PAC |= 0x80; PB |= 0x40; PBC |= 0x40; break;
			case 9: PB &= 0x7f; PBC |= 0x80; PB |= 0x40; PBC |= 0x40; break;
			case 10: PB &= 0xdf; PBC |= 0x20; PB |= 0x40; PBC |= 0x40; break;
			case 11: PB &= 0xef; PBC |= 0x10; PB |= 0x40; PBC |= 0x40; break;
			case 12: PA &= 0x7f; PAC |= 0x80; PB |= 0x80; PBC |= 0x80; break;
			case 13: PB &= 0xbf; PBC |= 0x40; PB |= 0x80; PBC |= 0x80; break;
			case 14: PB &= 0xdf; PBC |= 0x20; PB |= 0x80; PBC |= 0x80; break;
			case 15: PB &= 0xef; PBC |= 0x10; PB |= 0x80; PBC |= 0x80; break;
			case 16: PB &= 0x7f; PBC |= 0x80; PA |= 0x80; PAC |= 0x80; break;
			case 17: PB &= 0xbf; PBC |= 0x40; PA |= 0x80; PAC |= 0x80; break;
			case 18: PB &= 0xdf; PBC |= 0x20; PA |= 0x80; PAC |= 0x80; break;
			case 19: PB &= 0xef; PBC |= 0x10; PA |= 0x80; PAC |= 0x80; break;
			default: break; // left side off
		}
	}
	if ( right < 21 )
	{
		PAC &= 0xfe; PBC &= 0xf0; // disable all right outputs		
		switch (right)
		{
			case 0: PA &= 0xfe; PAC |= 0x01; PB |= 0x08; PBC |= 0x08; break;
			case 1: PB &= 0xfe; PBC |= 0x01; PB |= 0x08; PBC |= 0x08; break;
			case 2: PB &= 0xfd; PBC |= 0x02; PB |= 0x08; PBC |= 0x08; break;
			case 3: PB &= 0xfb; PBC |= 0x04; PB |= 0x08; PBC |= 0x08; break;
			case 4: PA &= 0xfe; PAC |= 0x01; PB |= 0x04; PBC |= 0x04; break;
			case 5: PB &= 0xfe; PBC |= 0x01; PB |= 0x04; PBC |= 0x04; break;
			case 6: PB &= 0xfd; PBC |= 0x02; PB |= 0x04; PBC |= 0x04; break;
			case 7: PB &= 0xf7; PBC |= 0x08; PB |= 0x04; PBC |= 0x04; break;
			case 8: PA &= 0xfe; PAC |= 0x01; PB |= 0x02; PBC |= 0x02; break;
			case 9: PB &= 0xfe; PBC |= 0x01; PB |= 0x02; PBC |= 0x02; break;
			case 10: PB &= 0xfb; PBC |= 0x04; PB |= 0x02; PBC |= 0x02; break;
			case 11: PB &= 0xf7; PBC |= 0x08; PB |= 0x02; PBC |= 0x02; break;
			case 12: PA &= 0xfe; PAC |= 0x01; PB |= 0x01; PBC |= 0x01; break;
			case 13: PB &= 0xfd; PBC |= 0x02; PB |= 0x01; PBC |= 0x01; break;
			case 14: PB &= 0xfb; PBC |= 0x04; PB |= 0x01; PBC |= 0x01; break;
			case 15: PB &= 0xf7; PBC |= 0x08; PB |= 0x01; PBC |= 0x01; break;
			case 16: PB &= 0xfe; PBC |= 0x01; PA |= 0x01; PAC |= 0x01; break;
			case 17: PB &= 0xfd; PBC |= 0x02; PA |= 0x01; PAC |= 0x01; break;
			case 18: PB &= 0xfb; PBC |= 0x04; PA |= 0x01; PAC |= 0x01; break;
			case 19: PB &= 0xf7; PBC |= 0x08; PA |= 0x01; PAC |= 0x01; break;
			default: break; // right side <off
		}
	}
}


/*******************************************************************************
* The badge shows a programmed sequence.
* The complete sequence is divided into "states".
* Every state consists of a "pattern" that is repeated a number (N) times
* Every pattern consists of several "steps" 
* During any step, at most 2 LEDs are lit.
*
* While playing the sequence in the main loop, the processor also needs to
* detect the asynchronous sync pulse. It does this by POLLING the IR receiver
* multiple times during EVERY step. If a low-to-high transition is detected on
* the pin connected to the IR receiver, then this means that the END of a sync
* pulse is detected (providing some immunity against antisocial badges). This
* will then result in the pattern being terminated and the state being reset to
* zero. KISS. It is probably also possible to deal with synchronization using
* interrupts, which might actually be just as easy...
*
* The main program cycles through the states, calls the appropriate pattern
* routine that repeats it pattern N times. At every step in a pattern the
* waituntil() function is called to wait some time before proceeding to the
* next step in the pattern. The IR receiver is checked in the waituntil()
* function.
*/

uint8_t state;			// The state of the main loop
uint32_t previoustime;          // The last time the LEDs were updated
uint8_t previouspinstate;	// The last state of the IR receiver pin


/*******************************************************************************
* waituntil() waits for at most <time> milliseconds
* returns 0 in case of a timeout (normal)
* returns 1 in case of a sync pulse detection
*/
uint8_t waituntil(int16_t time)
{
	uint32_t currenttime = millis();
	uint8_t currentpinstate = (PA >> 4) & 1;

	while ((currenttime - previoustime < time) && ( currentpinstate==0 || previouspinstate==1 ))
	{    
		currenttime = millis();
		previouspinstate=currentpinstate;
		currentpinstate = (PA >> 4) & 1;
	}
    
	if (currentpinstate==1 && previouspinstate==0) // rising edge on sync pin 
	{
		state=0;
		previoustime=currenttime;
		previouspinstate=1;
		return(1);
	}
	else // timeout
	{
		previoustime += time;
		return(0);
	}   
}


/*******************************************************************************
* to synchronize badges we want to transmit a 25 ms long 38 kHz synchronization
* pulse using the IR LED. We want to do this at the end of a complete sequence.
* Due to variation in clock speed some badges will reach the end of sequence
* first, transmit a pulse, and thereby resetting the sequence of all other
* badges in its range, and thereby also preventing any other badge in range
* from transmitting a synchronization pulse.
* This is KISS synchronization.
*
* we want to generate a <time> ms long 38kHz sync pulse on PA3 using timer 2
* IHRC 16MHz, 16000000/422=37.914 KHz
* TM2C [7:4]=0010 -> select IHRC
* TB2C [3:2]=10 -> output on PA3 (00=disable)
* TM2C [1] = 0 -> period mode
* TM2C [0] = 0 -> do not invert
* TM2S [7] = 0 -> 8 bit resolution
* TM2S [6:5]=00 -> prescaler 1
* TM2S [4:0]=00000 -> scaler 1
* TM2B [7:0] -> 211
*/
void syncpulse(int16_t time)
{
    TM2C=0; // stop
    TM2CT=0;
    TM2B=211;
    TM2S=0; // clear the counter
    TM2C=0b00101000; // go
    uint32_t currenttime = millis();
    previoustime=currenttime;
    while (currenttime - previoustime < time)
    {
    	currenttime = millis();
    }   
    TM2C=0; // stop PWM
    PA=0; // make sure IR LED is off
}


/*******************************************************************************
* Arduino-like setup() function called from main()
*/
void setup() { // Arduino-like setup function called once after power on
	// Initialize hardware:
  	// disable pull-ups on PB0-7, PA0, PA3 PA7
  	// PA4 is the sync input, which requires the pull-up
  	// PA5 and PA6 are unused (used for programming)
  	// PA1 and PA2 are not available on the package
  	PAPH = 0x76;
	PBPH = 0x00;
  	// set registers low
  	PA=0x00;
  	PB=0x00;
  
	// IR LED is active high source/sink on the PA3 pin - set only PA3 as output
	PAC=0x08;
	PBC=0x00;
  
	setup_millis();

	INTRQ = 0;
	__engint();                     // Enable global interrupts
	previoustime=millis();
	previouspinstate=1;
	state=0;
}


/*******************************************************************************
* individual pattern functions - with the number of repeats as a parameter
* singleledccw		singleledcw
* twoledsccw		twoledscw 
* twoledsflapdown	twoledsflapup 
* twoledsflap		twoledsrandom 
*/
void singleledccw(uint8_t n)
{
  	for (uint8_t r=0; r<n; r++)
	{
		for (int8_t i=0; i<20; i++)
		{
			setled(i,20);
                        if (waituntil(25)) {return; }
		}
		for (int8_t i=0; i<20; i++)
		{
			setled(20,19-i);
			if (waituntil(25)) {return; }
		}
	}
}

void singleledcw(uint8_t n)
{
  	for (uint8_t r=0; r<n; r++)
	{
		for (int8_t i=0; i<20; i++)
		{
			setled(20,i);
			if (waituntil(25)) {return; }
		}
		for (int8_t i=0; i<20; i++)
		{
			setled(19-i,20);
			if (waituntil(25)) {return; }
		}
	}
}

void twoledsccw(uint8_t n)
{
  	for (uint8_t r=0; r<n; r++)
	{
		for (int8_t i=0; i<20; i++)
		{
			setled(i,19-i);
			if (waituntil(25)) {return; }
		}
	}
}

void twoledscw(uint8_t n)
{
  	for (uint8_t r=0; r<n; r++)
	{
		for (int8_t i=0; i<20; i++)
		{
			setled(19-i,i);
			if (waituntil(25)) {return; }
		}
	}
}

void twoledsflapdown(uint8_t n)
{
  	for (uint8_t r=0; r<n; r++)
	{
		for (int8_t i=0; i<20; i++)
		{
			setled(i,i);
			if (waituntil(25)) {return; }
		}
	}
}

void twoledsflapup(uint8_t n)
{
  	for (uint8_t r=0; r<n; r++)
	{
		for (int8_t i=0; i<20; i++)
		{
			setled(19-i,19-i);
			if (waituntil(25)) {return; }
		}
	}
}

void twoledsflap(uint8_t n)
{
  	for (uint8_t r=0; r<n; r++)
	{
		for (int8_t i=0; i<20; i++)
		{
			setled(i,i);
			if (waituntil(25)) {return; }
		}
		for (int8_t i=0; i<20; i++)
		{
			setled(19-i,19-i);
			if (waituntil(25)) {return; }
		}
	}
}

uint8_t const randomsequence[]={ 9,19,3,2,16,17,6,18,1,8,0,14,15,5,7,10,11,12,4,10,10,1,15,8,17,9,6,16,7,13,11,0,2,3,4,18,12,14,5,19 };
void twoledsrandom(uint8_t n)
{
  	for (uint8_t r=0; r<n; r++)
	{
		for (int8_t i=0; i<20; i++)
		{
			setled(randomsequence[i],randomsequence[i+20]);
			if (waituntil(25)) {return; }
		}
		for (int8_t i=0; i<20; i++)
		{
			setled(randomsequence[i+20],randomsequence[i]);
			if (waituntil(25)) {return; }
		}
		for (int8_t i=0; i<20; i++)
		{
			setled(randomsequence[19-i],randomsequence[39-i]);
			if (waituntil(25)) {return; }
		}
		for (int8_t i=0; i<20; i++)
		{
			setled(randomsequence[39-i],randomsequence[19-i]);
			if (waituntil(25)) {return; }
		}
	}
}


/*******************************************************************************
* Arduino-like loop() function called from main()
*/
void loop()
{
	state++; // pre-switch-statement increment - if state is reset, this will immediately increment it, so first state is 1
	switch (state)
	{
		case 1:	singleledccw(4); break;
		case 2:	singleledcw(4); break;
		case 3:	twoledsccw(8); break;
		case 4:	twoledscw(8); break;
		case 5:	twoledsflapdown(8); break;
		case 6:	twoledsflapup(8); break;
		case 7:	twoledsflap(4); break;
		case 8:	twoledsrandom(4); break;
		default: // may not be rached by every badge
			syncpulse(25);
			state=0; // will immediately be incremented
		break;
	}
}


/*******************************************************************************
* Arduino-like setup() and loop() glue in main()
*/
void main()
{
	setup();
	while (1) loop();
}

