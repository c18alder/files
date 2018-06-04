//teSTING I2C 1
//#include <Wire.h>
//extern TwoWire Wire1;

#include <LiquidCrystal_I2C.h>     // DiSPLAY
#include "Adafruit_LEDBackpack.h" // LED BAR
#include <Servo.h>                // ENGINE

#define MAX_RESOURCE_FORMAT_SIZE 15
#define ERROR_CODE 1
#define NUMBER_OF_RESOURCES 37

//BAR LED
#define AMOUNT_OF_LEDS 24
//DISPLAY BUTTON
#define BUTTON_PIN 2


//MENU SELECTION
#define MAIN_MENU 1
#define MENU_CELLS_1_8 2
#define MENU_CELLS_9_16 3
#define MENU_CELLS_17_24 4
#define MENU_PWM_CURRENT 5
#define AMOUNT_OF_MENUS 5

//TO HANDLE MENUS AND LINES
#define CHARS_PER_LINE 20
#define NUMBER_OF_LINES 4
#define KMS_VALUE_CURSOR 9
#define TMP_FB_VALUE_CURSOR 4
#define TMP_RB_VALUE_CURSOR 14
#define TMP_MB_VALUE_CURSOR 4
#define TMP_BMS_VALUE_CURSOR 15

#define CELL_1D_1_VALUE_CURSOR 3
#define CELL_1D_2_VALUE_CURSOR 13

#define CELL_2D_1_VALUE_CURSOR 4
#define CELL_2D_2_VALUE_CURSOR 15

#define PWM_VALUE_CURSOR 15
#define CURRENT_VALUE_CURSOR 9

#define Temp_Start_2 11
#define Cell_start2 11

//DISPLAY FORMATS
#define D_KILOMETRAGE "Km = "
#define D_TFB "Tfb="
#define D_TRB "Trb="
#define D_TMB "Tmb="
#define D_TBMS "Tbms="
#define D_TBMS "Tbms="
#define D_PWM_EC "Pwm to EC = "
#define D_PWM_BC "Pwm to BC = "
#define D_CURRENT "I = "




//KEY VALUE TO VERIFY RESOURCE (no corrupted data):
#define CAR_SPEED_KEY "km/h"
#define CAR_SoC_KEY "%bat"
#define CAR_KILOMETRAGE_KEY "kms"
#define PWM_EC_KEY "pec"   //PWM Engine Controler
#define PWM_BC_KEY "pbc"  //PWM Battery Charger 
#define TEMP_FB_KEY "ta"
#define TEMP_RB_KEY "tb"
#define TEMP_MB_KEY "tc"
#define TEMP_BMS_KEY "td"
#define CELL_VOLT_KEY "cs" // a "." separates the cell value and number---> ej:  cs=18.649    (cell value is not devided by 200 yet (done in server) TODO : maybe do it here.. 

#define DEGREES_SYMBOL (char)223
//

#define NUM_CELLS 24
#define NUM_TEMPERATURES 4
//RESEOURCE ID
#define FIRST_CELL_VOLTAGE_ID 1
#define LAST_CELL_VOLTAGE_ID 24
#define TEMP_FB_ID 25
#define TEMP_RB_ID 26
#define TEMP_MB_ID 27
#define TEMP_BMS_ID 28

#define LAST_TEMPERATURE_SENSOR_ID 28
#define PWM_EC_ID 29
#define BATTERY_SOC_ID 30
#define PWM_BATTERY_CHARGER_ID 31
#define CAR_SPEED_ID 32
#define CAR_KILOMETRAGE_ID 33
#define BMS_CURRENT_ID 34

LiquidCrystal_I2C lcd(0x27, 20, 4);
Adafruit_24bargraph bar = Adafruit_24bargraph();
Servo myservo;

static bool first_turn_on = true;
//cuRRENT MENU OF THE DISPLAY
volatile static short int current_menu = MAIN_MENU;


//JUST THE VALUES AS CHARS.  ( TO USE ATOI if necessary
static char resources_values[NUMBER_OF_RESOURCES][MAX_RESOURCE_FORMAT_SIZE];
volatile static bool interrupt = false;
volatile static int ledState = HIGH;
volatile static int lastButtonState = LOW;

