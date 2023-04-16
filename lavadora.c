#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
  DDRB = 0x08; //Configuracion del puerto

  //Parpadear
  while (1) {
    PORTB = 0x00; //PORTB &= ~(1 << PB3); //Esto se puede hacer tambien asi
    _delay_ms(500); 
    PORTB = 0x08; //PORTB |=  (1 << PB3); //Esto se puede hacer tambien asi
    _delay_ms(500); 
  }
}
