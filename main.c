#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

/**
 * The tach and speedo fire an interrupt on their rising edge.  The interrupt
 * handler stores the number of timer counts that have transpired in the period
 * between interrupts.  The period of a timer count is known so the effective
 * interrupt frequency can be calculated.  The function for converting a
 * tach / speedo sensor frequency to a speed / rpm is known which lets us
 * figure out how to properly display this information on the display panel.
 *
 * AVR203: http://www.atmel.com/Images/doc8365.pdf
 *
 * Reference Oscillator 8MHz
 *
 * PB2 - Tach in - PCINT10 or ICP2
 * PA7 - Speedo in - PCINT7 or ICP1
 * PA1 - Speedo out - TOCC0
 * PA2 - Tach out - TOCC1
 * PA3 - Cruise control out - TOCC2
 *
 *
 * Timer2 - 8MHz / 1024 -> 7812.5Hz
 * 7812.5 / 65535 -> 0.119Hz Resolution
 *
 * I swapped TOCC2 and TOCC3 because it maps to output compare registers in an
 * advantageous way.
 *
 * OC2B -> TOCC0, TOCC2 - Speedo Frequency Output
 * OC2A -> TOCC3 - Tach Frequency Output
 *
 * TOCPMSA1 = 0x00;
 * TOCPMSA0 = 0;
 * TOCPMSA0 = (1 << TOCC0S1) | (1 << TOCC2S1) | (1 << TOCC3S1);
 * TOCPMCOE = (1 << TOCC0OE) | (1 << TOCC2OE) | (1 << TOCC3OE);
 *
 * OCR2AH / OCR2AL - Output compare register 2A - Tach Freq
 * OCR2BH / OCR2BL - Output compare register 2B - Speedo Freq
 *
 *
 *
 * TODO: Map ICP1 -> ICR1 - Speedo
 * TODO: Map ICP2 -> ICR2 - Tach
 *
 * ICR1H/ICR1L - Input Capture Register 1 - Speedo
 * ICR1H/ICR2L - Input Capture Register 2 - Tach
 *
 *
 * TODO: In normal timer mode, do i need to update the OCR registers with the
 * new delta every time it is met?  If I run Clear Timer on Compare Match it
 * helps this situation but makes the input capture side of things complicated
 *
 * Using output compare in normal mode is not recomended!!
 *
 *
 *
 *
 *
 * Plan:
 *
 * Use Timer0 (8bit) output compares for two output channels (Normal)
 * Use Timer1/2 (16bit) input compare for two input channels (CTC)
 *
 * Use output compareA/B on timer0 for setting frequency
 *  - handle interrupt to update next target counter value
 *  - Pick lowest frequency interrupt source to handle reading timer1/2 and
 *  calculating associated timer0 OCR values
 *
 */

volatile unsigned short pass1 = 0;

int main(void)
{
    // Set PA1, PA2, and PA3 as outputs
    DDRA |= _BV(DDA1);
    DDRA |= _BV(DDA2);
    DDRA |= _BV(DDA3);

    // Set PA7 and PB2 as inputs
    DDRA &= ~_BV(DDA7);
    DDRB &= ~_BV(DDB2);

    /*
     * Timer1 Configuration - Input Capture ICP1 - Speedo
     */

    // Enable rising edge detection
    TCCR1B |= _BV(ICES1);

    // Enable noise canceler
    TCCR1B |= _BV(ICNC1);

    // Divide clock by 64
    TCCR1B |= _BV(CS11);
    TCCR1B |= _BV(CS10);

    // Clear pending interrupts
    TIFR1 = _BV(ICF1);

    // Enable Interrupt on input compare
    TIMSK1 |= _BV(ICIE1);

    /*
     * Timer2 Configuration - Output Compare Speedo / Cruise
     */

    // Toggle OCR2A/B on Compare Match
    TCCR2A |= _BV(COM2A0);
    TCCR2A |= _BV(COM2B0);

    // Enable Clear Timer on Output Compare mode
    TCCR2B |= _BV(WGM22);

    // Divide clock by 64
    TCCR2B |= _BV(CS21);
    TCCR2B |= _BV(CS20);

    // Configure Speedo Output Compare A (TOCC3 - Cruise) Compare B (TOCC0 - Speedo)
    //TOCPMSA0 = _BV(TOCC0S1) | _BV(TOCC1S1) | _BV(TOCC2S1);
    //TOCPMCOE = _BV(TOCC0OE) | _BV(TOCC1OE) | _BV(TOCC2OE);
    TOCPMSA0 = _BV(TOCC0S1) | _BV(TOCC1S1);
    TOCPMCOE = _BV(TOCC0OE) | _BV(TOCC1OE);

    sei();
    while (1)
    {
        // Enable input compare if an entire frequency estimation cycle has
        // already been completed.
        if (pass1 == 0)
        {
            TIMSK1 |= _BV(ICIE1);
            pass1 = 1;
        }
        _delay_ms(10);
    }

    return 0;
}

ISR (TIMER1_CAPT_vect)
{
    // Save the status register and disable interrupts
    unsigned char sreg = SREG;
    cli();

    if (pass1 == 2)
    {
        unsigned short val = ICR1;

        // Halve the period as we measured from rising edge to rising edge
        // but the output register is a half period.
        OCR2A = val >> 1;
        OCR2B = val >> 1;

        // Disable input capture
        pass1 = 0;

        // Clear the current interrupt flag and disable this interrupt
        TIFR1 = _BV(ICF1);
        TIMSK1 &= ~_BV(ICIE1);
    }
    else if (pass1 == 1)
    {
        // Ignore the first interrupt which hasn't allowed the timer to
        // increment fully to incorporate the entire period of the measured
        // frequency.
        pass1 = 2;
    }

    // Reset Timer1 counter
    TCNT1 = 0;

    // Restore Status Register and re-enable interrupts
    SREG = sreg;
    sei();
}

