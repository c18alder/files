#include <cdcacm.h>
#include <usbhub.h>
#include <TimerOne.h>
#include "pgmstrings.h"

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>


#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) Serial.print(__VA_ARGS__)
#define PRINTFLN(...) Serial.println(__VA_ARGS__)
#else
#define PRINTF(...)
#define PRINTFLN(...)
#endif

//DEFINES FOR SENDING CMDs TO BMS AND READING
#define SEND_START_COMMAND 1
#define SEND_STOP_COMMAND 2
#define AMOUNT_LINES_READ_BMS 65  //AMOUNT DATA TO READ FROM BMS EACH TIME ( bms provides about 33 variables... so 45 is a conservative number to secure )

//KEY VALUE FOR THE RESOURCE TO BE SENT :
#define CAR_SPEED_KEY "km/h"
#define CAR_SoC_KEY "%bat"
#define CAR_KILOMETRAGE_KEY "kms"
#define PWM_EC_KEY "pec"   //PWM Engine Controler
#define PWM_BC_KEY "pbc"  //PWM Battery Charger 
#define TEMP_1_KEY "ta"
#define TEMP_2_KEY "tb"
#define TEMP_3_KEY "tc"
#define TEMP_4_KEY "td"
#define CELL_VOLT_KEY "cs" // a "." separates the cell value and number---> ej:  cs=18.649    (cell value is not devided by 200 yet (done in server) TODO : maybe do it here.. 


//DESCRIPTORS FOR PARSING DATA
#define Tsensor_char1 7
#define Tsensor_char2 4  //Temp Sensors 74
#define Tsensor_0 0
#define Tsensor_1 1
#define Tsensor_2 2
#define Tsensor_3 3
#define Sensor_NC_char0 9    //Sensor Not Conected 99
#define Sensor_NC_char1 9

#define Icurrent_char1 8
#define Icurrent_char2 4  //Current out of battery 84

#define SoC_char1 9       //SoC of the Battery   94
#define SoC_char2 4
#define SoC_perc 0   //% battery 
#define SoC_PWM 3  //  PWM Battery Charger


#define PWM_char1 9   //PWM Engine controller
#define PWM_char2 9

/////////////////////////////////////////////////RESOURCES ID ////////////////////
#define FIRST_CELL_VOLTAGE_ID 1
#define LAST_CELL_VOLTAGE_ID 24
#define FIRST_TEMPERATURE_SENSOR_ID 25
#define LAST_TEMPERATURE_SENSOR_ID 28
#define PWM_EC_ID 29
#define BATTERY_SOC_ID 30
#define PWM_BATTERY_CHARGER_ID 31
#define CAR_SPEED_ID 32
#define CAR_KILOMETRAGE_ID 33

///////////////BMS RESOURCES INFORMATION////////////////////////////////
#define BMS_BYTES_PER_LINE 6   //Including the  "\n" (new line)
#define NUM_OF_IDS 34   // each cell is one.
#define NUM_CELLS 24
#define NUM_TEMPERATURES 4

//TOLERANCE FOR UPDATING SERVER RESOURCES 

#define CELLS_NOTIFY_TOLERANCE 0.8   // 1 -> 100 %         .. don't notify SERVER if the change is not bigger than this .
#define TEMPERATURES_NOTIFY_TOLERANCE 0.8
#define PWM_EC_NOTIFY_TOLERANCE 0.8
#define BATTERY_SOC_NOTIFY_TOLERANCE 0.8
#define PWM_BATTERY_CHARGER_NOTIFY_TOLERANCE 0.8 
#define CAR_SPEED_NOTIFY_TOLERANCE 0.8
#define CAR_KILOMETRAGE_NOTIFY_TOLERANCE 0.8

//DEFINES FOR BMS TIMER INTERRUPT CONTROL AND CRITERIAS
#define BMS_TIMER_MULTIPLIER 2
#define BMS_TIMER_INTERRUPT 7000000   // X  seg
#define BMS_REQUEST_TIME BMS_TIMER_INTERRUPT*BMS_TIMER_MULTIPLIER


