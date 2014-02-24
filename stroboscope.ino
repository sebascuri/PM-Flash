#include <MsTimer2.h>
#include <Bounce.h>
#include <TimerOne.h>
#include <LiquidCrystal.h>

/* Constants */
const unsigned int  PULSE_DURATION = 10; // micro seconds
const unsigned int INTERRUPT_PER = 100; // micro secodns
const unsigned long INTERRUPT_FREQ = 1000000/INTERRUPT_PER; // Hz
const unsigned int COUNTER = 30;
const unsigned int F_MIN = 1; // Hz
const unsigned int F_MAX = 50; // Hz
const unsigned int ADC_MIN = 0;
const unsigned int ADC_MAX = 1022;
const unsigned int DEBOUNCE_TIME = 5; // miliseconds


/*Variables*/
enum mode {
  external,
  internal
} 
state;

// Frequency Measure
float frequency = 50.0;
unsigned int rpm = 60;

volatile boolean doBlink = false;
volatile boolean readState = false;

unsigned long previous_time = 0;

/* PINS */
const int D7 = 8;
const int D6 = 7;
const int D5 = 6;
const int D4 = 5;
const int E = 4;
const int RS = 3;

const int outputPin = 13;
const int outputInternalLed = 12;
const int outputExternalLed = 11;

const int statePin = A1;   // pushbutton connected to digital pin 10
const int phasePin = A2;

const int ExternalPin = 2;
const int interrupt = 0; //Hard Coded

const int Potentiometer = A0;

/* Libraries */
LiquidCrystal lcd(RS, E, D4, D5, D6, D7);
Bounce phase = Bounce(phasePin, DEBOUNCE_TIME);


void setup() {
  noInterrupts();           // disable all interrupts

  /******************* INIT LCD **************/
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.print("2013   PM-31.58");
  lcd.setCursor(0,1);
  lcd.print("ITBA LAB FISICA"); 

  delay(100000);
  reset_lcd(); 

  /******************* INIT Pins **************/
  pinMode(outputPin, OUTPUT);
  pinMode(outputInternalLed, OUTPUT);
  pinMode(outputExternalLed, OUTPUT);

  pinMode(statePin, INPUT_PULLUP);      
  pinMode(phasePin, INPUT_PULLUP);
  pinMode(ExternalPin, INPUT_PULLUP);

  /******************* INIT Timer **************/

  Timer1.initialize(INTERRUPT_PER); //100 micro sec
  Timer1.attachInterrupt(isr_timer); 
  delay(10);

  /******************* INIT State **************/

  state = (mode) !digitalRead(statePin);
  delay(10);
  digitalWrite(outputInternalLed, state);
  digitalWrite(outputExternalLed, !state);
  delay(10);

  if (state == external){
    attachInterrupt(interrupt, isr_external_freq, FALLING);
    delay(10);
  }

  interrupts();

}

void loop(){

  if (doBlink){
    Blink();
    TakeReadings();
    send_data_to_lcd(frequency, rpm);
    doBlink = false;
  }  
  if (readState){
    readState = false;
    read_state();
  }

}


void isr_timer(){
  static unsigned int interrupts_counter = 100;

  switch (state){
  case internal: 
    if (! interrupts_counter){
      doBlink = true;
      interrupts_counter = (unsigned int) (INTERRUPT_FREQ/frequency);    
      if(phase.fallingEdge()){
        interrupts_counter += (interrupts_counter/10);
      } 
    }
    else{
      interrupts_counter--;
    }
    break;

  case external:
    break;
  }
  readState = true;

}

void isr_external_freq(){
  doBlink = true;  
}

void TakeReadings(){
  static unsigned long new_time;
  static unsigned int counter = 0;

  switch (state){
  case internal:
    frequency = fmap(analogRead(Potentiometer), ADC_MIN, ADC_MAX, F_MIN, F_MAX);
    phase.update();

    frequency = round_frequency(frequency);

    break;
  case external:  
    counter += 1;
    if (counter == COUNTER){
      counter = 0;
      new_time = millis();
      frequency = 10^6 / (new_time - previous_time) / COUNTER;
      previous_time = new_time; 
    }
    break;
  default:
    break;
  }     

  rpm = (unsigned int) (frequency * 60);


}

void read_state()
{
  mode new_state = (mode) !digitalRead(statePin);
  if (  new_state != state ){
    state = new_state;
    digitalWrite(outputInternalLed, state);
    digitalWrite(outputExternalLed, !state);
    previous_time = millis();
    reset_lcd();
  }

  if (state == internal){
    detachInterrupt(interrupt);
  }
  else{
    attachInterrupt(interrupt, isr_external_freq, FALLING);
  }
}

void send_data_to_lcd(float frequency_lcd, int rpm_lcd){ 
  char freq_str[16]; 
  char rpm_str[16]; 
  char freq_str_aux[16];

  dtostrf(frequency_lcd, 5, 2, freq_str_aux);

  sprintf(freq_str, "f = %s Hz    ", freq_str_aux); 
  sprintf(rpm_str, "N = %i rpm    ", rpm_lcd);


  lcd.setCursor(0,0); 
  lcd.print(freq_str);
  lcd.setCursor(0,1); 
  lcd.print(rpm_str);

}

void Blink(){  
  digitalWrite(outputPin, HIGH); //PULSE ON
  MsTimer2::set(PULSE_DURATION , NoBlink);
  MsTimer2::start(); 
}

void NoBlink(void){
  digitalWrite(outputPin, LOW); //PULSE OFF 
  MsTimer2::stop();
}
  
  

float fmap(float x, float x1, float x2, float y1, float y2)
{
  return ((x - x1) * (y2 - y1) / (x2 - x1) + y1);
}

float round_frequency(float raw_frequency){  
  return (round(raw_frequency * 20) / 20.0);
}

void reset_lcd( void ){
  lcd.clear();
  lcd.begin(16, 2); 
}