//TO TEST WITHOUT READING
//static int line_sent_counter = 0;
//static int battery_counter = 0;
//static int speed_counter = 0;
//static int kilometrage_counter = 0;




int get_resource_id(char *data) { //returns the resource ID or 0 if no matches ( corrupted data )
  char *pch;
  char aux[MAX_RESOURCE_FORMAT_SIZE];
  strcpy(aux, data);
  pch = strtok (aux, "=.");
  if ( !(strcmp(CAR_SPEED_KEY, pch)) ) {
    return CAR_SPEED_ID;
  } else if ( !(strcmp(CAR_KILOMETRAGE_KEY, pch)) ) {
    return CAR_KILOMETRAGE_ID;
  } else if ( !(strcmp(PWM_BC_KEY, pch)) ) {
    return PWM_BATTERY_CHARGER_ID;
  } else if ( !(strcmp(CAR_SoC_KEY, pch)) ) {
    return BATTERY_SOC_ID;
  } else if ( !(strcmp(PWM_EC_KEY, pch)) ) {
    return PWM_EC_ID; bar.writeDisplay();
  } else if ( !(strcmp(TEMP_FB_KEY, pch)) ) {
    return TEMP_FB_ID;
  } else if ( !(strcmp(CAR_KILOMETRAGE_KEY, pch)) ) {
    return TEMP_RB_ID;
  } else if ( !(strcmp(CAR_KILOMETRAGE_KEY, pch)) ) {
    return TEMP_MB_ID;
  } else if ( !(strcmp(CAR_KILOMETRAGE_KEY, pch)) ) {
    return TEMP_BMS_ID;
  } else if ( !strcmp(CELL_VOLT_KEY, pch) ) {
    pch = strtok (NULL, "=.");
    for ( int i = FIRST_CELL_VOLTAGE_ID ; i <= LAST_CELL_VOLTAGE_ID ; i++) {
      if ( atoi(pch) == i ) {
        return i;
      }
    }
  } else {
    return 0;
  }
}