//SOME MATH FOR REQUESTING DISTANCE BASED ON SPEED  ( DEPENDS OF PRESICION WILLING TO USE IN THE DISPLAY ) 
#define PERIOD_KILOMETRAGE_LOW_SPEED ( 36/(BMS_REQUEST_TIME/1000000) )  // ( amount of timer interrupts to wait until requesting the kilometrage if driving less than 50km/h = 1 km every 72s )
#define PERIOD_KILOMETRAGE_HIGH_SPEED ( 15/(BMS_REQUEST_TIME/1000000) )  // ( amount of timer interrupts to wait until requesting the kilometrage if driving more than 50km/h.  120Kh/h = 1 km every 30s )  

// OTHER DEFINES 
#define FULL_UPDATE_MOMENT BMS_TIMER_MULTIPLIER*3   //every X bms request loop.
#define  EACH_LINE_AVG 10 //Milisecs average for BMS to send a line ( check in car )
#define ATTEMPS_TO_SEND_CMD 5  // ATTEMPS OF SENDING START OR STOP CMD BEFORE CONSIDERING THE BMS CRASHED. 
#define BMS_CRASHED_AFTER AMOUNT_LINES_READ_BMS*EACH_LINE_AVG*10     // milisecs to wait while data loop is being received to consider it crashed.       

#define CAR_IS_CHARGING 95


//NOTICE : Data is currently stored as Integers/Float and serialized before being sent. This is in order to implement the relative change analysis and 
//future value checks control (for example ( 0<SoC<100 ) and to have a more useful format of the data here to operate .

//Static variables to store the last parameters received ( if correct )  
static float cells_voltage[NUM_CELLS];
static int sensors_temp[NUM_TEMPERATURES];
static int pwm_ec = 0;
static int battery_SoC = 0;
static int PWM_battery_charger = 0;
static int battery_current = 0;
static int car_speed = 0;
static int car_kilometrage = 0;

//Static variables to store the previous parameters received ( the ones currently stored in the server )

static float in_server_cells_voltage[25];
static int in_server_sensors_temp[5];
static int in_server_pwm_ec = 0;
static int in_server_battery_SoC = 0;
static int in_server_PWM_battery_charger = 0;
static int in_server_car_speed = 0;
static int in_server_car_kilometrage = 0;

////AUX Variables/////
//VOLATILE FOR GUARATING PROPER INTERRUPT DATA TO THE MAIN LOOP ( CONSECUTIVE INTERRUPTS, for ex) 
volatile static int cont_bytes_total = 0;
volatile static  char  buf_aux[BMS_BYTES_PER_LINE];
volatile static int datain_counter = 0;        //counts lines received. Used to get a loop of data from BMS ( more than a loop be secure )
volatile static int atoi_buf[5];               //array used to handle the current line being received.           
volatile static int timer_interrupt_counter_EC = 0;
static volatile short int bms_timer_multiplier_counter=0;
volatile static short int full_update_trigger = 0; //when this reaches a X value, we send the whole data to server, disregarding changes or not.
static long aux_counter=0;
//FLAGS 
volatile static bool flag_bms_was_disconected=false;
volatile static bool flag_need_request=false; 
volatile static bool flag_timer_on=false;
volatile static bool flag_bms_crashed=false;
volatile static bool flag_reading_bms=false;
volatile static bool flag_first_read=false;
volatile static bool flag_first_connection=true;
volatile static bool flag_previously_connected=false;

 //function to parse the data and stores it in the corresponding static INT/FLOAT variable.
void parse_store_data(int *buffer) {            

  /////////////////////////////////////TEMOERATURE////////////////////////////////////////////////

  if (buffer[0] == Tsensor_char1 && buffer[1] == Tsensor_char2) {

     sensors_temp[ buffer[2] ] = (buffer[3] * 10 ) + buffer[4] ;
     
  }
  /////////////////////////////////////CURRENT/////////////////////////////////////////////////
  if (buffer[0] == Icurrent_char1 && buffer[1] == Icurrent_char2) {

  }
  /////////////////////////////////////SoC////////////////////////////////////////////////////
  if (buffer[0] == SoC_char1 && buffer[1] == SoC_char2) {
    switch ( buffer[2]) {
      case SoC_perc:
      case 1:   //to include the 100%
        battery_SoC = (buffer[2] * 100) + (buffer[3] * 10) + buffer[4];
        break;
        
      case SoC_PWM:
        PWM_battery_charger = (buffer[3] * 10) + buffer[4];
        break;
      default:
      ;                                       // TODO maybe implement some action if many useless data received
    }
  }

  /////////////////////////////////////PWM////////////////////////////////////////////////////
  if (buffer[0] == PWM_char1 && buffer[1] == PWM_char2) {
    if (buffer[2] == 0) {
      pwm_ec =  (buffer[3] * 10) + buffer[4];

    } else {
      //  Serial.print("\nPWM Descritor sent (99), but error in 3rd bit, not '0');
    }
  }
  /////////////////////////////////////VOLTAGE CELLS///////////////////////////////////////////
  if (  (buffer[0] * 10 + buffer[1]) >= 1 && (buffer[0] * 10 + buffer[1]) <= 24 ) { // IF it is in between 1 and 24
    float aux_buffer = 0;
    aux_buffer = ( (buffer[2] * 100) + (buffer[3] * 10) + buffer[4] ) / 200.0;
    cells_voltage[  (buffer[0] * 10 + buffer[1]) - 1  ] = aux_buffer;
  }

}
///////////END OF PARSE AND STORE FUNCTION/////////////////



