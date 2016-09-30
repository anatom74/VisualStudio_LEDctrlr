#include "Arduino.h"
//class Foo : public Stream {
//private:
//	Stream *_dev; // Somewhere to store the object's pointer
//
//public:
//	Foo(Stream& dev) : _dev(&dev) {} // Store the pointer to the object
//	size_t write(uint8_t val) { return _dev->write(val); }  // Call a function on the stored object pointer
//};
//
//Foo myFoo(Serial); // Create a new instance of the class with the Serial object as a parameter


#define LED232 Serial1   //define Serial port used for LED ctrlr

void setup()
{
	Serial.begin(115200);
	LED232.begin(115200);
  /* add setup code here */
}

void loop()
{
	//LED232.println("TEST_COM");
	//LED232.println(millis());  // Print something via your class.
//	char CC[50] = {};
//	char Cmdout[100] = {};/
//	int len;
//	int packetlen;
	char *string;
	string = "hey";
	
	//char cout = *CCcmd;

	while(1)
	{
		for (int x = 1; x < 4; x++)
		{

			delay(500);

			LEDPlayProg_compact(0xFF, 0, x);
			delay(500);

			LEDPlayTxtWindow_compact(0xFF, 0, 0, 5, 100, 3, 2,255,255,255, string);
		
		}
	}
}



/**
	Compose CC data for Play desired program on LED screen.

	@ID - id of display (0xFF for any)
	@option Bit0: Whether to save select play message to flash.    0 not to save，1 save。Bit1~7: Reserved, set to 0
	@prog_num Number of program to play 1~255 or 0
	note that limit is for 1 program at once only!!!!
*/
void LEDPlayProg_compact(uint8_t ID, uint8_t option, uint8_t prog_num)  //option
{
	char CCout[100] = {};
	int pCCoutlength;
	char Cmd_out[100] = {};
	int paclen;

  //function logic
	CCout[0] = 0x08;
	CCout[1] = option;
	CCout[2] = 0x01;  //1 program only
	CCout[3] = prog_num;
	pCCoutlength = 4;

	LEDPackCmd(ID, CCout, pCCoutlength, Cmd_out, &paclen);//pack CMD data
	SendLEDText232(Cmd_out, paclen); //send CMD data

}


/**
Compose CC data for: Play text in window CC=0x12

@ID - id of display (0xFF for any)
@window  - window number 0-7
@effect - effects for text (0-draw, 1 open from čleft,... se document Special effect for text and picture 
@alignment - alignment of text (0-2) Bit0~1: the horizontal alignment (
//////////////0: Left-aligned 
//////////////1: Horizontal center 
//////////////2: Right- aligned)
//////////////Bit2~3: vertical alignment (
//////////////0:Top-aligned
//////////////1:Vertically center,
//////////////2: Bottom- aligned)
//////////////Bit4~6: Reserved, set to 0
//////////////Bit7: Reserved, set to 0
@speed 1-100 (100 fastest)
@stayTime: seconds
@font: Bit0~3: font size,see：Font size code 0	8
//////1	12
//////2	16
//////3	24
//////4	32
//////5	40
//////6	48
//////7	56

//////Bit4~6 :font style,see :Font style code Font style code 	Font style describe
//////0	Default font style
//////1	Font style 1
//////2	Font style 2
//////3	Font style 3
//////4	Font style 4
//////5	Font style 5
//////6	Font style 6
//////7	Font style 7

//////Bit7 : reserved (set to 0)
@Red, Green, Blue: 0-255 color component
@*Text  - text string to show

*/
void LEDPlayTxtWindow_compact(uint8_t ID, uint8_t window, uint8_t effect, uint8_t alignment, uint8_t speed, uint16_t stayTime, uint8_t font, uint8_t Red, uint8_t Green, uint8_t Blue, char *Text)  //option
{
	
	char CCout[100] = {};
	int pCCoutlength;
	char Cmd_out[100] = {};
	int paclen;

	int length;
	char *temp = Text;
// function logic
	CCout[0] = 0x12;
	CCout[1] = window;
	CCout[2] = effect;  //special effect
	CCout[3] = alignment;
	CCout[4] = speed;
	CCout[5] = highByte(stayTime);
	CCout[6] = lowByte(stayTime);
	CCout[7] = font;
	CCout[8] = Red;  //R 0-255
	CCout[9] = Green; //G 0-255
	CCout[10] = Blue; //B

	length = strlen(temp);

	for (int i = 0; i < length; i++)
	{
		CCout[11 + i] = Text[i];
	}

	pCCoutlength = 11 + length;

	LEDPackCmd(ID, CCout, pCCoutlength, Cmd_out, &paclen);//pack CMD data
	SendLEDText232(Cmd_out, paclen); //send CMD data


}


/**
Compose Led command package for sending over serial.

@id ID of the controller card (FF for all)
@*CCdata CCdata sub command
@*CClength length of CCdata command buffer
@*outbuffer output buffer of packed command
@*bufflength legth of outbuffer
*/
void LEDPackCmd(uint8_t id, char *CCdata, int CClength, char *outbuffer, int *pBufflength)
{
	uint16_t crc = 0;
	int count = 0;

	outbuffer[count++] = 0x68;
	outbuffer[count++] = 0x32;
	outbuffer[count++] = id;
	outbuffer[count++] = 0x7B;
	outbuffer[count++] = 0x00; //0-not returning back info, 1 - return back info
	outbuffer[count++] = lowByte(CClength);
	outbuffer[count++] = highByte(CClength);
	outbuffer[count++] = 0x00;
	outbuffer[count++] = 0x00;

	for (int i = 0; i < CClength; i++)
	{
		outbuffer[count++] = CCdata[i];
	}

	for (int i = 0; i < count; i++)	//calculation of packet CRC
	{

		uint8_t a = outbuffer[i]; // to cast a number- if not, 128 is negative
		crc = crc + a;
	}

	outbuffer[count++] = lowByte(crc);
	outbuffer[count++] = highByte(crc);

	//strcat(LEDcmd, buffer);
	*pBufflength = count;
}


/**
Send command packet to Led controller over serial port

adds start and end byte and inserts needed escaped chars
*/
void SendLEDText232(char *buffer, int bufflen) //add start+stop and escaped chars and send them to serial port
{
	uint8_t tempval;
	uint8_t cnt = 0;
	char outbuf[50] = {};

	outbuf[0] = 0xa5; //start byte
	cnt++;

	for (int i = 0; i < bufflen; i++)  //check for inception of ecsaped char
	{
		tempval = buffer[i];

		switch (tempval)
		{
		case 0xa5:
			outbuf[cnt] = 0xaa;
			cnt++;
			outbuf[cnt] = 0x05;
			cnt++;
			break;
		case 0xae:
			outbuf[cnt] = 0xaa;
			cnt++;
			outbuf[cnt] = 0x0e;
			cnt++;
			break;
		case 0xaa:
			outbuf[cnt] = 0xaa;
			cnt++;
			outbuf[cnt] = 0x0a;
			cnt++;
			break;
		default:
			outbuf[cnt] = buffer[i];
			cnt++;
			break;
		}

	}
	outbuf[cnt] = 0xae;//end byte
	cnt++;

	for (int i = 0; i < cnt; i++)  //outbuffer scan
	{
		LED232.write(outbuf[i]);
		Serial.print(outbuf[i]);//debug
	}
}

