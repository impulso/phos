

#define NBPLOTS 10


#define RED_PIN 3
#define BLUE_PIN 6
#define LEDOFF 13
#define LEDON 8

#define TERMBAUDRATE 57600
#define BTBAUDRATE 57600

//uncomment to allow usb serail monitoring
//Leonardo use Serial1 for TTL and Serial for USB
#define MONITORING
#ifdef MONITORING
//if you want to use other Serial port then adapt line below
#define BT_BEGIN(x)      Serial.begin(x);while (!Serial) {}
#define BT_AVAILABLE    Serial.available
#define BT_READ       Serial.read
#define TERM_BEGIN(x)   Serial.begin(x)
#define TERM_PRINT(x)   Serial.print (x)
#define TERM_PRINTLN(x)   Serial.println (x)
#define TERM_PRINTDEC(x)  Serial.print (x, DEC)
#define TERM_PRINTDECLN(x)  Serial.println (x, DEC)

#else
#define BT_BEGIN(x)     Serial.begin(x);while (!Serial) {}
#define BT_AVAILABLE    Serial.available
#define BT_READ       Serial.read
#define TERM_BEGIN(x)   
#define TERM_PRINT(x)
#define TERM_PRINTLN(x)
#define TERM_PRINTDEC(x)
#define TERM_PRINTDECLN(x)
#endif


#ifdef DEMO
byte attention = 100;
#else
byte attention = 0;
#endif

byte meditation = 0;


 static byte poorQuality = 200;


static float attentionTarget = 0;
static float attentionResult = 0;
static float meditationTarget = 0;
static float meditationResult = 0;
static unsigned long lastEegManagerMillis=0;
  

float rval;
float bval;





void setup(){
  TERM_BEGIN(TERMBAUDRATE);     
  BT_BEGIN(BTBAUDRATE);
  pinMode(RED_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(LEDOFF, OUTPUT);
  pinMode(LEDON, OUTPUT);
}

void remapAndRamp(float input, float &target, float &result, float increment, float decrement, const float plots[NBPLOTS][2]){
  if(target>result) {
    result+=increment;
    if(result>target) result=target;
  }
  else if(target<result) {
    result-=decrement;  
    if(result<target) result=target;
  }
  
  for(int i=0;i<NBPLOTS;i++) {
    if(input<=plots[i][0]) {
      target=plots[i][1];
      break;
    }
  }
}

//eeg manager
float eegResult = 0;
float difficultyCoef=0.4;
void eegManager(){
  
  if((millis()-lastEegManagerMillis) > 10) {
    lastEegManagerMillis=millis();
    
    const float eegPlots [NBPLOTS][2] = {
      {10,5.0}, {20,10.0}, {30,15.0}, {40,20.0}, {50,30.0}, 
      {60,50.0}, {70,75.0}, {80,95.0}, {90,100.0}, {100,100.0}};
   
    remapAndRamp(attention, attentionTarget, attentionResult, .1, .1, eegPlots);
    remapAndRamp(meditation, meditationTarget, meditationResult, .1, .1, eegPlots);
    #ifdef DEMO
    static unsigned long lastDemoMaxResultMillis=0;
    if((millis()-lastDemoMaxResultMillis) > 1000) {
      TERM_PRINT(" Attention: ");
      TERM_PRINTDEC(attention);
      TERM_PRINT(" Meditation: ");
      TERM_PRINTDEC(meditation);
      TERM_PRINT(" eegResult: ");
      TERM_PRINTDECLN(eegResult);
      lastDemoMaxResultMillis=millis();
      static byte count=0;
      if(attentionResult>=100.0) count+=1;
      if(count>30) {
        attention=0;  
        difficultyCoef=0.5; 
      }
    }
    #endif
  }
}

void lightManager(){

  rval= map (attentionResult, 0,100, 0, 255);
  bval= map (attentionResult, 0,100, 255, 0);

  if(poorQuality > 50) {
    digitalWrite (RED_PIN, LOW);
    digitalWrite(BLUE_PIN,LOW);

  }
  else {

  analogWrite(RED_PIN, (int)rval);
  analogWrite(BLUE_PIN, (int)bval);

  }

    
}

//neurosky code
//original: http://developer.neurosky.com/docs/doku.php?id=arduino_tutorial 
////////////////////////////////
// Read data from Serial UART //
////////////////////////////////
unsigned long lastReceivedPacketMillis = 0;
byte ReadOneByte(){
  int ByteRead;
  while(!BT_AVAILABLE()){
    eegManager();
    lightManager();  
  };

  ByteRead = BT_READ();
#ifdef DEBUGOUTPUT
  TERM_PRINT((char)ByteRead);   // echo the same byte out the USB serial (for debug purposes)
#endif
  lastReceivedPacketMillis = millis();
  return ByteRead;
}
/////////////
//MAIN LOOP//
/////////////
void loop(){
  static byte generatedChecksum = 0;
  static byte checksum = 0;
  static int payloadLength = 0;
  static byte payloadData[64] = {0};

  static boolean bigPacket = false;

  // Look for sync bytes
    if(ReadOneByte() == 170){
    if(ReadOneByte() == 170){
      //TERM_PRINTLN("nada");
      payloadLength = ReadOneByte();
      if(payloadLength > 169)      //Payload length can not be greater than 169
        return;
      generatedChecksum = 0;
      for(int i = 0; i < payloadLength; i++){
        payloadData[i] = ReadOneByte();        //Read payload into memory
        generatedChecksum += payloadData[i];
      }
      checksum = ReadOneByte();     //Read checksum byte from stream
      generatedChecksum = 255 - generatedChecksum;   //Take one's compliment of generated checksum
      if(checksum == generatedChecksum){
        for(int i = 0; i < payloadLength; i++) {
          // Parse the payload
          switch (payloadData[i]){
            case 2:
              i++;
              poorQuality = payloadData[i];
              bigPacket = true;
              break;
            case 4:
              i++;
              attention = payloadData[i];
              break;
            case 5:
              i++;
              meditation = payloadData[i];
              break;
            case 0x80:
              i = i + 3;
              break;
            case 0x83:
              i = i + 25;
              break;
            default:
              break;
          } // switch
        } // for loop
#ifndef DEBUGOUTPUT
        //*** Add your code here ***
        if(poorQuality!=0) {
        digitalWrite(LEDOFF, HIGH);
        digitalWrite(LEDON, LOW);
        

        }
  
        else {
          
          digitalWrite(LEDOFF, LOW);  
          digitalWrite(LEDON, HIGH);
        
        }
#endif
        bigPacket = false;
      } else {
        // Checksum Error
      }  // end if else for checksum
    } // end if read 0xAA byte
  } // end if read 0xAA byte
}
//end of neurosky code