void change_just_value(int resource_id, int menu_code) { //TO UPDATE A PARTICULAR VALUE AND NOT BLINK ALL THE DISPLAY ( Kilometrage x Example )
  if (menu_code == MAIN_MENU) {
    if (resource_id == CAR_KILOMETRAGE_ID) {

      lcd.setCursor( KMS_VALUE_CURSOR , 1); //after '='
      lcd.print(resources_values[resource_id]);
      for (int i = 0; i < CHARS_PER_LINE - KMS_VALUE_CURSOR - strlen(resources_values[resource_id]) ; i++) {
        lcd.print(" ");
      }


    } else if (resource_id == TEMP_FB_ID) {

      lcd.setCursor( TMP_FB_VALUE_CURSOR , 2);
      lcd.print(resources_values[resource_id]);
      lcd.print(DEGREES_SYMBOL);  //PRINTS '°'
      lcd.print("C");

      for (int i = TMP_FB_VALUE_CURSOR + strlen(resources_values[resource_id]) + 2; i < Temp_Start_2 - 1 ; i++) {

        lcd.print(" ");
      }


    } else if (resource_id == TEMP_RB_ID) {
      lcd.setCursor( TMP_RB_VALUE_CURSOR , 2);
      lcd.print(resources_values[resource_id]);
      lcd.print(DEGREES_SYMBOL);  //PRINTS '°'
      lcd.print("C");
      for (int i = TMP_RB_VALUE_CURSOR + strlen(resources_values[resource_id]) + 1; i < CHARS_PER_LINE - 1 ; i++) {
        lcd.print(" ");
      }
    } else if (resource_id == TEMP_MB_ID) {
      lcd.setCursor( TMP_MB_VALUE_CURSOR , 3);
      lcd.print(resources_values[resource_id]);
      lcd.print(DEGREES_SYMBOL);  //PRINTS '°'
      lcd.print("C");

      for (int i = TMP_MB_VALUE_CURSOR + strlen(resources_values[resource_id]) + 2; i < Temp_Start_2 - 1; i++) {
        lcd.print(" ");
      }
    } else if (resource_id == TEMP_BMS_ID) {
      lcd.setCursor( TMP_BMS_VALUE_CURSOR , 3);
      lcd.print(resources_values[resource_id]);
      lcd.print(DEGREES_SYMBOL);  //PRINTS '°'
      lcd.print("C");
      for (int i = TMP_BMS_VALUE_CURSOR + strlen(resources_values[resource_id]) + 1; i < CHARS_PER_LINE - 1; i++) {
        lcd.print(" ");
      }



    }

  } else if (menu_code == MENU_CELLS_1_8) {

    switch (resource_id) {
      case 1:
      case 3:
      case 5:
      case 7:
        lcd.setCursor( CELL_1D_1_VALUE_CURSOR , (resource_id - 1) / 2); //  (resource_id-1)/2  trick to set the line.
        lcd.print(resources_values[resource_id]);
        lcd.print("V");
        for (int i = CELL_1D_1_VALUE_CURSOR + strlen(resources_values[resource_id]) + 1; i < Cell_start2 - 1; i++) {
          lcd.print(" ");
        }
        break;
      case 2:
      case 4:
      case 6:
      case 8:
        lcd.setCursor( CELL_1D_2_VALUE_CURSOR , (resource_id - 1) / 2);
        lcd.print(resources_values[resource_id]);
        lcd.print("V");
        for (int i = CELL_1D_2_VALUE_CURSOR + strlen(resources_values[resource_id]) + 1; i < CHARS_PER_LINE - 1; i++) {
          lcd.print(" ");
        }
        break;
      default:
        ;

    }

  } else if (menu_code == MENU_CELLS_9_16) {
    switch (resource_id) {
      case 9:
        lcd.setCursor( CELL_1D_1_VALUE_CURSOR , (resource_id - 9) / 2); //  (resource_id-9)/2  trick to set the line.
        lcd.print(resources_values[resource_id]);
        lcd.print("V");
        for (int i = CELL_1D_1_VALUE_CURSOR + strlen(resources_values[resource_id]) + 1; i < Cell_start2 - 1; i++) {
          lcd.print(" ");
        }
        break;

      case 11:
      case 13:
      case 15:
        lcd.setCursor( CELL_2D_1_VALUE_CURSOR , (resource_id - 9) / 2); //  (resource_id-9)/2  trick to set the line.
        lcd.print(resources_values[resource_id]);
        lcd.print("V");
        for (int i = CELL_2D_1_VALUE_CURSOR + strlen(resources_values[resource_id]) + 1; i < Cell_start2 - 1; i++) {
          lcd.print(" ");

        }
        break;
      case 10:
      case 12:
      case 14:
      case 16:
        lcd.setCursor( CELL_2D_2_VALUE_CURSOR , (resource_id - 9) / 2);
        lcd.print(resources_values[resource_id]);
        lcd.print("V");
        for (int i = CELL_2D_2_VALUE_CURSOR + strlen(resources_values[resource_id]) + 1; i < CHARS_PER_LINE - 1; i++) {
          lcd.print(" ");
        }
        break;
      default:
        ;

    }

  } else if (menu_code == MENU_CELLS_17_24) {

    switch (resource_id) {
      case 17:
      case 19:
      case 21:
      case 23:
        lcd.setCursor( CELL_2D_1_VALUE_CURSOR , (resource_id - 17) / 2); //  (resource_id-1)/2  trick to set the line.
        lcd.print(resources_values[resource_id]);
        lcd.print("V");
        for (int i = CELL_2D_1_VALUE_CURSOR + strlen(resources_values[resource_id]) + 1; i < Cell_start2 - 1; i++) {
          lcd.print(" ");
        }
        break;
      case 18:
      case 20:
      case 22:
      case 24:
        lcd.setCursor( CELL_2D_2_VALUE_CURSOR , (resource_id - 17) / 2);
        lcd.print(resources_values[resource_id]);
        lcd.print("V");
        for (int i = CELL_2D_2_VALUE_CURSOR + strlen(resources_values[resource_id]) + 1; i < CHARS_PER_LINE - 1; i++) {
          lcd.print(" ");
        }
        break;
      default:
        ;

    }


  } else if (menu_code == MENU_PWM_CURRENT) {
    if (resource_id == PWM_EC_ID) {
      lcd.setCursor( 3, 0);
      lcd.print(D_PWM_EC);
      lcd.print(resources_values[resource_id]);
      lcd.print("%");
      for (int i = PWM_VALUE_CURSOR + strlen(resources_values[resource_id]) + 1; i < CHARS_PER_LINE; i++) {
        lcd.print(" ");
      }
    } else if (resource_id == PWM_BATTERY_CHARGER_ID) {
      lcd.setCursor( 3, 1);
      lcd.print(D_PWM_BC);
      lcd.print(resources_values[resource_id]);
      lcd.print("%");
      for (int i = PWM_VALUE_CURSOR + strlen(resources_values[resource_id]) + 1; i < CHARS_PER_LINE; i++) {
        lcd.print(" ");
      }
    } else if (resource_id == BMS_CURRENT_ID) {
      lcd.setCursor( 5, 2);
      lcd.print(D_CURRENT);
      lcd.print(resources_values[resource_id]);
      lcd.print(" A");

      for (int i = CURRENT_VALUE_CURSOR + strlen(resources_values[resource_id]) + 2; i < CHARS_PER_LINE; i++) {
        lcd.print(" ");
      }
    }

  }

}
//// PRINTS THE MENU. IF THE MENU TO BE PRINTED IS THE SAME BEING DISPLAYED, IT JUST CHANGES THE CORRESPONDING RESOURCE VALUE.
int lcd_print( int menu_code, int resource_id) {
  bool print_all_the_menu = (resource_id == 0);

  if (menu_code == current_menu && !print_all_the_menu) { //just change the corresponding line.
    change_just_value(resource_id, menu_code);




  } else if (menu_code == MAIN_MENU ) {
    lcd.clear();
    lcd.print("OPEN SOURCE VEHICLE");
    lcd.setCursor( 4, 1);
    lcd.print(D_KILOMETRAGE);
    lcd.print(resources_values[CAR_KILOMETRAGE_ID]);
    lcd.setCursor( 0, 2);
    lcd.print(D_TFB);
    lcd.print(resources_values[TEMP_FB_ID]);
    lcd.print(DEGREES_SYMBOL);  //PRINTS '°'
    lcd.print("C");
    lcd.setCursor( 10, 2);
    lcd.print(D_TRB);
    lcd.print(resources_values[TEMP_RB_ID]);
    lcd.print(DEGREES_SYMBOL);  //PRINTS '°'
    lcd.print("C");
    lcd.setCursor( 0, 3);
    lcd.print(D_TMB);
    lcd.print(resources_values[TEMP_MB_ID]);
    lcd.print(DEGREES_SYMBOL);  //PRINTS '°'
    lcd.print("C");
    lcd.setCursor( 10, 3);
    lcd.print(D_TBMS);
    lcd.print(resources_values[TEMP_BMS_ID]);
    lcd.print(DEGREES_SYMBOL);  //PRINTS '°'
    lcd.print("C");
    current_menu = MAIN_MENU;
    return 0;

  } else if (menu_code == MENU_CELLS_1_8) {
    char aux_str[10];
    char aux_num[3];
    int counter = 1;
    lcd.clear();
    for (int i = 0 ; i < 4 ; i++) {
      for (int j = 0 ; j < 13 ; j = j + 10) {
        lcd.setCursor( j, i);
        strcpy(aux_str, "C");
        sprintf(aux_num, "%d", counter);
        strcat(aux_str, aux_num);
        strcat(aux_str, "=");
        lcd.print(aux_str);
        lcd.print(resources_values[counter]);
        lcd.print("V");
        counter++;
      }
    }
    current_menu = MENU_CELLS_1_8;

  } else if (menu_code == MENU_CELLS_9_16) {
    char aux_str[10];
    char aux_num[3];
    int counter = 9;

    lcd.clear();
    for (int i = 0 ; i < 4 ; i++) {
      for (int j = 0 ; j < 13 ; j = j + 11) {

        lcd.setCursor( j, i);
        strcpy(aux_str, "C");
        sprintf(aux_num, "%d", counter);
        strcat(aux_str, aux_num);
        strcat(aux_str, "=");
        lcd.print(aux_str);
        lcd.print(resources_values[counter]);
        lcd.print("V");
        counter++;
      }
    }
    current_menu = MENU_CELLS_9_16;
  } else if (menu_code == MENU_CELLS_17_24) {
    char aux_str[10];
    char aux_num[3];
    int counter = 17;
    lcd.clear();
    for (int i = 0 ; i < 4 ; i++) {
      for (int j = 0 ; j < 13 ; j = j + 11) {
        lcd.setCursor( j, i);
        strcpy(aux_str, "C");
        sprintf(aux_num, "%d", counter);
        strcat(aux_str, aux_num);
        strcat(aux_str, "=");
        lcd.print(aux_str);
        lcd.print(resources_values[counter]);
        lcd.print("V");
        counter++;
      }
    }
    current_menu = MENU_CELLS_17_24;
  } else if (menu_code == MENU_PWM_CURRENT) {
    lcd.clear();
    lcd.setCursor( 3, 0);
    lcd.print(D_PWM_EC);
    lcd.print(resources_values[PWM_EC_ID]);
    lcd.print("%");
    lcd.setCursor( 3, 1);
    lcd.print(D_PWM_BC);
    lcd.print(resources_values[PWM_BATTERY_CHARGER_ID]);
    lcd.print("%");
    lcd.setCursor( 5, 2);
    lcd.print(D_CURRENT);
    lcd.print(resources_values[BMS_CURRENT_ID]);
    lcd.print(" A");
    lcd.setCursor(0, 3);
    lcd.print("   TO MAIN MENU   ");
    current_menu = MENU_PWM_CURRENT;
  } else {
    return ERROR_CODE;
  }
  return 0;
}

