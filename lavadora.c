#include <avr/interrupt.h>

// Definiciones de estados para la FSM
#define IDLE      0   //  Esperando inicio/reanudación del lavado
#define CONFIG    1   //  Configuración de la lavadora 
#define AGUA      2   //  Llenado del tanque de agua
#define LAVADO    3   //  Lavado de la ropa
#define ENJUAGUE  4   //  Enjuague de la ropa
#define SECADO    5   //  Secado de la ropa

// Definiciones globales
int inicio_pausa        = 0;
int estado              = IDLE;
int siguiente_estado    = IDLE;
int ciclo_activo        = IDLE;
int tanque_lleno        = 0;
int lavado_finalizado   = 0;
int enjuague_finalizado = 0;
int secado_finalizado   = 0;
int segundos            = 0;
int ciclos              = 0;
int overflows_1s        = 31;    // 1s ~ 31 overflows         
int nivel_agua          = 0;

// Funciones

void delay_1s(){
  TCNT0   = 0x00;                     // Se inicializa el contador interno en 0
  ciclos  = 0;
  TIMSK   = 0b10;                     // Se activa la interrupción por OVF
  while(ciclos < overflows_1s){}      // Se cuentan los overflows
  TIMSK   = 0b00;                     // Se desactivan las interrupciones por OVF
}

void mostrar_tiempo(int tiempo){
  switch(tiempo){
    case 0:
      PORTB = 0x00;
      break;

    case 1:
      PORTB = 0x80;
      break;

    case 2:
      PORTB = 0x40;
      break;

    case 3:
      PORTB = 0xC0;
      break;

    case 4:
      PORTB = 0x20;
      break;

    case 5:
      PORTB = 0xA0;
      break;

    case 6:
      PORTB = 0x60;
      break;

    case 7:
      PORTB = 0xE0;
      break;

    case 8:
      PORTB = 0x10;
      break;

    case 9:
      PORTB = 0x90;
      break;

    case 10:
      PORTB = 0x08;
      break;

    default:
      break;
  }
}

int main(void)
{
  // Configuración del MCU
  DDRA = 0x00; //Configuracion del puerto A
  DDRB = 0x11; //Configuracion del puerto B
  DDRD = 0x3C; //Configuracion del puerto D

  // Habilitamos interrupciones PCIE1 y PCIE2
  GIMSK   = 0x18;
  
  PCMSK1  |= 0x07; // PCIE1 disparada por PCINT8, PCINT9 y PCINT10
  PCMSK2  |= 0x01; // PCIE2 disparada por PCINT11
 
  // Configuración del timer0
  TCCR0A = 0x00; 
  TCCR0B = 0b101;  //  Prescaling de 1024
  TIMSK  = 0b00;   //  Se inicia con la interrupción por overflow (OVF) desactivada
  
  sei(); // Se habilitan las ISRs  
  
  while (1) {

    switch (estado){

      case IDLE:
        if (inicio_pausa){
          if (ciclo_activo == IDLE){
            siguiente_estado = CONFIG;
          } else {
            siguiente_estado = ciclo_activo;  
          }  
        }
        break;

      case CONFIG:
        if (!nivel_agua){
          siguiente_estado = CONFIG;
        } else {
          siguiente_estado = AGUA;
        }
        break;

      case AGUA:
        PORTD = 0x20;
        if (!inicio_pausa){
          ciclo_activo = AGUA;
          siguiente_estado = IDLE;
        } else if (tanque_lleno){
          siguiente_estado = LAVADO;
          segundos = 0;
        } else {
          siguiente_estado = AGUA;
          delay_1s();
          ++segundos;
          mostrar_tiempo(segundos);
          if (segundos == 1*nivel_agua){
            tanque_lleno = 1;
          }
        }
        break;

      case LAVADO:
        PORTD = 0x10;
        if (!inicio_pausa){
          ciclo_activo = LAVADO;
          siguiente_estado = IDLE;
        } else if (lavado_finalizado){
          siguiente_estado = ENJUAGUE;
          segundos = 0;  
        } else {
          siguiente_estado = LAVADO;
          delay_1s();
          ++segundos;
          mostrar_tiempo(segundos);
          if (segundos == 3 && nivel_agua == 1){
            lavado_finalizado = 1;
          } else if (segundos == 7 && nivel_agua == 2){
            lavado_finalizado = 1;
          } else if (segundos == 10 && nivel_agua == 3){
            lavado_finalizado = 1;
          }
        }
        break;

      case ENJUAGUE:
        PORTD = 0x08;
        if (!inicio_pausa){
          ciclo_activo = ENJUAGUE;
          siguiente_estado = IDLE;
        } else if (enjuague_finalizado){
          siguiente_estado = SECADO;
          segundos = 0;
        } else {
          siguiente_estado = ENJUAGUE;
          delay_1s();
          ++segundos;
          mostrar_tiempo(segundos);
          if (segundos == 2 && nivel_agua == 1){
            enjuague_finalizado = 1;
          } else if (segundos == 4 && nivel_agua == 2){
            enjuague_finalizado = 1;
          } else if (segundos == 5 && nivel_agua == 3){
            enjuague_finalizado = 1;
          }
        }
        break;

      case SECADO:
        PORTD = 0x04;
        if (!inicio_pausa){
          ciclo_activo = SECADO;
          siguiente_estado = IDLE;
        } else if (secado_finalizado){
          siguiente_estado = IDLE;
          segundos = 0;
        } else {
          siguiente_estado = SECADO;
          delay_1s();
          segundos++;
          mostrar_tiempo(segundos);
          if (segundos == 3*nivel_agua){
            secado_finalizado = 1;
          }
        }
        break;

      default:
        break;
    }
    estado = siguiente_estado;
  }
}

// Interrupciones 

ISR(TIMER0_OVF_vect){ ciclos++; } // Interrupción del timer0     


ISR(PCINT_A_vect){        // Interrupción de los selectores del nivel de agua
  if (PORTA0){
    nivel_agua = 3;
  } else if (PORTA1) {
    nivel_agua = 2;
  } else if (PORTA2){
    nivel_agua = 1;
  } else {
    nivel_agua = 0;
  }
}

// Interrupción del botón de inicio/pausa
ISR(PCINT_D_vect){ inicio_pausa = !inicio_pausa; }       