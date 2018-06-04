
int line_sent_counter=0;
static int speed_counter=0;
static int kilometrage_counter=0;
static int battery_counter=100;
void setup() {
  // put your setup code here, to run once:
Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:


line_sent_counter++;
      char to_send[15];
      char help[10];
    if(line_sent_counter>=1 && line_sent_counter<=24){   // CELL VOLT 
      
      strcpy(to_send,"cs=");
      sprintf(help,"%d",line_sent_counter);
      strcat(to_send,help);
      strcat(to_send,".");
      sprintf(help,"%d",random(0,400));
      strcat(to_send,help);
      strcat(to_send,"\n");
          Serial.write(to_send , strlen(to_send) ) ;

           
      }else if(line_sent_counter==25 ){  //TEMPS 
        strcpy(to_send,"ta=");
        sprintf(help,"%d",random(0,40));
        strcat(to_send,help);
        strcat(to_send,"\n");
        Serial.write(to_send , strlen(to_send) ) ;
     
      }else if(line_sent_counter==26){
         strcpy(to_send,"tb=");
        sprintf(help,"%d",random(0,40));
        strcat(to_send,help);
        strcat(to_send,"\n");
        Serial.write(to_send , strlen(to_send) ) ;
        
      }else if(line_sent_counter==27){  //SOC
         strcpy(to_send,"tc=");
        sprintf(help,"%d",random(0,40));
        strcat(to_send,help);
        strcat(to_send,"\n");
        Serial.write(to_send , strlen(to_send) ) ;
    
      }else if(line_sent_counter==28){ 
         strcpy(to_send,"td=");
        sprintf(help,"%d",random(0,40));
        strcat(to_send,help);
        strcat(to_send,"\n");
        Serial.write(to_send , strlen(to_send) ) ;
      
      }else if(line_sent_counter==29){ 
        strcpy(to_send,"pec=");
        sprintf(help,"%d",random(0,100));
        strcat(to_send,help);
        strcat(to_send,"\n");
        Serial.write(to_send , strlen(to_send) ) ;
   
      }else if(line_sent_counter==30){ 
        strcpy(to_send,"%bat=");
        battery_counter=battery_counter-10;
        sprintf(help,"%d", battery_counter);
        strcat(to_send,help);
        strcat(to_send,"\n");
        Serial.write(to_send , strlen(to_send) ) ;
        
      }else if(line_sent_counter==31){ 
        strcpy(to_send,"pbc=");
        sprintf(help,"%d",random(0,100));
        strcat(to_send,help);
        strcat(to_send,"\n");
        Serial.write(to_send , strlen(to_send) ) ;
       
      }else if(line_sent_counter==32){ 
        strcpy(to_send,"km/h=");
        sprintf(help,"%d",speed_counter); 
        strcat(to_send,help);
        strcat(to_send,"\n");
        Serial.write(to_send , strlen(to_send) ) ;
      
       speed_counter= speed_counter+10;
      }else if(line_sent_counter==33){ 
        strcpy(to_send,"kms=");
        kilometrage_counter=kilometrage_counter+random(0,20);
        sprintf(help,"%d",kilometrage_counter); 
        strcat(to_send,help);
        strcat(to_send,"\n");
        Serial.write(to_send , strlen(to_send) ) ;
      
      }else{
        line_sent_counter=0;
      }
        delay(1100);
//     


}
