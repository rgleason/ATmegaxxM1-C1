/*
  wiring_digital.c - digital input and output functions
  Part of Arduino - http://www.arduino.cc/

  Copyright (c) 2005-2006 David A. Mellis
  
     
  Modified to support ATmega32M1, ATmega64M1, etc.   Mar 2016  
        Al Thomason:   https://github.com/thomasonw/ATmegaxxM1-C1
                       http://smartmppt.blogspot.com/search/label/xxM1-IDE
                      



  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA

  Modified 28 September 2010 by Mark Sproul
*/

#define ARDUINO_MAIN
#include "wiring_private.h"
#include "pins_arduino.h"

void pinMode(uint8_t pin, uint8_t mode)
{
	uint8_t bit = digitalPinToBitMask(pin);
	uint8_t port = digitalPinToPort(pin);
	volatile uint8_t *reg, *out;

     
     
#if defined (__AVR_ATmega32C1__) || defined(__AVR_ATmega64C1__) || defined(__AVR_ATmega16M1__) || defined(__AVR_ATmega32M1__) || defined(__AVR_ATmega64M1__)

     
        uint8_t b = POC;                                                // ATmegaxxM1 has its fuses set to disable all three PCS channels
                POC = b;                                                // until there is a write to the POC register.  So, write back its contents
                                                                        // to free these pins.  (Thank you K. Nankivell for helping ID this issue)

    // AT: Setting up an Differential channel?
    // AT: Using very crude and simple re-entrant calling of pinMode() to pre-configure physical ports...
    if (pin == AD0){
        pinMode (9, INPUT);                                             // AMP0 is D9-D8
        pinMode (8, INPUT);
        
        DIDR1  |= (B00011000);                                          // Disable digitial inputs - to reduce noise
        AMP0CSR = _BV(AMP0EN) | ((mode & 0x03) << 4);                   // Enable Diff-Amp AD0:  Free running, enabled, and gain = passed 'mode'
        
    } else if (pin == AD1){
        pinMode (A4, INPUT);                                            // AMP1 is A4-A3
        pinMode (A3, INPUT);

        DIDR1  |= (B00000011);                                          // Disable digitial inputs - to reduce noise
        AMP1CSR = _BV(AMP1EN) | ((mode & 0x03) << 4);                   // Enable Diff-Amp AD1:  Free running, enabled, and gain = passed 'mode'
        
    } else if (pin == AD2){
        pinMode (10, INPUT);                                            // AMP2 is D10-A6
        pinMode (A6, INPUT);

        DIDR1  |= (B01000000);                                          // Disable digitial inputs - to reduce noise
        DIDR0  |= (B01000000); 
        AMP2CSR = _BV(AMP2EN) | ((mode & 0x03) << 4);                   // Enable Diff-Amp AD2:  Free running, enabled, and gain = passed 'mode'0
        
        
        
    } else {
        
        
        if ((pin ==  8) || (pin ==  9)) {                               // Oh oh, are they coming in an re-defining a port that was configured as Differential?
            AMP0CSR = 0;                                                // Yes, need to shut-down the Op Amp and release the input ports.
            DIDR1  &= ~(B00011000);                                     // Re-enable digitial inputs
        
        } else if ((pin == A3) || (pin == A4)) {
            AMP1CSR = 0; 
            DIDR1  &= ~(B00000011); 
           
        } else if ((pin == 10) || (pin == A6)) {
            AMP2CSR = 0; 
            DIDR1  &= ~(B01000000);     
            DIDR0  &= ~(B01000000);
        }  
    }
    
    
     if (pin == DAC_PORT) {      
        DACON &= ~((1<<DAEN) | (1<<DAOE));                           // If DAC pin is being reassigned, make sure to turn off the DAC and release the pin.
     }
     
        
        
      // AT: Continue with original code.
#endif


      
      
	if (port == NOT_A_PIN) return;

	// JWS: can I let the optimizer do this?
	reg = portModeRegister(port);
	out = portOutputRegister(port);

	if (mode == INPUT) { 
		uint8_t oldSREG = SREG;
                cli();
		*reg &= ~bit;
		*out &= ~bit;
		SREG = oldSREG;
	} else if (mode == INPUT_PULLUP) {
		uint8_t oldSREG = SREG;
                cli();
		*reg &= ~bit;
		*out |= bit;
		SREG = oldSREG;
	} else {
		uint8_t oldSREG = SREG;
                cli();
		*reg |= bit;
		SREG = oldSREG;
	}
}

