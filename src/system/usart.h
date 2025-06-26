#ifndef __USART_H__
#define __USART_H__

void usart_setup(void);
int usart_send_char(const uint8_t *ch);
int usart_send_string(const char *str);
uint8_t usart_read_ch(uint8_t *ch);

#endif // __USART_H__
