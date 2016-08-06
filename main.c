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
 * Low frequency reproduction is important, Speedometer 18Hz input is ~6mph
 * Input Frequencies
 * - Speedometer 0 - 450Hz
 * - Tachometer 0 - 565Hz
 *
 * Multipliers
 * - Speedometer 5 or 53/10
 * - Tachometer 4/3
 *
 * Output Frequencies
 * - Speedometer 0 - 3000Hz
 * - Tachometer 0 - 600Hz
 *
 *
 * Without filtering, minimum resolution is set by update rate.  So 10Hz update
 * 10 * (6.28 / 18.86) = 3.3mph
 * 10 * (10 / 18.86) = 5.3kph
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


/*
 * Step 1 At 10Hz Read counter value
 * Step 2 Add counter value to 10 slot circular buffer which pops one value off
 *        the end, subtracts it from a total, then adds the new value on.
 * Step 3 If counter value is > ~10, use the counter directly, otherwise use
 *        rolling average to approximate speed
 */

int main(void)
{
    // Set PA1, PA2, and PA3 as outputs
    DDRA |= _BV(DDA1); // TOCC0
    DDRA |= _BV(DDA2); // TOCC1
    //DDRA |= _BV(DDA3); // TOCC2

    // Set PA7, PB2, PA4, PA3 as inputs
    //DDRA &= ~_BV(DDA7);
    //DDRB &= ~_BV(DDB2);
    DDRA &= ~_BV(DDA3); // T0
    DDRA &= ~_BV(DDA4); // T1

    /*
     * Counter1 Configuration
     */

    // External rising edge clock from pin T1
    TCCR1B |= _BV(CS12);
    TCCR1B |= _BV(CS11);
    TCCR1B |= _BV(CS10);

    /*
     * Timer2 Configuration - Output Compare based pin toggle
     */

    // Toggle OCR2A/B on Compare Match
    // TODO: only one
    TCCR2A |= _BV(COM2A0);
    TCCR2A |= _BV(COM2B0);

    // Enable Clear Timer on Output Compare mode
    TCCR2B |= _BV(WGM22);

    // Divide clock by 64, this gives us 65535/(8e6/64.) = 0.52sec before a
    // rollover occurs.  Providing us the maximum output resolution given a
    // 10Hz control cycle.
    TCCR2B |= _BV(CS21);
    TCCR2B |= _BV(CS20);
    const uint32_t timer2_clock = 8000000UL / 64UL;

    // Configure Speedo Output Compare A (TOCC3 - Cruise) Compare B (TOCC0 - Speedo)
    //TOCPMSA0 = _BV(TOCC0S1) | _BV(TOCC1S1) | _BV(TOCC2S1);
    //TOCPMCOE = _BV(TOCC0OE) | _BV(TOCC1OE) | _BV(TOCC2OE);
    TOCPMSA0 = _BV(TOCC0S1) | _BV(TOCC1S1);
    TOCPMCOE = _BV(TOCC0OE) | _BV(TOCC1OE);

    // Zero counters to begin
    TCNT1 = 0;
    TCNT2 = 0;

    while (1)
    {
        // Read counter1 value and zero it out
        uint32_t val = (uint32_t)TCNT1;
        TCNT1 = 0;

        // Counts per second assuming 10Hz cycle
        val *= 10;

        // Calculate new output frequency
        val *= 53;
        val /= 10;

        // Calculate ticks at Timer2 clock rate
        uint32_t output_counts = 65535;
        if (val > 0)
            output_counts = timer2_clock / val;


        // If output_counts is less than the current counter value, the counter
        // is forced to roll-over before it will result in a toggle.  To prevent
        // this, reset the counter.  Resetting the counter will not correctly
        // invert the pin however
        if (TCNT2 >= output_counts)
        {
            TCNT2 = 0;
            // TODO Figure out how to invert TOCC pin manually
        }

        // Set the output count period
        // TODO: only use 1 of the two
        const uint16_t reg_val = (uint16_t)(output_counts >> 1);
        OCR2A = reg_val;
        OCR2B = reg_val;

        _delay_ms(100);
    }

    return 0;
}