//////FUNCTIONS FOR SETTING TO SET USB CONNECTION PARAMETERS/////////////
class ACMAsyncOper : public CDCAsyncOper
{
  public:
    uint8_t OnInit(ACM *pacm);
};

uint8_t ACMAsyncOper::OnInit(ACM *pacm)
{
  uint8_t rcode;
  // Set DTR = 1 RTS=1
  rcode = pacm->SetControlLineState(3);

  if (rcode)
  {
    ErrorMessage<uint8_t>(PSTR("SetControlLineState"), rcode);
    return rcode;
  }

  LINE_CODING  lc;
  lc.dwDTERate  = 9600;
  lc.bCharFormat  = 0;
  lc.bParityType  = 0;
  lc.bDataBits  = 8;

  rcode = pacm->SetLineCoding(&lc);

  if (rcode)
    ErrorMessage<uint8_t>(PSTR("SetLineCoding"), rcode);

  return rcode;
}
////// ///////////////////////////////////////////////////////////////////////


///USB CLASS///
USB     Usb;
//USBHub     Hub(&Usb);
ACMAsyncOper  AsyncOper;
ACM           Acm(&Usb, &AsyncOper);
/////////////////////////////////////////////////////////////////////////////

/////////////FUNCTION THAT SEND START AND STOP COMMANDS 
//IF USER ENTERS instruction to (to debug manually ). or  automatically by default. //////////////

uint8_t send_start_stop_cmd(short int start_or_stop) {
  //delay(10);
  short int atmp_counter=0;
  uint8_t rcode = 0;
  uint8_t start_cmd[2] = {'0', '1'}; //Start CMD
  uint8_t stop_cmd[2] = {'0', '0'}; //Stop CMD
  if (start_or_stop == SEND_START_COMMAND && (flag_need_request || flag_first_read) ) {
          while(  atmp_counter < ATTEMPS_TO_SEND_CMD ){
                rcode = Acm.SndData(2, start_cmd);
                if (!rcode) {
                  flag_reading_bms = true;
                  break;
                }else{
                  atmp_counter++;
                  delay(30*atmp_counter);  // to give a better chance.  
                }
                
          }
    
   
    

  } else if (start_or_stop == SEND_STOP_COMMAND && (flag_need_request || flag_first_read ) ) {
        while(  atmp_counter < ATTEMPS_TO_SEND_CMD ){
                
                rcode = Acm.SndData(2, stop_cmd);      //send stop cmd to BMS.
                if (!rcode) {
                  flag_reading_bms = false;
                  break;
                }else{
                   atmp_counter++;     
                   delay(30*atmp_counter);  // to give a increising  better chance
                }
        
        }
  
  }
  if ( atmp_counter >=  ATTEMPS_TO_SEND_CMD){
      flag_bms_crashed=true;    //if after so many attempts it didn't work, probably crashed.
  }
  return  rcode;
}
/////////////////////////////////////////////////////

