#ifndef __GPIO_H__
#define __GPIO_H__

/* Pinout
 * - P3.0 = input?
 * - P3.1 = input?
 * - P3.2 = relay (active high)
 * - P3.3 = G (active high)
 * - P3.4 = C (active high) / BUT3 (active low)
 * - P3.5 = DP (active high)
 * - P3.6 = D (active high) / BUT1 (active low)
 * - P3.7 = input?
 * - P1.0 = B (active high)
 * - P1.1 = BUT4 (active low)
 * - P1.2 = A (active high)
 * - P1.3 = F (active high)
 * - P1.4 = D2 (active high)
 * - P1.5 = D3 (active high)
 * - P1.6 = D1 (active high)
 * - P1.7 = E (active low) / BUT2 (active low)
 * - P5.4 = trigger in (active high, external pull-up) 
 */

#define GPIO_A  P12
#define GPIO_B  P10
#define GPIO_C  P34
#define GPIO_D  P36
#define GPIO_E  P17
#define GPIO_F  P13
#define GPIO_G  P33
#define GPIO_DP P35

#define GPIO_TRIGGER P54
#define GPIO_BUTTON1 P36
#define GPIO_BUTTON2 P17
#define GPIO_BUTTON3 P34
#define GPIO_BUTTON4 P11

#define GPIO_D1 P16
#define GPIO_D2 P14
#define GPIO_D3 P15

#define GPIO_RELAY  P32

#endif /* __GPIO_H__ */