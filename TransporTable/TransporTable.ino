
// Intel Edison - TransporTable
// Author: Sven.Eliasson@gmx.de

#include <SPI.h>
#include <Ethernet.h>
#include <Time.h>
#include <Wire.h>
#include "rgb_lcd.h"

rgb_lcd lcd;
const int colorR = 255;
const int colorG = 0;
const int colorB = 0;

// globals 
byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress server(195,30,121,20); // Server is efa.mvv-muenchen.de
char  readstring[10000]; 
String bodyString; 
int i = 0;
int timout_counter = 0; 
EthernetClient client;

// cmd states
#define INIT_REQUEST    100
#define WAIT_FOR_BODY   101
#define COLLECT_BODY    102
#define EVAL_BODY       103
#define STOP            200
#define GET_TIME        104


int decodeState;
String date_;
String time_; 

void getTime_(void){
     
  char *cmd = "/bin/date +%F%t%T"; // outputs as 2013-10-21     22:25:00
  FILE *ptr;
  char buf[64];
  if ((ptr = popen(cmd, "r")) != NULL){
    while (fgets(buf, 64, ptr) != NULL){
      //Serial.print(buf);
    }
  date_ = String(buf[0])+String(buf[1])+String(buf[2])+String(buf[3])+String(buf[5])+String(buf[6])+String(buf[8])+String(buf[9]); 
  time_ = String(buf[11]) + String(buf[12]) + String(buf[14]) + String(buf[15]);
  
  }
  (void) pclose(ptr);
}

void init_request(void){
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    decodeState = STOP;
    }delay(3000);
    Serial.println("connecting...");
    
  if (client.connect(server, 80)) {
    Serial.println("Connected to sever - sending request");
    String cmd ="GET /xhr_departures?locationServerActive=1&stateless=1&type_dm=any&name_dm=1001170&useAllStops=1&useRealtime=1&line=mvv%3A21002%3A+%3AH%3As15&line=mvv%3A21002%3A+%3AR%3As15&line=mvv%3A21007%3A+%3AH%3As15&line=mvv%3A21007%3A+%3AR%3As15&line=mvv%3A22015%3A+%3AH%3As15&line=mvv%3A22015%3A+%3AR%3As15&line=mvv%3A22025%3A+%3AH%3As15&line=mvv%3A22025%3A+%3AR%3As15&line=mvv%3A22027%3AN%3AH%3As15&line=mvv%3A22027%3AN%3AR%3As15&line=mvv%3A23058%3A+%3AR%3As15&line=mvv%3A23045%3AN%3AH%3As15&line=mvv%3A20030%3AX%3AH%3As15&limit=18&mode=direct&zope_command=enquiry%3Adepartures&itdDate=";
    cmd = cmd + date_+ "&itdTime=" + time_ + "&_=1430609400761 HTTP/1.0";
    //client.println("GET /xhr_departures?locationServerActive=1&type_dm=any&name_dm=1001170&useAllStops=1&useRealtime=0&line=mvv%3A21002%3A+%3AH%3As15&line=mvv%3A21002%3A+%3AR%3As15&line=mvv%3A22015%3A+%3AH%3As15&line=mvv%3A22015%3A+%3AR%3As15&line=mvv%3A22025%3A+%3AH%3As15&line=mvv%3A22025%3A+%3AR%3As15&line=mvv%3A22027%3AN%3AH%3As15&line=mvv%3A22027%3AN%3AR%3As15&line=mvv%3A23058%3A+%3AR%3As15&line=mvv%3A23045%3AN%3AH%3As15&limit=5&mode=direct&zope_command=enquiry%3Adepartures&itdDate=20150504&itdTime=1129&_=1430609400761 HTTP/1.0");
    client.println(cmd);
    client.println("\r\n");
    delay(100);
  }else{ Serial.println("connection failed"); }
  decodeState = WAIT_FOR_BODY;
  Serial.print("Wait for body\n");
}

int returnTimeDiff(String time_con){
  
  int hours_c =(time_con.substring(1,2)).toInt();
  int minutes_c =(time_con.substring(3,5)).toInt();
  
  int hours_t =(time_.substring(1,2)).toInt();
  int minutes_t =(time_.substring(2,4)).toInt();
    
  int min_diff = minutes_c - minutes_t;
  min_diff += 60*(hours_c-hours_t);
  Serial.print("\n");
  Serial.print(minutes_c); 
  Serial.print("\n");
  Serial.print(time_);
  Serial.print(minutes_t);
  Serial.print("\n");
  Serial.print("\n");
  return min_diff; 
  }