///////////////////////////FUNCTION TO ENSURE 5 BYTES LINES/////////////////////////////////////////////
//static buf_aux is used.( This is the static buffer used to store the incoming bytes until the 5 of the line is reached
//because Arduino is not really trustworthy that it will read 5 bytes at a time.. Sometimes it reads 2 and in the next loop 3 and the information would be lost after each loop if not static.
//THIS IS NECESSARY ( or at least a good precaution) BECAUSE THE BUFFER READ IS REALLY CRAPY (IT Doesn't always read 5 at a time) . This ensures the 5 bytes after the \n.
void wait_full_line_and_store(char *buf, int received) {

  for (uint16_t i = 0; i < received; i++ ) {

    if (cont_bytes_total > 0 && cont_bytes_total < (BMS_BYTES_PER_LINE) && buf[i] != '\n' ) {
      buf_aux[cont_bytes_total - 1] = buf[i];
      cont_bytes_total++;
    } else if ( cont_bytes_total == (BMS_BYTES_PER_LINE) &&  buf[i] == '\n') {
      //Convert the 5 byte char array into a 5 byte integer array

      atoi_buf[0] = buf_aux[0] - '0';    //GUILLUME TODO, make it pretty!   This is the ATOI part.
      atoi_buf[1] = atoi(buf_aux + 1) / 1000;
      atoi_buf[2] = atoi(buf_aux + 2) / 100;
      atoi_buf[3] = atoi(buf_aux + 3) / 10;
      atoi_buf[4] = atoi(buf_aux + 4);

      parse_store_data(atoi_buf);

      cont_bytes_total = 0;
      datain_counter++;
      
      // TO DEBUG PRINT HERE WHAT DATA IS IT PARSING FROM THE BMS
     // for (int j = 0; j < 5; j++) {
        //Serial.print(buf_aux[j]);
     // }

    } else if (cont_bytes_total == (BMS_BYTES_PER_LINE) &&  buf[i] != '\n') {
      cont_bytes_total = 0; // some error in the BMS received data. Look for next data.
    }
    if (buf[i] == '\n') {
      cont_bytes_total = 1;
    }

  }   
}
///////////////////////////////////////////////////////////////////////


//ANALYZES IF any data has changed enough to update the server ( BASED ON % change ) ... returns the corresponding resourse ID. 0 otherwise.


int compare_server_vs_new(int id_counter) {   

  if (id_counter >= FIRST_CELL_VOLTAGE_ID && id_counter <= LAST_CELL_VOLTAGE_ID) {                        //TODO: different criterias can be implemented for each resource tolerance.


    if ( ( (in_server_cells_voltage[id_counter - 1] * (1 - CELLS_NOTIFY_TOLERANCE) ) >= cells_voltage[id_counter - 1] ) || ( (in_server_cells_voltage[id_counter - 1] * (1 + CELLS_NOTIFY_TOLERANCE) ) <= cells_voltage[id_counter - 1] ) ) {

      if ( !(in_server_cells_voltage[id_counter - 1] == 0 &&  cells_voltage[id_counter - 1] == 0) ) {
        return true;
      }
    } else {

      return false;
    }

  } else if (id_counter >= FIRST_TEMPERATURE_SENSOR_ID && id_counter <= LAST_TEMPERATURE_SENSOR_ID) {

    if (  ( (in_server_sensors_temp[id_counter - FIRST_TEMPERATURE_SENSOR_ID] * (1.0 - TEMPERATURES_NOTIFY_TOLERANCE) ) >= sensors_temp[id_counter - FIRST_TEMPERATURE_SENSOR_ID] ) || ( (in_server_sensors_temp[id_counter - FIRST_TEMPERATURE_SENSOR_ID] * (1.0 + TEMPERATURES_NOTIFY_TOLERANCE) ) <= sensors_temp[id_counter - FIRST_TEMPERATURE_SENSOR_ID] ) ) {
      if ( !(in_server_sensors_temp[id_counter - FIRST_TEMPERATURE_SENSOR_ID] == 0 &&  sensors_temp[id_counter - FIRST_TEMPERATURE_SENSOR_ID] == 0) ) {
        return true;
      }
    } else {
      return false;
    }

  } else if (id_counter == PWM_EC_ID) {
    if (  (in_server_pwm_ec * (1.0 - PWM_EC_NOTIFY_TOLERANCE) ) >= pwm_ec || (in_server_pwm_ec * (1.0 + PWM_EC_NOTIFY_TOLERANCE) ) <= pwm_ec ) {
      if ( !(in_server_pwm_ec == 0 &&  pwm_ec == 0) ) {
        return true;
      }
    } else {
      return false;
    }
  } else if (id_counter == BATTERY_SOC_ID) {
    if (  (in_server_battery_SoC * (1.0 - BATTERY_SOC_NOTIFY_TOLERANCE) ) >= battery_SoC || (in_server_battery_SoC * (1.0 + BATTERY_SOC_NOTIFY_TOLERANCE) ) <= battery_SoC ) {
      if ( !(in_server_battery_SoC == 0 &&  battery_SoC == 0) ) {
        return true;
      }
    } else {
      return false;
    }
  } else if (id_counter == PWM_BATTERY_CHARGER_ID) {
    if (  (in_server_PWM_battery_charger * (1.0 - PWM_BATTERY_CHARGER_NOTIFY_TOLERANCE) ) >= PWM_battery_charger || (in_server_PWM_battery_charger * (1.0 + PWM_BATTERY_CHARGER_NOTIFY_TOLERANCE) ) <= PWM_battery_charger ) {
      if ( !(in_server_PWM_battery_charger == 0 &&  PWM_battery_charger == 0) ) {
        return true;
      }
    } else {
      return false;
    }
  } else if (id_counter == CAR_SPEED_ID) {
    if (  (in_server_car_speed * (1.0 - CAR_SPEED_NOTIFY_TOLERANCE) ) >= car_speed || (in_server_car_speed * (1.0 + CAR_SPEED_NOTIFY_TOLERANCE) ) <= car_speed ) {
      if ( !(in_server_car_speed == 0 &&  car_speed == 0) ) {
        return true;
      }
    } else {
      return false;
    }
  } else if (id_counter == CAR_KILOMETRAGE_ID) {
    if (  (in_server_car_kilometrage * (1.0 - CAR_KILOMETRAGE_NOTIFY_TOLERANCE) ) >= car_kilometrage || (in_server_car_kilometrage * (1.0 + CAR_KILOMETRAGE_NOTIFY_TOLERANCE) ) <= car_kilometrage ) {
      if ( !(in_server_car_kilometrage == 0 &&  car_kilometrage == 0) ) {
        return true;
      }
    } else {
      return false;
    }
  }


  return false;
}

