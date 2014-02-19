/*
 BUZZCARD HID RFID Format: Standard corp 1k
   - http://www.midwestbas.com/store/media/pdf/infinias/customer_wiegand_card_formats.pdf

  PIN # declaration
*/
#define BEEPER_PIN 4
#define RED_PIN 5 // Red LED
#define GREEN_PIN 6 // GREN LED
#define HOLD_PIN 7 //Stop the RFID Reader

/*
	DEBUG IFDEF Declaration
*/
#define DEBUG 0

/*
  RFID parsing information
*/
#define ID_START_BIT 14 //The starting bit of the Id stored in buzzcard
#define ID_BIT_LENGTH 20 //Bit Length of ID stored in buzzcard
#define BUZZCARD_BIT_LENGTH 35 //How many bits (total) are stored in the buzzcard
/*
  Static Globals
*/
static volatile byte RFID_Data[35]; //Have to store raw RFID data as array as its too long to fit in a natural arduino datatype
static volatile int RFID_bitsRead = 0; //How many bits have we read so far?
static int hold = false;

#include <SPI.h>
#include <boards.h>
#include <services.h> 
#include <keyboard.h>

void setup()
{
  Serial.begin(57600);

  // Attach pin change interrupt service routines from the Wiegand RFID readers
  attachInterrupt(0, dataZero_High, RISING);//DATA0 to pin 3 - data
  attachInterrupt(4, dataOne_High, RISING); //DATA1 to pin 7 - clock
  delay(10);
  
  /* Beeper Setup */
  pinMode(BEEPER_PIN, OUTPUT);
  digitalWrite(BEEPER_PIN, HIGH);
  /* Red LED Setup */
  pinMode(RED_PIN, OUTPUT);
  digitalWrite(RED_PIN, HIGH);
  /* Green LED Setup */
  pinMode(GREEN_PIN, OUTPUT);
  digitalWrite(GREEN_PIN, HIGH);
  /* RFID Hold Setup */
  pinMode(HOLD_PIN, OUTPUT);
  digitalWrite(HOLD_PIN, HIGH);

  // put the reader input variables to zero
  for(int i = 0; i < 35; i++) {
    RFID_Data[i] = 0x00;
  }
  RFID_bitsRead = 0;
}

void loop() {
  if(RFID_bitsRead == BUZZCARD_BIT_LENGTH) {
    RFID_do_events();

  /*
    If we read more than BUZZCARD_BIT_LENGTH,
    then we've had an error somewhere.  
    Therefore we pause and reset
  */
  } else if(RFID_bitsRead > BUZZCARD_BIT_LENGTH) { 
    delay(3500); //STOP Executing for ~3.5 secs..aka reset
    RFID_reset();
  }

  /*
    When not being interrupted by RFID Reader, 
    and done with parsing of RFID codes....
    we do our ble events (advertising, reading, writing)
  */
  if(!hold) {

  }
}


/*
  HARDWARE INTERRUPTS
 */
void dataZero_High(void) { //Binary0
  RFID_Data[RFID_bitsRead] = 0;
  ++RFID_bitsRead;
  //Put on a hold
  if(!hold) {
    RFID_lock();
  }
}

void dataOne_High(void) { //Binary1
  RFID_Data[RFID_bitsRead] = 1;
  ++RFID_bitsRead;
  //Put on a hold
  if(!hold) {
    RFID_lock();
  }
}
/*
  Functions
*/
void RFID_do_events() {
  if(RFID_bitsRead >= 35) { //Why is this condition here? This function is called when RFID_bitsRead == BUZZCARD_ID
    unsigned long Card_Data = parseId();
	
	//print parsed card ID (num) to Serial port if in DEBUG mode
	#ifdef DEBUG
		Serial.println(Card_Data);
	#endif
	
    RFID_reset();
  }
}

void RFID_lock() {
  Serial.println("Hold engaged");
  hold = true;
  digitalWrite(HOLD_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(RED_PIN, HIGH);
}

void RFID_reset() {
  RFID_bitsRead = 0;
  for(int i = 0; i < 35; ++i) {
    RFID_Data[i] = 0;
  }
  RFID_unlock(); //Resume RFID hardware interrupts will resume
}

void RFID_unlock() {
  Serial.println("Hold dis-engaged");
  hold = false;
  digitalWrite(HOLD_PIN, HIGH);
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, HIGH);
}




unsigned long parseId() {
  unsigned long parsedId = 0;
  for(int i = ID_START_BIT; i < (ID_START_BIT + ID_BIT_LENGTH); i++) {
      parsedId <<= 1; //Move parsedID over one bit to leave LSB open for RFID_Data
      parsedId |= RFID_Data[i] & 0x1; //RFID data stored in first bit
  }
  return parsedId;
}
/*
  Convert buzzcard ID to a string, with a newline terminator
*/
int id2str(char * destination, unsigned long id) {
 int length = sprintf(destination, "%lu\n", id);
 return length;
}

/*
    Send destination param of id2str to computer via keyboard library
    @param String pointer to the ID
    @param Length of id string
    @return 1 on sucess, 0 on fail
*/
int send_str( char * id_str, int id_length) {
  int tot_length;
  int length = 0;
  Keyboard.begin();
  for(int i=0; i<id_length; i++){
    length = Keyboard.print(id_str[i]);//on sucess length should always be 1 byte
    #ifdef DEBUG
      Serial.print("send_str: Sent character "
      Serial.print(id_str[i]);
      Serial.print("length = ");
      Serial.println(length);
    #endif 
    if(length != 1) {
      Keyboard.end();
      return 0;
    }
    ++tot_length;
    length = 0;
  }
  Keyboard.end();
  if(tot_length != id_length){
     #ifdef DEBUG
       Serial.println("Error(send_str): tot_length sent via keyboard does not equal to expected id_length\ntot_length = %d, id_length = %d", tot_length, id_length);
     #endif 
     return 0;
  } else {
     return 1; 
  }
  
 
}

/*
  Send the buzzcard ID through Bluetooth LE
*/
/*
void sendId(unsigned long id) {
  char buffer[11]; //max unsinged long = '4294967295\n'
  int length = id2str(buffer, id);
  for(int i = 0; i < length; ++i) {
    ble_write(buffer[i]);
  }
}
*/

