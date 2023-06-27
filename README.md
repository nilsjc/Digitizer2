# Digitizer2 (or The Pattern Generator)
Pattern generator for eurorack modular synthesizer. Create trigger patterns with a clock and analog input. With a potentiometer 16 functions can be selected.<br>
Based on the now obsolete PIC16F690, but could easily be ported to any similar MCU. No assembly language involved.<br>
The circuit could definitely be improved, especially the analog input stage and the DAC that is an discreet R-R2 ladder stage.<br>
### In/Outs
<ul>
  <li>clock input</li>
  <li>dynamic cv input</li>
  <li>reset input</li>
  <li>switch direction input</li>
  <li>6 trigger outputs</li>
  <li>1 analog CV out</li>
 </ul>
 
### Selectable Functions
<ul>
  <li>Instant 6-bit analog to digital conversion.</li>
  <li>Clocked 6-bit analog to digital conversion.</li>
  <li>Clock divider up/down/back and forth</li>
  <li>Clocked Sine Function</li>
  <li>Dynamic divider with square or sine function</li>
  <li>Noise</li>
  <li>Random Wave</li>
  <li>Six Druma Patterns from Roland CR-78</li>
</ul>
   
Some of the clocked functions also X-OR the result from the 6-bit analog conversion with the 6-bit output, like the drum patterns. 
This can create interesting and musical variations. The module can be used for both generating CV and audio signals.<br>

![Alt text](digitizer_schematic.jpg?raw=true "Schematic")<br>
Schematic.<br>
![Alt text](digitizer_prototype.jpg?raw=true "Picture of prototype")<br>
The prototype

  