// SIMPLE UPDATE OF THE SERVER STORED VALUE IN THIS ARDUINO. ( Before notifying the server )
void replace_server_with_new(int resource_id) {    //FIRST TIME ALL DATA IS GOING TO BE REPLACED SINCE IT IS BASED ON CHANGES compared to 0 (start value )

  if (resource_id >= FIRST_CELL_VOLTAGE_ID && resource_id <= LAST_CELL_VOLTAGE_ID) {
    in_server_cells_voltage[resource_id - 1] = cells_voltage[resource_id - 1];

  } else if (resource_id >= 25 && resource_id <= 28) {
    in_server_sensors_temp[resource_id - FIRST_TEMPERATURE_SENSOR_ID] = sensors_temp[resource_id - FIRST_TEMPERATURE_SENSOR_ID];

  } else if (resource_id == PWM_EC_ID) {
    in_server_pwm_ec = pwm_ec;
  } else if (resource_id == BATTERY_SOC_ID) {
    in_server_battery_SoC = battery_SoC;
  } else if (resource_id == PWM_BATTERY_CHARGER_ID) {
    in_server_PWM_battery_charger = PWM_battery_charger;
  } else if (resource_id == CAR_SPEED_ID) {
    in_server_car_speed = car_speed;
  } else if (resource_id == CAR_KILOMETRAGE_ID) {
    in_server_car_kilometrage = car_kilometrage;
  }
}
//PREPARES THE RESOURCE FORMAT ( adds key and new line ) AND SENDS IT THROUGH SERIAL PORT TO SERVER.

 //FIRST TIME ALL DATA IS GOING TO BE UPDATED SINCE IT IS BASED ON CHANGES compared to 0 (start value ).    ADDS  A KEY AND A TERMINATING VALUE ( '\n' ) 