void setup() {
   lcd.begin(16, 2);
   lcd.setRGB(colorR, colorG, colorB);
   
   Serial.begin(9600);
   while (!Serial){;}
   //decodeState = INIT_REQUEST;
   decodeState = INIT_REQUEST;
   getTime_();
}

void loop(){  
  
  if(decodeState == INIT_REQUEST){ init_request();}
  
  if(client.available() || client.connected() ){
    int h = client.available();
    for( h; h>0; h--){
      char c = client.read();    // collect string
      if (c != '\n' && c != '\r' && c != '\t' && c!='\v' && c!='\f' && c!=' ') {readstring[i++] = c;}           
      if (c == '\n'){ // after linefeed check content       
                 
        switch(decodeState){
          
          case COLLECT_BODY: // wait till all body chars are collected
            if( strstr(readstring, "</tbody>")){
              readstring[i] = '\0';
              Serial.print(readstring);
              client.stop();
              decodeState = EVAL_BODY;
              bodyString = readstring; 
            }break;
            
          case WAIT_FOR_BODY:  // as long as no body detected  -> discard string
            readstring[i++] = '\n';
            readstring[i++] = '\0';
            Serial.print(readstring); 
            if( strstr(readstring, "<tbody>")){
              decodeState = COLLECT_BODY;
              Serial.print("Body detected:  \n");  
            }
            i =0; // reset string index (overwrite)
            break;
        }      
      }
    }
  }
  
  if(decodeState == EVAL_BODY){
  int j = 0; int k =0;
  String message; 
  
  for(int num =0; num <=7; num++){
    j = bodyString.indexOf("\"printable\">",j) + strlen("\"printable\">");
    k = bodyString.indexOf("</span>",j);
    if( k-j>3 || k-j < 0 ){ k=j+3;}
    //Serial.print( bodyString.substring(j,k) ); Serial.print("-");
    message = bodyString.substring(j,k);
    message = bodyString.substring(j,k) +" ";
    
    j = bodyString.indexOf("<tdwidth=\"75%\">",k) + strlen("<tdwidth=\"75%\">");
    k = bodyString.indexOf("<spanstyle",j);
    if( k-j>10 || k-j < 0 ){ k=j+10;}
    //Serial.print( bodyString.substring(j,k) ); Serial.print("-");
    message += bodyString.substring(j,k);
    
    j = bodyString.indexOf("\"dm_time\">",k) + strlen("\"dm_time\">");
    k = bodyString.indexOf("</td>",j);
    if( k-j>10 || k-j < 0 ){ k=j+10;}
    //Serial.print( bodyString.substring(j,k) );
    Serial.print("\n");
    
    lcd.setCursor(0, 0);
    lcd.print("                ");
     lcd.setCursor(0, 0);
    lcd.print(message);
    lcd.setCursor(14, 0);
    lcd.print(returnTimeDiff(bodyString.substring(j,k)));
    Serial.print(message);
    
    ////////NEXT CONNECTION /////////////////
    j = bodyString.indexOf("<spanclass=\"printable\">",j) + strlen("<spanclass=\"printable\">");
    //j = bodyString.indexOf("title=",j) + strlen("title=  ");
    k = bodyString.indexOf("</span>",j);
    if( k-j>3 || k-j < 0 ){ k=j+3;}
    Serial.print( bodyString.substring(j,k) ); Serial.print("-");
    message = bodyString.substring(j,k) +" ";
   
    
    j = bodyString.indexOf("<tdwidth=\"75%\">",k) + strlen("<tdwidth=\"75%\">");
    k = bodyString.indexOf("<spanstyle",j);
    if( k-j>10 || k-j < 0 ){ k=j+10;}
    //Serial.print( bodyString.substring(j,k) ); Serial.print("-");
    message += bodyString.substring(j,k);
    
    j = bodyString.indexOf("\"dm_time\">",k) + strlen("\"dm_time\">");
    k = bodyString.indexOf("</td>",j);
    if( k-j>10 || k-j < 0 ){ k=j+10;}
    //Serial.print( bodyString.substring(j,k) );

    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(message);
    lcd.setCursor(14, 1);
    lcd.print(returnTimeDiff(bodyString.substring(j,k)));
    Serial.print(message);
    
    
    
    delay(1000); 
    
    
    }
    decodeState == GET_TIME;
  }   
  
  if( decodeState == GET_TIME){
    getTime_();
    decodeState == INIT_REQUEST;
  }
  
  
}

