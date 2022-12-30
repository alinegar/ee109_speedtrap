extern volatile int state;
extern volatile unsigned int time, speed, cnt, buzzCNT, r, new_speed;
extern volatile unsigned char timer, sensor, buzzer;  // Flags

void timer2_init(void);
void timer1_init(void);
void timer0_init(void);
void screenSetup(void);