void menu_toggle() {
  lcd.clear();
  if (current_menu == AMOUNT_OF_MENUS) {
    current_menu = MAIN_MENU;
  } else {
    current_menu++;
  }
  lcd_print(current_menu, 0);
  return;
}

void update_ledbar(char *new_soc_value) { // led bar has 2 led colors, but mixing creates the yellow.
  int colour = LED_OFF;

  for (int i = 0; i < AMOUNT_OF_LEDS; i++) { // the rest, turned off.
    bar.setBar(i, LED_OFF);
  }

  // SETS THE PERCENTACE OF EACH COLOUR ( MADE FOR 4 COLOURS )
  if (atoi(new_soc_value) <= 15) { //10 %   red
    colour = LED_RED;


  } else if ( atoi(new_soc_value) > 10  && atoi(new_soc_value) <= 30 ) { //orange
    colour = LED_YELLOW; // turn it orange after

  } else if ( atoi(new_soc_value) > 30   && atoi(new_soc_value) <= 60 ) {
    colour = LED_YELLOW;

  } else {
    colour = LED_GREEN;
  }
  for (int i = 0; i < (atoi(new_soc_value)*AMOUNT_OF_LEDS / 100 ); i++) { //the ones to turn on. 24 (AMOUNT_OF_LEDS) on --> 100%     ,     12 ON -->50%  ... etc
    bar.setBar(i, colour); // turn it orange here
  }

  bar.writeDisplay();
}

