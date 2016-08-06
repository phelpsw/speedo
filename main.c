#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

/**
 * Using the Timer0 8 bit counter, the input pin clocks the counter.  At the
 * control rate (10Hz) Timer0 is read.  Timer2 is setup to toggle TOCC0 and
 * TOCC1 pins when Timer2 matches the Output Compare Register.  When the
 * Timer0 value is read at the control rate, the corresponding OCR register
 * value is calculated based on the Timer2 clock rate and the desired
 * multiplier.  This sets the outputs to generate the desired output frequency.
 *
 * Board Labels:
 * PB2 - Tach in
 * PA7 - Speedo in
 * PA1 - Speedo out
 * PA2 - Tach out
 * PA3 - Cruise control out
 *
 * Repurposed Pin Names
 * PA3 - Input (Label: Cruise)
 * PA1 - Output1 (Label: Speedo)
 * PA2 - Output2 (Label: Tach)
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
 */

int main(void)
{
    // Set PA1 and PA2 as outputs
    DDRA |= _BV(DDA1); // TOCC0
    DDRA |= _BV(DDA2); // TOCC1

    // Set PA7, PB2, PA4, PA3 as inputs
    DDRA &= ~_BV(DDA3); // T0

    /*
     * Counter1 Configuration
     */

    // External rising edge clock from pin T0
    TCCR0B |= _BV(CS12);
    TCCR0B |= _BV(CS11);
    TCCR0B |= _BV(CS10);

    /*
     * Timer2 Configuration - Output Compare based pin toggle
     */

    // Toggle OCR2A/B on Compare Match
    TCCR2A |= _BV(COM2A0); // TOCC0 / PA1
    TCCR2A |= _BV(COM2B0); // TOCC1 / PA2

    // Enable Clear Timer on Output Compare mode
    TCCR2B |= _BV(WGM22);

    // Divide clock by 64, this gives us 65535/(8e6/64.) = 0.52sec before a
    // rollover occurs.  Providing us the maximum output resolution given a
    // 10Hz control cycle.
    TCCR2B |= _BV(CS21);
    TCCR2B |= _BV(CS20);
    const uint32_t timer2_clock = 8000000UL / 64UL;

    // Configure Output Compare A and B for TOCC0 and TOCC1 output
    TOCPMSA0 = _BV(TOCC0S1) | _BV(TOCC1S1);
    TOCPMCOE = _BV(TOCC0OE) | _BV(TOCC1OE);

    // Zero counters to begin
    TCNT0 = 0;
    TCNT2 = 0;

    while (1)
    {
        // Read counter0 value and zero it out
        uint32_t val = (uint32_t)TCNT0;
        TCNT0 = 0;

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
        // this, reset the counter.
        if (TCNT2 >= output_counts)
        {
            TCNT2 = 0;

            // Force output compare to trigger manually
            TCCR2C |= _BV(FOC2A);
            TCCR2C |= _BV(FOC2B);
        }

        // Set the output count period
        const uint16_t reg_val = (uint16_t)(output_counts >> 1);
        OCR2A = reg_val;
        OCR2B = reg_val;

        _delay_ms(100);
    }

    return 0;
}

