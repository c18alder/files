
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>



#define MAX_RESOURCE_FORMAT_SIZE 15
#define ERROR_CODE 1
#define NUMBER_OF_RESOURCES 37


//MENU SELECTION
#define MAIN_MENU 1
#define MENU_CELLS_1_8 2
#define MENU_CELLS_9_16 3
#define MENU_CELLS_17_24 4
#define MENU_PWM_CURRENT 5


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
#define CELL_2D_2_VALUE_CURSOR 14

#define Temp_Start_2 11
#define Cell_start2 11

//DISPLAY FORMATS 
#define D_KILOMETRAGE "Km = "
#define D_TFB "Tfb="
#define D_TRB "Trb="
#define D_TMB "Tmb="
#define D_TBMS "Tbms="
#define D_TBMS "Tbms="





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

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display



//TO TEST IT HERE
uint8_t aux[64];
static unsigned long line_sent_counter=0;
static int con_status=0;

//RESOURCES SAVED IN THE PRINTING FORMAT ( Except cells )
static short int current_menu=MAIN_MENU;

static short int cells_voltage[NUM_CELLS];
static char cells[NUM_CELLS][MAX_RESOURCE_FORMAT_SIZE];
static char sensors_temp_FB[MAX_RESOURCE_FORMAT_SIZE];
static char sensors_temp_RB[MAX_RESOURCE_FORMAT_SIZE];
static char sensors_TEMP_MB_ID[MAX_RESOURCE_FORMAT_SIZE];
static char sensors_temp_BMS[MAX_RESOURCE_FORMAT_SIZE];

static char pwm_ec[MAX_RESOURCE_FORMAT_SIZE];
static char battery_SoC[MAX_RESOURCE_FORMAT_SIZE];
static char PWM_battery_charger[MAX_RESOURCE_FORMAT_SIZE];
static char battery_current[MAX_RESOURCE_FORMAT_SIZE];
static char car_speed[MAX_RESOURCE_FORMAT_SIZE];
static char car_kilometrage[MAX_RESOURCE_FORMAT_SIZE];


static char car_kilometrage_value[7];
static char sensors_temp_FB_value[7];
static char sensors_temp_RB_value[7];
static char sensors_TEMP_MB_ID_value[7];
static char sensors_temp_BMS_value[7];

static char resources_values[NUMBER_OF_RESOURCES][MAX_RESOURCE_FORMAT_SIZE];



int get_resource_id(char *data){  //returns the resource ID or 0 if no matches ( corrupted data )
        char *pch;      
        char aux[MAX_RESOURCE_FORMAT_SIZE];
        strcpy(aux,data);
        pch= strtok (aux, "=."); 
    if( !(strcmp(CAR_SPEED_KEY,pch)) ){
       return CAR_SPEED_ID;
    }else if( !(strcmp(CAR_KILOMETRAGE_KEY,pch)) ){
      return CAR_KILOMETRAGE_ID; 
    }else if( !(strcmp(PWM_BC_KEY,pch)) ){
      return PWM_BATTERY_CHARGER_ID;
    }else if( !(strcmp(CAR_SoC_KEY,pch)) ){
      return BATTERY_SOC_ID;
    }else if( !(strcmp(PWM_EC_KEY,pch)) ){
      return PWM_EC_ID;
    }else if( !(strcmp(TEMP_FB_KEY,pch)) ){   
      return TEMP_FB_ID;
    }else if( !(strcmp(CAR_KILOMETRAGE_ID,pch)) ){
      return TEMP_RB_ID;
    }else if( !(strcmp(CAR_KILOMETRAGE_ID,pch)) ){
      return TEMP_MB_ID;
    }else if( !(strcmp(CAR_KILOMETRAGE_ID,pch)) ){
      return TEMP_BMS_ID;
    }else if( !strcmp(CELL_VOLT_KEY,pch) ) {
      pch = strtok (NULL, "=.");
      for ( int i=FIRST_CELL_VOLTAGE_ID ; i<=LAST_CELL_VOLTAGE_ID ; i++){
         if( atoi(pch) == i ){
              return i;
         }
      }
    }else{
      return 0;  
    }
}