void notify_server_of_change(int resource_id) {                    
  short int rcode = 0;
  char put_together[12];
  char auxiliar[4];

  if (resource_id >= FIRST_CELL_VOLTAGE_ID && resource_id <= LAST_CELL_VOLTAGE_ID) {   //sends the format cs=cn.cv cn=cell number cv=cell value * 100(integer)
    

    int aux_float_int = in_server_cells_voltage[resource_id - 1] * 100;
    strcpy(put_together, CELL_VOLT_KEY);
    sprintf(auxiliar, "%d", resource_id);
    strcat(put_together, "=");
    strcat(put_together, auxiliar);
    strcat(put_together, "."); 
    sprintf(auxiliar, "%d", aux_float_int);
    strcat(put_together, auxiliar);

  } else if (resource_id >= FIRST_TEMPERATURE_SENSOR_ID && resource_id <= LAST_TEMPERATURE_SENSOR_ID) {
    if (resource_id == FIRST_TEMPERATURE_SENSOR_ID)
      strcpy(put_together, TEMP_1_KEY);
    if (resource_id == FIRST_TEMPERATURE_SENSOR_ID + 1)
      strcpy(put_together, TEMP_2_KEY);
    if (resource_id == FIRST_TEMPERATURE_SENSOR_ID + 2)
      strcpy(put_together, TEMP_3_KEY);
    if (resource_id == LAST_TEMPERATURE_SENSOR_ID)
      strcpy(put_together, TEMP_4_KEY);

    strcat(put_together, "=");
    sprintf(auxiliar, "%d", in_server_sensors_temp[resource_id - FIRST_TEMPERATURE_SENSOR_ID]);
    strcat(put_together, auxiliar);

  } else if (resource_id == PWM_EC_ID) {
    strcpy(put_together, PWM_EC_KEY);
    strcat(put_together, "=");
    sprintf(auxiliar, "%d", in_server_pwm_ec);
    strcat(put_together, auxiliar);

  } else if (resource_id == BATTERY_SOC_ID) {
    strcpy(put_together, CAR_SoC_KEY);
    strcat(put_together, "=");
    sprintf(auxiliar, "%d", in_server_battery_SoC);
    strcat(put_together, auxiliar);

  } else if (resource_id == PWM_BATTERY_CHARGER_ID) {
    strcpy(put_together, PWM_BC_KEY);
    strcat(put_together, "=");
    sprintf(auxiliar, "%d", in_server_PWM_battery_charger);
    strcat(put_together, auxiliar);

  } else if (resource_id == CAR_SPEED_ID) {
    strcpy(put_together, CAR_SPEED_KEY);
    strcat(put_together, "=");
    sprintf(auxiliar, "%d", in_server_car_speed);
    strcat(put_together, auxiliar);

  } else if (resource_id == CAR_KILOMETRAGE_ID) {
    strcpy(put_together, CAR_KILOMETRAGE_KEY);
    strcat(put_together, "=");
    sprintf(auxiliar, "%d", in_server_car_kilometrage);
    strcat(put_together, auxiliar);
  } else {
    //ERROR IN resource_id doesn't match
    rcode = 1;
  }
  if (!rcode) {
    strcat(put_together, "\n");
    Serial.print(put_together);
    
  }

}
//MAIN FUNCTION CALLED AFTER A RESOURCE IS RECEIVED. 
//FIRST TIME ALL DATA IS GOING TO BE UPDATED , THEN IT WILL BASED ON CHANGES ... AND EVERY FULL_UPDATE_MOMENT/TIMER_MULTIPLIER  all data is going to be updated to enhances reliability
void compare_replace_notify() {

  
  if (full_update_trigger >= FULL_UPDATE_MOMENT) {   //UPDATES ALL RESOURCES DISREGARDING CHANGES OR NOT
    
    for (int resource_ID = 1; resource_ID <= NUM_OF_IDS ; resource_ID++) { 
      replace_server_with_new(resource_ID);
      notify_server_of_change(resource_ID); 
      delay(30);// in betweet notification sending. To provide time to the OPENMOTE to receive, notify client in between readings ... etc. TODO: analyze how to optimize this delay 
    }
    full_update_trigger = 0;
  } else { 
                                                      //UPDATES JUST THE CHANGED ENOUGH VALUES
    for (int resource_ID = 1; resource_ID <= NUM_OF_IDS; resource_ID++) {
      if ( compare_server_vs_new(resource_ID) && full_update_trigger < FULL_UPDATE_MOMENT) {
        replace_server_with_new(resource_ID);
        notify_server_of_change(resource_ID);
        delay(20);   // in betweet notification sending. To provide time to the OPENMOTE to receive, notify client in between readings ... etc. TODO: analyze how to optimize this delay 
      }    
    }
    
  }
  return;
}
////OPTIMIZATION CRITERIAS FOR EC REQUEST GO HERE. returns true if request necessary, 0 otherwise.
int engine_controller_request_necessary() {  
  if (in_server_PWM_battery_charger == CAR_IS_CHARGING ) { //BATTERY CHARGING.
    return 0;
  } else if (in_server_car_speed == 0) { //Car not moving.
    return CAR_SPEED_ID;
  } else if (  (timer_interrupt_counter_EC == PERIOD_KILOMETRAGE_HIGH_SPEED)  &&  in_server_car_speed > 50 ) { //car goes > 50km/h and enough time passed since last request.
    timer_interrupt_counter_EC = 0;
    return CAR_SPEED_ID + CAR_KILOMETRAGE_ID;
  } else if (  (timer_interrupt_counter_EC == PERIOD_KILOMETRAGE_LOW_SPEED)  &&  in_server_car_speed < 50 ) {
    timer_interrupt_counter_EC = 0;
    return CAR_SPEED_ID + CAR_KILOMETRAGE_ID;
  } else if (timer_interrupt_counter_EC == PERIOD_KILOMETRAGE_LOW_SPEED) {
    timer_interrupt_counter_EC = 0;
    return CAR_SPEED_ID + CAR_KILOMETRAGE_ID;
  } else {
    return CAR_SPEED_ID;
  }

}


