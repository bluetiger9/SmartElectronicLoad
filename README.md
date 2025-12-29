# SmartElectronicLoad
A smart electronic load


## Component Selection

1. Current Sense Amplifier OpAmp:
   - amplifies the voltage drop from the current sense resistors
   - low input offset (<=1mV, lower is better) - offset is amplified with the same amount as the sense current
   - high gain bandwidth product (>=5-10MHz, higher is better) - can limit the overall bandwidth of the load - for ex. 5MHz GBP with 10x amplification results in a 0.5Mhz effective bandwidth
   - input and output must go down to negative rail (0V)
   - input and output may not go up to the positive rail (3V3/12V) - in this case current range is reduced according to the upper limits
   - supply: 0-3V3 (or 0-12V)

2. Load Set OpAmp:
   - controls the gate of the load MOSFETs
   - input and output must go down to the negative rail (0V)
   - gain bandwidth product (>=1MHz) - can limit the overall bandwidth of the load
   - supply: 0-12V

3. Voltage Sense Buffer / Current Sense Buffer OpAmps
   - acts as a buffer for the voltage sense signals
   - input and output must go down to the negative rail (0V)
   - input and output may not go up to the positive rail (3V3/12V) - in this case voltage / current sense ranges are reduces accordingly
   - supply: 0-3V3 (or 0-12V)

4. DAC Buffer OpAmp
   - acts as a buffer for the DAC output
   - input and output must go down to the negative rail (0V)
   - input and output may not go up to the positive rail (3V3/12V) - in this case current range is reduced according to the upper limits
   - supply: 0-3V3 (or 0-12V)

5. MOSFET-s
   - acts as the active dissipates power element
   - N-channel MOSFET in TO-247 or TO-220 package
   - thermal resistance (lower is better) drive heat dissipation capacity
   - switching speed can affect the overall load bandwidth
   - drain source voltage limits the maximum voltage of the load
   - ON state resistance limits the maximum current at low voltages
   - can be in isolated or non-isolated package - non-isolated need additional parts to avoid exposing the load voltage on the heat sink

6. Current Sense Resistor
   - used to measure the current dissipated by the load
   - should be in the range of 5mV-100mΩ - resistor value in combination with multiplication factor gives the current range of the load - ex. 10mΩ + 10x (@3.3V) results in a 0-33A range (per channel)
   - SMD-2512 package
   - two resistors can be placed in parallel to the lower heat dissipation (which causes drift)

7. ESP32 SoC
   - the "brain" of the load
   - ESP32-S3-WROOM-1 modules (or ESP32-S2-SOLO-2, not recommended as it only has one core, and the built in DAC is no longer used)
   - 4MB+ flash, optional PSRAM (Quad SPI only - Octal SPI variants disable some GPIO pins)


## Bandwidth / Stability Tunning:
1. C1/R9 (and C2/R10) sets the overall bandwidth of the load
   - ex. 330Ω with 4.7nF results in a bandwidth of a ~100kHz
   - bandwidth can be increased by changing C1/C2 (or R9/R10) - ex 1nF achieves a bandwidth ~480kHz
     (*note: the current sense and load set OpAmps must keep up)

2. R65 (and R69) sets the MOSFETs gate current
   - default value is 10Ω
   - in case of oscillations, changing this resistor to a higher (ex. 47Ω) or lower (?) value (ex. 1Ω) may help stabilizing the load