void change_just_value(int resource_id, int menu_code){  //TO UPDATE A PARTICULAR VALUE AND NOT BLINK ALL THE DISPLAY ( Kilometrage x Example )
  if(menu_code==MAIN_MENU){
              if(resource_id==CAR_KILOMETRAGE_ID){
             
                        lcd.setCursor( KMS_VALUE_CURSOR , 1); //after '='
                        lcd.print(resources_values[resource_id]); 
                        for(int i=0; i<CHARS_PER_LINE-KMS_VALUE_CURSOR-strlen(resources_values[resource_id]) ; i++){
                             lcd.print(" "); 
                        }
                          
              }else if(resource_id==TEMP_FB_ID){
               
                        lcd.setCursor( TMP_FB_VALUE_CURSOR , 2);
                        lcd.print(resources_values[resource_id]); 
                        lcd.print(DEGREES_SYMBOL);  //PRINTS '°' 
                        lcd.print("C"); 

                        for(int i=TMP_FB_VALUE_CURSOR+strlen(resources_values[resource_id])+2; i<Temp_Start_2 - 1 ; i++){

                             lcd.print(" "); 
                        }
                   
                       
              }else if(resource_id==TEMP_RB_ID){
                        lcd.setCursor( TMP_RB_VALUE_CURSOR , 2);
                        lcd.print(resources_values[resource_id]); 
                        lcd.print(DEGREES_SYMBOL);  //PRINTS '°' 
                        lcd.print("C"); 
                        for(int i=TMP_RB_VALUE_CURSOR+strlen(resources_values[resource_id]); i<CHARS_PER_LINE-1 ; i++){
                             lcd.print(" "); 
                        }
              }else if(resource_id==TEMP_MB_ID){
                        lcd.setCursor( TMP_MB_VALUE_CURSOR , 3);
                        lcd.print(resources_values[resource_id]); 
                        lcd.print(DEGREES_SYMBOL);  //PRINTS '°' 
                        lcd.print("C"); 
                                                                         
                        for(int i=TMP_MB_VALUE_CURSOR+strlen(resources_values[resource_id])+2; i<Temp_Start_2 - 1; i++){
                             lcd.print(" "); 
                        }
              }else if(resource_id==TEMP_BMS_ID){
                        lcd.setCursor( TMP_BMS_VALUE_CURSOR , 3);
                        lcd.print(resources_values[resource_id]); 
                        lcd.print(DEGREES_SYMBOL);  //PRINTS '°' 
                        lcd.print("C"); 
                        for(int i=TMP_BMS_VALUE_CURSOR+strlen(resources_values[resource_id]); i<CHARS_PER_LINE-1; i++){
                             lcd.print(" "); 
                         }
            
              
            
              }
   
   }else if(menu_code==MENU_CELLS_1_8){
          
            switch(resource_id){
                case 1:
                   case 3:
                      case 5:
                         case 7:
                             lcd.setCursor( CELL_1D_1_VALUE_CURSOR , (resource_id-1)/2);     //  (resource_id-1)/2  trick to set the line. 
                             lcd.print(resources_values[resource_id]); 
                             lcd.print("V");
                             for(int i=CELL_1D_1_VALUE_CURSOR+strlen(resources_values[resource_id]); i<Cell_start2-1; i++){
                                       lcd.print(" "); 
                             }
                             break;
               case 2:
                  case 4:
                     case 6:
                        case 8:
                             lcd.setCursor( CELL_1D_2_VALUE_CURSOR , (resource_id-1)/2);
                             lcd.print(resources_values[resource_id]); 
                             lcd.print("V");
                             for(int i=CELL_1D_2_VALUE_CURSOR+strlen(resources_values[resource_id]); i<CHARS_PER_LINE-1; i++){
                                       lcd.print(" "); 
                             }
                             break;
              default:
                   ;
                   
            }
  
   }else if(menu_code==MENU_CELLS_9_16){
              switch(resource_id){
                case 9:
                     lcd.setCursor( CELL_1D_1_VALUE_CURSOR , (resource_id-9)/2);     //  (resource_id-9)/2  trick to set the line. 
                     lcd.print(resources_values[resource_id]); 
                     lcd.print("V");
                     for(int i=CELL_1D_1_VALUE_CURSOR+strlen(resources_values[resource_id]); i<Cell_start2-1; i++){
                               lcd.print(" "); 
                     }
                   break;
                    
               case 11:
                  case 13:
                      case 15:
                           lcd.setCursor( CELL_2D_1_VALUE_CURSOR , (resource_id-9)/2);     //  (resource_id-9)/2  trick to set the line. 
                           lcd.print(resources_values[resource_id]); 
                           lcd.print("V");
                           for(int i=CELL_2D_1_VALUE_CURSOR+strlen(resources_values[resource_id]); i<Cell_start2-1; i++){
                                     lcd.print(" "); 
                   }
                   break;
               case 10:
                  case 12:
                     case 14:
                        case 16:
                   lcd.setCursor( CELL_1D_2_VALUE_CURSOR , (resource_id-9)/2);
                   lcd.print(resources_values[resource_id]); 
                   lcd.print("V");
                   for(int i=CELL_2D_1_VALUE_CURSOR+strlen(resources_values[resource_id]); i<CHARS_PER_LINE-1; i++){
                             lcd.print(" "); 
                   }
                   break;
              default:
                   ;
                   
            }
   
   }else if(menu_code==MENU_CELLS_17_24){

                 switch(resource_id){
                              case 17:
                                 case 19:
                                    case 21:
                                       case 23:
                                           lcd.setCursor( CELL_2D_1_VALUE_CURSOR , (resource_id-17)/2);     //  (resource_id-1)/2  trick to set the line. 
                                           lcd.print(resources_values[resource_id]); 
                                           lcd.print("V");
                                           for(int i=CELL_2D_1_VALUE_CURSOR+strlen(resources_values[resource_id]); i<Cell_start2-1; i++){
                                                     lcd.print(" "); 
                                           }
                                           break;
                             case 18:
                                case 20:
                                   case 22:
                                      case 24:
                                           lcd.setCursor( CELL_2D_2_VALUE_CURSOR , (resource_id-17)/2);
                                           lcd.print(resources_values[resource_id]); 
                                           lcd.print("V");
                                           for(int i=CELL_2D_1_VALUE_CURSOR+strlen(resources_values[resource_id]); i<CHARS_PER_LINE-1; i++){
                                                     lcd.print(" "); 
                                           }
                                           break;
                            default:
                                 ;
                                 
                   }
                           
   }else if(menu_code==MENU_PWM_CURRENT){
          if(resource_id==PWM_EC_ID){
              lcd.setCursor( 0,0);
              for(int i=0; i<CHARS_PER_LINE; i++){
                          lcd.print(" "); 
              }
              lcd.setCursor( (CHARS_PER_LINE-strlen(resources_values[resource_id]) ) / 2,0);
              lcd.print(resources_values[resource_id]);    
              
          }else if(resource_id==PWM_BATTERY_CHARGER_ID){
              lcd.setCursor( 0,1);
              for(int i=0; i<CHARS_PER_LINE; i++){
                          lcd.print(" "); 
              }
              lcd.setCursor( (CHARS_PER_LINE-strlen(resources_values[resource_id]) ) / 2,1);
              lcd.print(resources_values[resource_id]);    
          }else if(resource_id==BMS_CURRENT_ID){
             lcd.setCursor( 0,2);
              for(int i=0; i<CHARS_PER_LINE; i++){
                          lcd.print(" "); 
              }
              lcd.setCursor( (CHARS_PER_LINE-strlen(resources_values[resource_id]) ) / 2,2);
              lcd.print(resources_values[resource_id]);    
            
          }
         
   } 
  
}
//// PRINTS THE MENU. IF THE MENU TO BE PRINTED IS THE SAME BEING DISPLAYED, IT JUST CHANGES THE CORRESPONDING RESOURCE VALUE. 
int lcd_print( int menu_code, int resource_id){
bool print_all_the_menu=(resource_id == 0);
  
    if(menu_code==current_menu && !print_all_the_menu){  //just change the corresponding line.  
         change_just_value(resource_id,menu_code);




    }else if(menu_code==MAIN_MENU ){ 
         lcd.clear();
         lcd.print("OPEN SOURCE VEHICLE"); 
         lcd.setCursor( 4,1);
         lcd.print(D_KILOMETRAGE); 
         lcd.print(resources_values[CAR_KILOMETRAGE_ID]); 
         lcd.setCursor( 0,2);
         lcd.print(D_TFB); 
         lcd.print(resources_values[TEMP_FB_ID]); 
         lcd.print(DEGREES_SYMBOL);  //PRINTS '°' 
         lcd.print("C"); 
         lcd.setCursor( 10,2);
         lcd.print(D_TRB); 
         lcd.print(resources_values[TEMP_RB_ID]); 
         lcd.print(DEGREES_SYMBOL);  //PRINTS '°' 
         lcd.print("C");
         lcd.setCursor( 0,3);
         lcd.print(D_TMB);
         lcd.print(resources_values[TEMP_MB_ID]); 
         lcd.print(DEGREES_SYMBOL);  //PRINTS '°' 
         lcd.print("C");  
         lcd.setCursor( 10,3);
         lcd.print(D_TBMS);
         lcd.print(resources_values[TEMP_BMS_ID]); 
         lcd.print(DEGREES_SYMBOL);  //PRINTS '°' 
         lcd.print("C"); 
         current_menu=MAIN_MENU;
         return 0;
      
    }else if(menu_code==MENU_CELLS_1_8){
    char aux_str[10];
    char aux_num[3];
    int counter=1;
          lcd.clear(); 
          for (int i=0 ; i<4 ; i++){
           for (int j=0 ; j<13 ; j=j+10){
                 lcd.setCursor( j,i);
                 strcpy(aux_str,"C");
                 sprintf(aux_num,"%d",counter);
                 strcat(aux_str,aux_num);
                 strcat(aux_str,"=");
                 lcd.print(aux_str);
                 lcd.print(resources_values[counter]); 
                 lcd.print("V");
                 counter++;
           }
         }
           current_menu=MENU_CELLS_1_8;
           
    }else if(menu_code==MENU_CELLS_9_16){
          char aux_str[10];
          char aux_num[3];
          int counter=9;
      
          lcd.clear(); 
          for (int i=0 ; i<4 ; i++){
           for (int j=0 ; j<13 ; j=j+11){
                 Serial.println("HO");
                 lcd.setCursor( j,i);
                 strcpy(aux_str,"C");
                 sprintf(aux_num,"%d",counter);
                 strcat(aux_str,aux_num);
                 strcat(aux_str,"=");
                 lcd.print(aux_str);
                 lcd.print(resources_values[counter]);
                 lcd.print("V");
                 counter++;
           }
         }
              current_menu=MENU_CELLS_9_16;
    }else if(menu_code==MENU_CELLS_17_24){
          char aux_str[10];
          char aux_num[3];
          int counter=17;
          lcd.clear(); 
          for (int i=0 ; i<4 ; i++){
           for (int j=0 ; j<13 ; j=j+11){
                 lcd.setCursor( j,i);
                 strcpy(aux_str,"C");
                 sprintf(aux_num,"%d",counter);
                 strcat(aux_str,aux_num);
                 strcat(aux_str,"=");
                 lcd.print(aux_str);
                 lcd.print(resources_values[counter]);
                 lcd.print("V");
                 counter++;
           }
         }
               current_menu=MENU_CELLS_17_24;
    }else if(menu_code==MENU_PWM_CURRENT){
        lcd.clear();
        lcd.setCursor( (CHARS_PER_LINE-strlen(PWM_battery_charger) )/2,0);        
        lcd.print(PWM_battery_charger);
        lcd.setCursor(  ( (CHARS_PER_LINE-strlen(pwm_ec) )/2,1) ,1);
        lcd.print(pwm_ec);
        lcd.setCursor( ( (CHARS_PER_LINE-strlen(battery_current) )/2,1),2);
        lcd.print(battery_current);
        lcd.setCursor(0,3);
        lcd.print("   TO MAIN MENU   ");
        current_menu=MENU_PWM_CURRENT;
    }else{
       return ERROR_CODE;
    }
    return 0;
}
void setup() {

lcd.init();                      // initialize the lcd 
lcd.init();
lcd.backlight();
Serial.begin(9600);

//INITIAL VALUES ON DISPLAY
for(int i=0; i<NUMBER_OF_RESOURCES;i++){
    strcpy(resources_values[i],"___");
}
lcd_print(1,0);
}

void loop() {
  



strcpy(resources_values[TEMP_FB_ID],"11");
strcpy(resources_values[TEMP_RB_ID],"16");
strcpy(resources_values[TEMP_MB_ID],"25");
strcpy(resources_values[TEMP_BMS_ID],"59");
strcpy(resources_values[2],"3.23");
strcpy(resources_values[9],"4.23");

delay(5000);
lcd_print(1,0);
for(int i=0;i<500;i++){
 lcd_print(1,CAR_KILOMETRAGE_ID);
sprintf(resources_values[CAR_KILOMETRAGE_ID],"%d",i);
 

delay(2000);

}
//TO TEST IT 






}
