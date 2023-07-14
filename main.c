/*
 * File:   main.c
 * Author: Nils Edvardsson
 * Created 18 maj 2023, 21:51
 */
#pragma config FOSC = INTRCIO   // Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA4/OSC2/CLKOUT pin, I/O function on RA5/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF       // MCLR Pin Function Select bit (MCLR pin function is MCLR)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF       // Brown-out Reset Selection bits (BOR enabled)
#pragma config IESO = OFF        // Internal External Switchover bit (Internal External Switchover mode is enabled)
#pragma config FCMEN = OFF       // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is enabled)


#include <xc.h>
#define VOLTAGE_INPUT 10 // RB4 AN10
#define SELECT_INPUT 3  // RA4 AN3
#define UPDOWNINPUT PORTBbits.RB6
#define RESETINPUT PORTBbits.RB5
#define CLOCK_INPUT PORTAbits.RA2

// MODES
#define INSTANT_AD 0
#define CLOCKED_AD 1
#define COUNT_UP 2
#define COUNT_DOWN 3
#define ALTERNATE 4
#define NOISE 5
#define RANDWAVE 6
#define SINEWAVE 7
#define DIVIDER 8
#define SINE_DIVIDER 9
#define BOSSANOVA 10
#define ROCK1 11
#define DISCO1 12
#define RUMBA 13
#define WALTZ 14
#define ENKA 15

// wave selector "enums"
#define SELECT_LINEAR 10
#define SELECT_SINEWAVE 20
#define SELECT_EMPTY 0

unsigned char lock = 0;
unsigned char xorOn = 0;
unsigned char extInput = 0;
unsigned char tick = 0;
unsigned char funcNumber = 0;
unsigned char top = 63;
unsigned char topPlusOne = 64;
unsigned char extUD_noChange = 1;
unsigned char extRS_noChange = 1;

unsigned volatile char newValue = 0;
unsigned volatile char oldValue = 0;
unsigned volatile char index = 0;
unsigned volatile char up = 1;
unsigned volatile char preset[64];
unsigned volatile char rythmBeat = 16;
unsigned volatile char d_index=0;
unsigned volatile char variation = 0;

unsigned volatile int nze = 0b1000000000000000;

const unsigned char PulseTime = 128;

const unsigned char SineWaveLo[] = { 32,44,54,61,63,61,54,44,32,19,9,2,0,2,9,19};

__uint24 kick;
__uint24 snare;
__uint24 hihat;
__uint24 cymbal;
__uint24 rim;
__uint24 accent;
__uint24 check;

void resetRythm(void)
{
    check=1;
    check <<= (rythmBeat-1);
}
void selectRythm(void)
{
    switch(funcNumber)
    {
        case ROCK1:
		rythmBeat = 16;
		cymbal = 0;
		rim =   0b0000000100000000;
		snare = 0b0000101000001000;
		kick  = 0b1000000010000000;
		hihat = 0b1010101010101010;
		accent= 0b1000100010001000;
		break;
            
        case BOSSANOVA:
		rythmBeat = 16;
		snare = 0b0000000100000001;
		cymbal= 0b1000001000000100;
		hihat = 0b1111111111111111;
		rim = 0b1010010010100100;
		kick = 0b1001100110011001;
		accent = 0b0000100000001000;
		break;
            
        case DISCO1:
		rythmBeat = 16;
		rim = 0;
		cymbal = 0b1000000000001000;
		hihat  = 0b1010101010101010;
		snare  = 0b0000100000001000;
		kick   = 0b1000000110000000;
		accent = 0b1000100000001000;
		break;
            
        case RUMBA:
		hihat =    0b1111111110111111;
		snare =    0b0110110101101101;
		rim =      0b1001001010010010;
		cymbal   = 0b1001001000101000;
		kick    =  0b1000101010001010;
		accent  =  0b1000110000001000;
		rythmBeat = 16;
		break;
            
        case WALTZ:
		rythmBeat = 12;
		rim = 0;
		cymbal =    0b100000101000;
		snare  =    0b000010001000;
		kick   =    0b100000000000;
		hihat  =    0b001000100010;
		accent =    0b000000001000;
		break;
            
        case ENKA:
		rythmBeat = 16;
		hihat   = 0b1010101010101010;
		snare   = 0b0011001000110010;
		kick    = 0b1000100010001000;
		accent  = 0b1000000010000000;
		rim     = 0b0001000100010001;
		cymbal  = 0b0000000010000000;
		break;            
    }
    resetRythm();
}
void (*selectedFunction)(void);