// Forcing this inline keeps the callers from having to push their own stuff
// on the stack. It is a good performance win and only takes 1 more byte per
// user than calling. (It will take more bytes on the 168.)
//
// But shouldn't this be moved into pinMode? Seems silly to check and do on
// each digitalread or write.
//
// Mark Sproul:
// - Removed inline. Save 170 bytes on atmega1280
// - changed to a switch statment; added 32 bytes but much easier to read and maintain.
// - Added more #ifdefs, now compiles for atmega645
//
//static inline void turnOffPWM(uint8_t timer) __attribute__ ((always_inline));
//static inline void turnOffPWM(uint8_t timer)
static void turnOffPWM(uint8_t timer)
{
	switch (timer)
	{
		#if defined(TCCR1A) && defined(COM1A1)
		case TIMER1A:   cbi(TCCR1A, COM1A1);    break;
		#endif
		#if defined(TCCR1A) && defined(COM1B1)
		case TIMER1B:   cbi(TCCR1A, COM1B1);    break;
		#endif
		#if defined(TCCR1A) && defined(COM1C1)
		case TIMER1C:   cbi(TCCR1A, COM1C1);    break;
		#endif
		
		#if defined(TCCR2) && defined(COM21)
		case  TIMER2:   cbi(TCCR2, COM21);      break;
		#endif
		
		#if defined(TCCR0A) && defined(COM0A1)
		case  TIMER0A:  cbi(TCCR0A, COM0A1);    break;
		#endif
		
		#if defined(TCCR0A) && defined(COM0B1)
		case  TIMER0B:  cbi(TCCR0A, COM0B1);    break;
		#endif
		#if defined(TCCR2A) && defined(COM2A1)
		case  TIMER2A:  cbi(TCCR2A, COM2A1);    break;
		#endif
		#if defined(TCCR2A) && defined(COM2B1)
		case  TIMER2B:  cbi(TCCR2A, COM2B1);    break;
		#endif
		
		#if defined(TCCR3A) && defined(COM3A1)
		case  TIMER3A:  cbi(TCCR3A, COM3A1);    break;
		#endif
		#if defined(TCCR3A) && defined(COM3B1)
		case  TIMER3B:  cbi(TCCR3A, COM3B1);    break;
		#endif
		#if defined(TCCR3A) && defined(COM3C1)
		case  TIMER3C:  cbi(TCCR3A, COM3C1);    break;
		#endif

		#if defined(TCCR4A) && defined(COM4A1)
		case  TIMER4A:  cbi(TCCR4A, COM4A1);    break;
		#endif					
		#if defined(TCCR4A) && defined(COM4B1)
		case  TIMER4B:  cbi(TCCR4A, COM4B1);    break;
		#endif
		#if defined(TCCR4A) && defined(COM4C1)
		case  TIMER4C:  cbi(TCCR4A, COM4C1);    break;
		#endif			
		#if defined(TCCR4C) && defined(COM4D1)
		case TIMER4D:	cbi(TCCR4C, COM4D1);	break;
		#endif			
			
		#if defined(TCCR5A)
		case  TIMER5A:  cbi(TCCR5A, COM5A1);    break;
		case  TIMER5B:  cbi(TCCR5A, COM5B1);    break;
		case  TIMER5C:  cbi(TCCR5A, COM5C1);    break;
		#endif
	}
}

void digitalWrite(uint8_t pin, uint8_t val)
{
	uint8_t timer = digitalPinToTimer(pin);
	uint8_t bit = digitalPinToBitMask(pin);
	uint8_t port = digitalPinToPort(pin);
	volatile uint8_t *out;

	if (port == NOT_A_PIN) return;

	// If the pin that support PWM output, we need to turn it off
	// before doing a digital write.
	if (timer != NOT_ON_TIMER) turnOffPWM(timer);

	out = portOutputRegister(port);

	uint8_t oldSREG = SREG;
	cli();

	if (val == LOW) {
		*out &= ~bit;
	} else {
		*out |= bit;
	}

	SREG = oldSREG;
}

int digitalRead(uint8_t pin)
{
	uint8_t timer = digitalPinToTimer(pin);
	uint8_t bit = digitalPinToBitMask(pin);
	uint8_t port = digitalPinToPort(pin);

	if (port == NOT_A_PIN) return LOW;

	// If the pin that support PWM output, we need to turn it off
	// before getting a digital reading.
	if (timer != NOT_ON_TIMER) turnOffPWM(timer);

	if (*portInputRegister(port) & bit) return HIGH;
	return LOW;
}