void update_speed_display(char *new_speed_value) {
  if (atoi(new_speed_value) > 170)
    return;
  double val = map(atoi(new_speed_value), 0, 200, 0, 180);
  if (val > 170)
    val = 170;
  myservo.write(val);

}


int update_incoming_resource(char *data) {
  int aux_atoi;
  char *pch;
  char auxy[10];
  char auxy2[10];


  pch = strtok (data, "=.\n");
  if (pch == NULL) {
    return 0; //ERROR
  }

  if (!strcmp(CELL_VOLT_KEY, pch)) {

    pch = strtok (NULL, "=.\n");
    if (pch == NULL) {
      return 0; //ERROR
    }

    aux_atoi = atoi(pch);
    pch = strtok (NULL, "=.\n");

    sprintf(auxy, "%d", atoi(pch) / 100 );
    strcat(auxy, ".");
    sprintf(auxy2, "%d", atoi(pch) - ((atoi(pch) / 100) * 100) );
    strcat(auxy, auxy2 ); //PRINTING Floats in arduino is annoying, I am doing this.

    strcpy(resources_values[aux_atoi], auxy);



    if (aux_atoi <= 8 && current_menu == MENU_CELLS_1_8) {

      lcd_print(MENU_CELLS_1_8, aux_atoi);

    } else if (aux_atoi <= 16 && current_menu == MENU_CELLS_9_16) {
      lcd_print(MENU_CELLS_9_16, aux_atoi);
    } else if (aux_atoi <= 24 && current_menu == MENU_CELLS_17_24) {
      lcd_print(MENU_CELLS_17_24, aux_atoi);
    }



  } else if (!strcmp(TEMP_FB_KEY, pch)) {
    pch = strtok (NULL, "=.\n");
    strcpy(resources_values[TEMP_FB_ID], pch);
    if (current_menu == MAIN_MENU) {
      lcd_print(MAIN_MENU, TEMP_FB_ID);
    }
  } else if (!strcmp(TEMP_RB_KEY, pch)) {
    pch = strtok (NULL, "=.\n");
    strcpy(resources_values[TEMP_RB_ID], pch);
    if (current_menu == MAIN_MENU) {
      lcd_print(MAIN_MENU, TEMP_RB_ID);
    }
  } else if (!strcmp(TEMP_MB_KEY, pch)) {
    pch = strtok (NULL, "=.\n");
    strcpy(resources_values[TEMP_MB_ID], pch);
    if (current_menu == MAIN_MENU) {
      lcd_print(MAIN_MENU, TEMP_MB_ID);
    }
  } else if (!strcmp(TEMP_BMS_KEY, pch)) {
    pch = strtok (NULL, "=.\n");
    strcpy(resources_values[TEMP_BMS_ID], pch);
    if (current_menu == MAIN_MENU) {
      lcd_print(MAIN_MENU, TEMP_BMS_ID);
    }
  } else if (!strcmp(PWM_BC_KEY, pch)) {
    pch = strtok (NULL, "=.\n");
    strcpy(resources_values[PWM_BATTERY_CHARGER_ID], pch);
    if (current_menu == MENU_PWM_CURRENT) {
      lcd_print(MENU_PWM_CURRENT, PWM_BATTERY_CHARGER_ID);
    }
  } else if (!strcmp(PWM_EC_KEY, pch)) {
    pch = strtok (NULL, "=.\n");
    strcpy(resources_values[PWM_EC_ID], pch);
    if (current_menu == MENU_PWM_CURRENT) {
      lcd_print(MENU_PWM_CURRENT, PWM_EC_ID);
    }
  } else if (!strcmp(CAR_KILOMETRAGE_KEY, pch)) {
    pch = strtok (NULL, "=.\n");
    strcpy(resources_values[CAR_KILOMETRAGE_ID], pch);
    if (current_menu == MAIN_MENU) {
      lcd_print(MAIN_MENU, CAR_KILOMETRAGE_ID);
    }
  } else if (!strcmp(CAR_SoC_KEY, pch)) {
    pch = strtok (NULL, "=.\n");
    strcpy(resources_values[BATTERY_SOC_ID], pch);
    update_ledbar(resources_values[BATTERY_SOC_ID]);  //update ledbar
  } else if (!strcmp(CAR_SPEED_KEY, pch)) {
    pch = strtok (NULL, "=.\n");
    strcpy(resources_values[CAR_SPEED_ID], pch);
    update_speed_display(resources_values[CAR_SPEED_ID]);
    // ENGINE

  } else {
    return ERROR_CODE;
  }
  return 0;
}