void rythmGen(void)
{
    unsigned char drumOutput = 0;
    char d;
    if((cymbal & check)>0)drumOutput    |=0b100000; //cymbal
    if((accent & check)>0)drumOutput    |=0b010000; //accent
    if((snare & check) >0)drumOutput    |=0b001000; //snare
    if((kick & check)  >0)drumOutput    |=0b000100; //kick
    if((rim & check)   >0)drumOutput    |=0b000010; //rim
    if((hihat & check) >0)drumOutput    |=0b000001; //hihat
    check >>=1;
    if(check==0)
    {
        resetRythm();
    }
    drumOutput ^= extInput;
    PORTC = drumOutput;
    if(variation<0b1000) // if select pot is halfway to next rythm, switch of the pulse generation mode
    {
        for(d=0;d<PulseTime;d++){}; // short pulse
        PORTC = 0;
    }
    
}
void countIndex(void)
{
    if(up)
    {
        index++;
    }else{
        index--;
    }
    // if index is top plus one, then up is 1
    if(index==topPlusOne)
    {
        index=0;
    }
    //if index is way over top then index has passed thru zero and up is -1
    if(index>topPlusOne)
    {
        index=top;     
    }
}

void countUpDown(void)
{
    if(up){
       index++; 
    }else{
        index--;
    }
    if(index>=top)
    {
        up=0;
    }
    else if(index==0)
    {
        up=1;
    }
}

void clockedAD()
{
    ADCON0bits.GO_DONE = 1; // start a/d conversion
    while(ADCON0bits.GO_DONE){};
    index = ADRESH >> 2;
}


void noise(void)
{
    if(lock)
    {
        unsigned int LSB = nze & 0b1;
        nze >>=1;
        nze |= (LSB << 15);
    }else
    {
        unsigned int tap1 = (nze >> 12) & 0b1;
        unsigned int tap2 = (nze >> 7) & 0b1;
        unsigned int tap3 = (nze >> 4) & 0b1;
        unsigned int tap4 = (nze >> 1) & 0b1;
        unsigned int tap5 = tap4 ^ tap3;
        unsigned int tap6 = tap5 ^ tap2;
        nze >>= 1;
        nze |= ((tap1 ^ tap6) << 15);
    }
    index = nze & 0b111111;
    
}
void noiseWave(void)
{
    if(lock)
    {
        unsigned int LSB = nze & 0b1;
        nze >>=1;
        nze |= (LSB << 15);
    }else
    {
        char x = 0;
        unsigned int tap1, tap2, tap3, tap4, tap5, tap6;

        for(x=0; x < 2; x++)
        {
            tap1 = (nze >> 12) & 0b1;
            tap2 = (nze >> 7) & 0b1;
            tap3 = (nze >> 4) & 0b1;
            tap4 = (nze >> 1) & 0b1;
            tap5 = tap4 ^ tap3;
            tap6 = tap5 ^ tap2;
            nze >>= 1;
            nze |= ((tap1 ^ tap6) << 15);
        }
        lock = 1;
    }
    index = nze & 0b111111;
    
}

void divide()
{
    if(d_index>=extInput)
    {
        index++;
        d_index=0;
        if(index>top)
        {
            index=0;
        }
    }else{
        d_index++;
    }
}
void noFunction(void)
{
    
}


void initAD(void)
{
    ANSEL = 0b1000; //ANS3
    ANSELH = 0b100; //ANS10
    ADCON0bits.CHS = VOLTAGE_INPUT;
    ADFM = 0;   // ADRESH = msb 8-bit
    ADON = 1;   // activate A/D of the mcu
}
void __interrupt() myISR(void)
{
    INTF = 0;
    (*selectedFunction)();
    if(xorOn)
    {
        newValue = preset[index] ^ extInput;
    }else{
        newValue = preset[index];
    }
    if(oldValue != newValue)
    {
        PORTC = newValue;
        oldValue = newValue;
    }
}

void initPresetArray(char select_wave)
{
    char x;
    if(select_wave==SELECT_SINEWAVE)
    {
        for(x=0;x<16;++x)
        {
            preset[x] = SineWaveLo[x];
        }
    }
    if(select_wave==SELECT_LINEAR)
    {
        for(x=0;x<64;++x)
        {
            preset[x]=x;
        }
    }
}