void automatic_request_execution(){
    //BMS REQUEST
          
          //if after 3 secs of this time the Arduino received no data from BMS... we assume it crashed.
      int rcode = 0;
      rcode = send_start_stop_cmd(SEND_START_COMMAND);
       delay(10);  
       if(rcode){
          Serial.print("Error st CMD ");  //BMS Crashed. 
          return;
       }
       
       datain_counter=0;
      // NON BLOCKING SERIAL READING ( IT ACCEPTS INTERRUPTS, interrupts are active.  )
      Serial.print("READ "); Serial.println(flag_reading_bms);
      unsigned long initial_time=millis();
      while (datain_counter < AMOUNT_LINES_READ_BMS && flag_reading_bms) {  //BLOCKING READ OF A LOOP OF DATA     
        Serial.println(datain_counter);
        int rcode = 0;
        char  buf[20];
        uint16_t rcvd = 20;
        rcode = Acm.RcvData(&rcvd, buf);   //reads received data.
        if ( rcvd ) { //more than zero bytes received
          wait_full_line_and_store(buf, rcvd);   //makes sure 5 bytes line is complete (necessary with arduino's buffer and serial prints probable problems ) and stores the parsed data in memory.
        }
       
        if ( (millis()-initial_time>BMS_CRASHED_AFTER)  &&  datain_counter < (AMOUNT_LINES_READ_BMS ) ) { // if after X seg and we didn't receive all loop     
          flag_bms_crashed=true;
          break;      
        }
      }
      Serial.print("ms of loop:");Serial.println( millis()-initial_time);
      if (flag_bms_crashed){    //If CRASHED return to main loop.
       flag_bms_crashed=true;
       Serial.println( millis()-initial_time);
       Serial.println( datain_counter );
        return;
      }
     // END OF NON BLOCKING SERIAL READING  ( AMOUNT_LINES_READ_BMS read from BMS )
  
      delay(30);
      rcode = send_start_stop_cmd(SEND_STOP_COMMAND);

       if(rcode){
          Serial.print("Error with stop CMD ");
          datain_counter = 0;
          return;
       }
      compare_replace_notify();
      datain_counter = 0;
      
  
  //ENGINE CONTROLLER  REQUEST  ( Optimized )
//
//  int ec_which_resource = engine_controller_request_necessary(); //0 if none. CAR_SPEED_ID if just that is needed. CAR_KILOMETRAGE_ID if just that is needed. CAR_SPEED_ID+CAR_KILOMETRAGE_ID if both needed.
//  if (  ec_which_resource == ( CAR_SPEED_ID + CAR_KILOMETRAGE_ID )  ) {
//    //REQUEST BOTH
//  } else if (  ec_which_resource == CAR_KILOMETRAGE_ID ) {
//    //REQUEST car_kilometrage
//  } else if ( ec_which_resource == CAR_SPEED_ID ) {
//    //REQUEST car_speed
//  } else {
//    //REQUEST none.
//  }
  flag_need_request=false;
}

