extern volatile unsigned int dataCNT;
extern volatile unsigned char dataValid, startData;  // Flags
extern volatile unsigned char charBuff[5];  //5 byte buffer

void tx_send(char c);