void selectFunction()
{
    switch(funcNumber)
    {
        case CLOCKED_AD:
            selectedFunction = &clockedAD;
            initPresetArray(SELECT_LINEAR);
            xorOn = 0;
            break;
            
        case INSTANT_AD:
            selectedFunction = &noFunction;
            initPresetArray(SELECT_EMPTY);
            xorOn = 0;
            break;
            
        case COUNT_UP:
            selectedFunction = &countIndex;
            initPresetArray(SELECT_LINEAR);
            up = 1;
            xorOn = 1;
            top = 63;
            topPlusOne = 64;
            break;
            
        case COUNT_DOWN:
            selectedFunction = &countIndex;
            initPresetArray(SELECT_LINEAR);
            top = 63;
            topPlusOne = 64;
            up = 0;
            xorOn = 1;
            break;
            
        case ALTERNATE:
            selectedFunction = &countUpDown;
            initPresetArray(SELECT_LINEAR);
            top = 63;
            topPlusOne = 64;
            up = 1;
            xorOn = 1;
            break;
            
        case NOISE:
            selectedFunction = &noise;
            up = 1;
            xorOn = 1;
            index = 0;
            lock = 0;
            nze |= 0b1000100000000000;
            initPresetArray(SELECT_LINEAR);
            break;
            
        case RANDWAVE:
            selectedFunction = &noiseWave;
            xorOn = 1;
            index = 0;
            lock = 0;
            nze |= 0b1000100000000000;
            initPresetArray(SELECT_LINEAR);
            break;
                
        case SINEWAVE:
            selectedFunction = &countIndex;
            top = 15;
            topPlusOne = 16;
            initPresetArray(SELECT_SINEWAVE);
            up = 1;
            xorOn = 1;
            break;
            
        case DIVIDER:
            selectedFunction = &divide;
            top = 15;
            topPlusOne = 16;
            initPresetArray(SELECT_LINEAR);
            xorOn = 0;
            break;
            
        case SINE_DIVIDER:
            selectedFunction = &divide;
            top = 15;
            topPlusOne = 16;
            initPresetArray(SELECT_SINEWAVE);
            xorOn = 0;
            break;
            
        default:
            selectedFunction = &rythmGen;
            initPresetArray(SELECT_EMPTY);
            selectRythm();
            xorOn = 0;
            break;
    }
}
void main(void) {
    OSCCON = 0b1110001; // 8 MHz internal oscillator
    TMR2IE = 0; // timer 2 not interrupt enabled 
    RABIE = 0; // port B changes interrupt not enabled
    PEIE = 0; // peripheral interrupt not enabled
    TRISA = 0b00010100;
    TRISC = 0b00000000;
    TRISB = 0b01110000;
    PORTC = 0;
    initAD();
    INTE = 1; //enable RA2/INT interrupts
    INTEDG = 1; //rising edge
    GIE = 1; //  global interupt enable
    selectedFunction = &countIndex;
    initPresetArray(0);
    while(1)
    {
        ADCON0bits.GO_DONE = 1; // start a/d conversion
        
        if(UPDOWNINPUT && extUD_noChange)
        {
            up ^= 1;
            lock ^= 1;
            extUD_noChange = 0;
        }
        if(!extUD_noChange && !UPDOWNINPUT)
        {
            extUD_noChange = 1;
        }
        
        
        if(RESETINPUT && extRS_noChange)
        {
            if(funcNumber<10)
            {
                if(up)
                {
                    index=0;
                }else
                {
                    index=top;
                }
            }else{
                resetRythm();
            }
            extRS_noChange = 0;
        }
        if(!extRS_noChange && !RESETINPUT)
        {
            extRS_noChange = 1;
        }
        
        while(ADCON0bits.GO_DONE){};
        
        // instant AD is a special mode where the A/D samples
        // as fast as possible and outputs it
        if(funcNumber == INSTANT_AD)
        {
            newValue = ADRESH >> 2;
            if(oldValue != newValue)
            {
                PORTC = newValue;
                oldValue = newValue;
            }
        }else
        {
            extInput = ADRESH >> 2;
        }
        
        // this handles the selector pot
        if(tick % 16 == 0)
        {
            ADCON0bits.CHS = SELECT_INPUT; // select AN3
            char l;
            for(l=0; l < 8; l++){}
            ADCON0bits.GO_DONE = 1; // start a/d conversion
            while(ADCON0bits.GO_DONE){};
            char adResult = ADRESH;
            char newFuncNumber = (adResult >> 4) & 0b1111;
            
            // variation checks if the pot is halfway to the new setting.
            // this is used by the drum machine part to change from pulse
            // output to gate output
            variation = adResult & 0b1000;
            if(newFuncNumber != funcNumber)
            {
                funcNumber = newFuncNumber;
                selectFunction();
            }
            // don´t forget or the external 
            // input will not be sampled!
            // select AN10
            ADCON0bits.CHS = VOLTAGE_INPUT;
            for(l=0; l < 8; l++){}
        }
        tick++;
    }
    return;
}
