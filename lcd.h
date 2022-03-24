#ifndef __LCD_H__
#define __LCD_H__

enum SegmentSymbols { SYMBOL_DASH };

/* Draw a number formed as <integer>.<decimal>, where decimal is in [0,9] interval */

void lcd_draw_fractional(uint16_t integer, uint16_t decimal);
void lcd_draw_number(int num);
void lcd_draw_text(const char* text);
void lcd_draw_milliseconds(uint32_t t);

void lcd_init(void);
void lcd_render(void);

#endif /* __LCD_H__ */