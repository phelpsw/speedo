#include <avr/io.h>
#include <avr/interrupt.h>

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
 * PA3 - Tach out - TOCC2
 * PA4 - Cruise control out - TOCC3
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
char toggle;

unsigned int ReadICR1(void)
{
    unsigned char sreg;
    unsigned int i;

    /* Save global interrupt flag */
    sreg = SREG;

    /* Disable interrupts */
    cli();

    /* Read ICR1 into i */
    i = ICR1;

    /* Restore global interrupt flag */
    SREG = sreg;

    return i;
}

void WriteOCR2A(unsigned int val)
{
    unsigned char sreg;

    /* Save global interrupt flag */
    sreg = SREG;

    /* Disable interrupts */
    cli();

    /* Assign OCR2 val */
    OCR2A = val;

    /* Restore global interrupt flag */
    SREG = sreg;

    return;
}


int main(void)
{

    // TCCR0A - Compare Output Mode, non-PWM Mode 11.9.2 pg 85
    // (1 << COM0A1) | (0 << COM0A0) - Toggle OC0A on Compare Match
    //
    // Waveform Generation Mode - Normal pg 87
    // (0 << WGM02) | (0 << WGM01) | (0 << WGM00) 

    // Set PA1, PA3 and PA4 as outputs
    // Set PA7 as input
    DDRA = 0b00011010;
    PORTA = 0x0;

    // Set PB2 as input
    DDRB = 0x0;

    toggle = 0;

    // Overflow should occur at 122Hz
    // Divide clock by 256
    TCCR0B |= _BV(CS02);

    // Clear TOV0 / clear pending interrupts
    TIFR0 = _BV(TOV0);

    // Interrupt on overflow
    TIMSK0 |= _BV(TOIE0);

    /*
     * Timer1 Configuration - Input Capture ICP1 - Speedo
     */

    // TODO: Consider noise canceler

    // Enable rising edge detection
    TCCR1B |= _BV(ICES1);

    // Enable Clear Timer on Input Compare mode
    TCCR1B |= _BV(WGM12);
    TCCR1B |= _BV(WGM13);

    // Divide clock by 64
    TCCR1B |= _BV(CS11);
    TCCR1B |= _BV(CS10);

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
    //TOCPMSA0 = (1 << TOCC0S1) | (1 << TOCC3S1);
    //TOCPMCOE = (1 << TOCC0OE) | (1 << TOCC3OE);


    // enable interrupts
    sei();
    while (1);
    {
        // we have a working timer
    }
    return 0;
}

ISR (TIMER0_OVF_vect)
{
    // action to be done every 250us
    if (toggle == 1)
    {
        PORTA = 0b00011010;
        toggle = 0;
    }
    else
    {
        PORTA = 0b00011010;
        toggle = 1;
    }
}

