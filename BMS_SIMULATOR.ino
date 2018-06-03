
#include <stdint.h>
#define SEND_DATA 1
#define NOT_SEND_DATA 2
#define ERROR_CMD 3
#include <TimerOne.h>

int con_status = NOT_SEND_DATA; //When start data flow received, con_status changes
int cont = 0;
uint8_t aux[64];
uint8_t incomingData[64];
int numBytes;
static int counter=0;
static unsigned long line_sent_counter=0;

int in_server_pwm_ec=30;
int in_server_pwm_ec2=29;

void setup() {

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(3, INPUT);
  Serial.begin(9600);
  while (!Serial) {
    ;
  }

 // Timer1.initialize(1000000); //   USE THE MULTIPLIER TO GET FACTORS of 5 secs (BMS_TIMER_MAX)
//  Timer1.attachInterrupt( handle_request_timer_interrupt); 

 
}

void loop() {
  


  cont = 0;
  while (Serial.available() > 1) {
    cont = Serial.readBytes(incomingData, 64);
  }
 
  //TESTING THE ENGINE CONTROLER
   for(int i=0;i<9;i++){
         Serial.print(i);
         delay(1000);
   }
  // Serial.println(0xAA);

  ////////////////////////////////

 
  
  if (incomingData[0] == '0' && incomingData[1] == '1' && cont == 2) {
    //digitalWrite(4, HIGH);
  //  Serial.print("\n\n\nSTART CMD RECEIVED \n\n\n");
    incomingData[0] = 0;
    incomingData[1] = 0;
    con_status = SEND_DATA;
  } else if (incomingData[0] == '0' && incomingData[1] == '0' && cont == 2) {
    //digitalWrite(4, HIGH);
  //  Serial.print("\n\n\nSTOP CMD RECEIVED\n\n\n");
    incomingData[0] = 0;
    incomingData[1] = 0;
    con_status = NOT_SEND_DATA;
  }

  if (con_status == SEND_DATA) {

    line_sent_counter++;

    if(line_sent_counter>=1 && line_sent_counter<=24){   // CELL VOLT 
            aux[0]=(line_sent_counter/10) + '0';
            aux[1]=( line_sent_counter-((line_sent_counter/10)*10)  ) + '0';
            aux[2]=random(48, 58);
            aux[3]=random(48, 58);
            aux[4]=random(48, 58);
      }else if(line_sent_counter>=25 && line_sent_counter<=28){  //TEMPS 
           aux[0]='7';
            aux[1]='4';
            aux[2]=line_sent_counter - 25 + '0';        
            aux[3]=random(48, 58);
            aux[4]=random(48, 58);
      }else if(line_sent_counter==29){
            aux[0]='8';
            aux[1]='4';
            aux[2]=random(48, 58);
            aux[3]=random(48, 58);
            aux[4]=random(48, 58);
      }else if(line_sent_counter==30){  //SOC
            aux[0]='9';
            aux[1]='4';
            aux[2]='0';
            aux[3]=random(48, 58);
            aux[4]=random(48, 58);
      }else if(line_sent_counter==31){ // PWM BatChar 
            aux[0]='9';
            aux[1]='4';
            aux[2]='3';
            aux[3]=random(48, 58);
            aux[4]=random(48, 58);
      }else if(line_sent_counter==32){ // PWM BatChar 
            aux[0]='9';
            aux[1]='9';
            aux[2]='0';
            aux[3]=random(48, 58);
            aux[4]=random(48, 58);
      }else if(line_sent_counter>32 &&  line_sent_counter<35 ){   //RANDOM DATA to simulate errors or useless data
            aux[0]=random(48, 58);
            aux[1]=random(48, 58);
            aux[2]=random(48, 58);
            aux[3]=random(48, 58);
            aux[4]=random(48, 58);
      }else{
            aux[0]=random(48, 58);
            aux[1]=random(48, 58);
            aux[2]=random(48, 58);
            aux[3]=random(48, 58);
            aux[4]=random(48, 58);
             line_sent_counter=0;
      }
      
    aux[5]='\n';
    Serial.write(aux,6);
    delay(1000);
    }



}