// CALLBACK FUNCTION FROM TIMER INTERRUPT
void handle_request_timer_interrupt() {   //keep low amount of tasks in interrupts functions ( embebbed system )
  noInterrupts(); // done by default ( just to remember )
  
  if ( Acm.isReady() && flag_timer_on ) {  //IF COMMUNICATION ESTABLISHED.
            
          //TIMER DISABLES THE INTERRUPTS WHEN ENTERING CALLBACK FUNCTION. 
          if( flag_first_read){
                 Serial.println("firt read");
                 flag_first_read=true;
          }else{
            
                full_update_trigger++;    //if FULL_UPDATE_MOMENT = full_update_trigger   ... it updates all data, disregarding changes or not.
                if (bms_timer_multiplier_counter < BMS_TIMER_MULTIPLIER) { // TO MAKE LONGER THAN 5 sec TIMERS
                   bms_timer_multiplier_counter++;        
                } else {
                  bms_timer_multiplier_counter = 1;
                  timer_interrupt_counter_EC++;
                  flag_need_request=true;
                }
                  
          } 
   }     
    interrupts(); // done by default ( just to remember )
}


void setup()
{

  Serial.begin( 115200 );
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect.
#endif
    PRINTFLN("Start");
  if (Usb.Init() == -1)
    PRINTFLN("OSCOKIRQ failed to assert");

  //////////INITIALIZE static vectors///////
  for (int i = 0; i < 24; i++) {
    cells_voltage[i] = 0;
    in_server_cells_voltage[i] = 0;
    if (i < 5) {
      sensors_temp[i] = 0;
      in_server_sensors_temp[i] = 0;
    }
  }



  delay( 200 );

  for (int i = 0; i < BMS_BYTES_PER_LINE - 1; i++) {
    buf_aux[i] = 0;
  }

  Timer1.initialize(BMS_TIMER_INTERRUPT); //   USE THE MULTIPLIER TO GET FACTORS of 5 secs (BMS_TIMER_INTERRUPT)
  Timer1.attachInterrupt( handle_request_timer_interrupt , BMS_TIMER_INTERRUPT );
}



void loop()   // 
{
  
  Usb.Task();   //needs to be in the main loop. //Function that established and mantain the USB conection.


  if (flag_bms_crashed){
      Timer1.stop();
      flag_timer_on = false;
      flag_need_request=false;
      datain_counter = 0;
      Serial.println("\nBMS HAS CRASHED!!");         //TODO implement a cmd/resource to notify server and client. 
  }
  
  if( Usb.getUsbTaskState() != USB_STATE_RUNNING ){       //TRYIES TO CONNECT       
    flag_first_connection=true;
    

    //Waiting for BMS or connecting to it.
     delay(50); 
               Timer1.stop();
               flag_timer_on = false;
              
  }else if ( Usb.getUsbTaskState() == USB_STATE_RUNNING && flag_first_connection) {   //First time gives 500ms time to start the comm. 
     Serial.print("RUNNING\n");
     
      flag_reading_bms=false;
      flag_first_connection=false;
      flag_bms_crashed=false;
      delay(1000); //AFTER COMMUNICATION CHANNEL IS ESTABLISHED IT WAITS THIS TIME TO enhance the chances that the server is operational and receives the first full stream of data. (TODO: analyze this, maybe not needed, or maybe other values )   
       //TO FORCE THE FIRST REQUEST as soon as possible ( TODO : CHECK TIMER FIRST ATTACH, weird behaviour. 
     // automatic_request_execution();
  }else if(!flag_bms_crashed){
        if ( Acm.isReady())  {  //IF CONNECTION ESTABLISHED ( double check ). 

                if (  !flag_timer_on ){  // Sets the timer for requests.  
                              flag_timer_on = true;   
                              flag_first_read=true;  
                              Timer1.restart();
                              Timer1.start();
                 }
               
                 if (flag_need_request || flag_first_read){  //Set by timer interrupt     
                        automatic_request_execution();
                        flag_first_read=false;
                 }
                 
          delay(20);  //delay maybe not necessary here. Depends on Arduiino HOst library working. TODO: check
          }
      
  }
  
  
  
}  //TODO ANALYZE WHY IT IS NOT ALWAYS WORKING ON NON-SELF-SUPPLY DEVICES .. 