void button_interrupt() {

Serial.println(" Botton ");
interrupts();
delay(50); // to avoid debounce 
  menu_toggle();
}

void setup() {



  //PWM
  myservo.attach(9);
  myservo.write(0);

  //LED BAR
  bar.begin(0x70);  // pass in the address
  for (int i = 0; i < AMOUNT_OF_LEDS; i++) { // the rest, turned off.
    bar.setBar(i, LED_OFF);
  }
  bar.writeDisplay();

  //LCD
  pinMode(BUTTON_PIN, INPUT);
  lcd.init();                      // initialize the lcd
  // lcd.init();
  lcd.backlight();
  Serial.begin(9600);


  //BUTTON
  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_interrupt, FALLING);




}


void loop() {


  char buffer_read[64];
  char recv[64];  //bytes received
  int initial_time;
  int s = 0;


  if (first_turn_on) {                   // TODO : put it in set Up.  (is useful here in case something detaches, etc.. maybe )
    Serial.println("just one time");

    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
      strcpy(resources_values[i], "___");
    }
    lcd_print(2, 0);
    first_turn_on = false;
  }



  if ( Serial.available() > 0) {
    do {
      if ( ( buffer_read[s] = Serial.read() ) != -1) {
        s++;
      }

    } while (s < 15 && buffer_read[s - 1] != '\n');
  }
  if (buffer_read[s - 1] == '\n') {
    buffer_read[s] = '\0';
    Serial.print(buffer_read);
    update_incoming_resource(buffer_read);


  }

  //DEBUG WITHOUT READING
  //  line_sent_counter++;
  //  char to_send[15];
  //  char help[10];
  //
  //  if (line_sent_counter >= 1 && line_sent_counter <= 24) { // CELL VOLT
  //
  //    strcpy(to_send, "cs=");
  //    sprintf(help, "%d", line_sent_counter);
  //    strcat(to_send, help);
  //
  //    strcat(to_send, ".");
  //    sprintf(help, "%d", random(0, 400));
  //    strcat(to_send, help);
  //    strcat(to_send, "\n");
  //    //  Serial.write(to_send , strlen(to_send) ) ;
  //
  //
  //  } else if (line_sent_counter == 25 ) { //TEMPS
  //    strcpy(to_send, "ta=");
  //    sprintf(help, "%d", random(0, 40));
  //    strcat(to_send, help);
  //    strcat(to_send, "\n");
  //    // Serial.write(to_send , strlen(to_send) ) ;
  //
  //  } else if (line_sent_counter == 26) {
  //    strcpy(to_send, "tb=");
  //    sprintf(help, "%d", random(0, 40));
  //    strcat(to_send, help);
  //    strcat(to_send, "\n");
  //    // Serial.write(to_send , strlen(to_send) ) ;
  //
  //  } else if (line_sent_counter == 27) { //SOC
  //    strcpy(to_send, "tc=");
  //    sprintf(help, "%d", random(0, 40));
  //    strcat(to_send, help);
  //    strcat(to_send, "\n");
  //    //  Serial.write(to_send , strlen(to_send) ) ;
  //
  //  } else if (line_sent_counter == 28) {
  //    strcpy(to_send, "td=");
  //    sprintf(help, "%d", random(0, 40));
  //    strcat(to_send, help);
  //    strcat(to_send, "\n");
  //    //  Serial.write(to_send , strlen(to_send) ) ;
  //
  //  } else if (line_sent_counter == 29) {
  //    strcpy(to_send, "pec=");
  //    sprintf(help, "%d", random(0, 100));
  //    strcat(to_send, help);
  //    strcat(to_send, "\n");
  //    //  Serial.write(to_send , strlen(to_send) ) ;
  //
  //  } else if (line_sent_counter == 30) {
  //    strcpy(to_send, "%bat=");
  //    battery_counter = battery_counter - 10;
  //    sprintf(help, "%d", battery_counter);
  //    strcat(to_send, help);
  //    strcat(to_send, "\n");
  //    //  Serial.write(to_send , strlen(to_send) ) ;
  //
  //  } else if (line_sent_counter == 31) {
  //    strcpy(to_send, "pbc=");
  //    sprintf(help, "%d", random(0, 100));
  //    strcat(to_send, help);
  //    strcat(to_send, "\n");
  //    //  Serial.write(to_send , strlen(to_send) ) ;
  //
  //  } else if (line_sent_counter == 32) {
  //    strcpy(to_send, "km/h=");
  //    sprintf(help, "%d", speed_counter);
  //    strcat(to_send, help);
  //    strcat(to_send, "\n");
  //    //  Serial.write(to_send , strlen(to_send) ) ;
  //
  //    speed_counter = speed_counter + 10;
  //  } else if (line_sent_counter == 33) {
  //    strcpy(to_send, "kms=");
  //    kilometrage_counter = kilometrage_counter + random(0, 20);
  //    sprintf(help, "%d", kilometrage_counter);
  //    strcat(to_send, help);
  //    strcat(to_send, "\n");
  //    //  Serial.write(to_send , strlen(to_send) ) ;
  //
  //  } else {
  //    line_sent_counter = 0;
  //  }
  //
  //
  //
  //    if(line_sent_counter!=0){
  //         update_incoming_resource(to_send);
  //    }
  //
  //
  //  delay(500);
  //
  //
  //
  //  //  if (interrupt) {
  //  //    menu_toggle();
  //  //    interrupt = false;
  //  //
  //  //  }



}
