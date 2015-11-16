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
 * PA2 - Tach out - TOCC?
 * PA3 - Cruise control out - TOCC?
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

    /* Assign OCR2A val */
    OCR2A = val;

    /* Restore global interrupt flag */
    SREG = sreg;

    return;
}

void WriteOCR2B(unsigned int val)
{
    unsigned char sreg;

    /* Save global interrupt flag */
    sreg = SREG;

    /* Disable interrupts */
    cli();

    /* Assign OCR2B val */
    OCR2B = val;

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

    DDRA |= _BV(DDA1);
    DDRA |= _BV(DDA2);
    DDRA |= _BV(DDA3);

    DDRB = 0;

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
    TOCPMSA0 = (1 << TOCC0S1) | (1 << TOCC3S1);
    TOCPMCOE = (1 << TOCC0OE) | (1 << TOCC3OE);

    // Output compare mode works, it seems like ReadICR1 may always return a 0
    // enable noise rejection and solder wires to pad for test.
    unsigned int val = ReadICR1();
    WriteOCR2A(val);
    WriteOCR2B(val);

    // enable interrupts
    sei();
    while (1);

    return 0;
}

ISR (TIMER0_OVF_vect)
{
    //PORTA ^= (_BV(PA1) | _BV(PA2) | _BV(PA3));
    //unsigned int val = ReadICR1();
    //WriteOCR2A(val);
    //WriteOCR2B(val);
    sei();
}

