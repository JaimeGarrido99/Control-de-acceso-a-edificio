#include <msp430.h> 
#include <string.h>
#include "grlib.h"
#include <stdlib.h>
#include "Crystalfontz128x128_ST7735.h"
#include "HAL_MSP430G2_Crystalfontz128x128_ST7735.h"
#include <stdio.h>

/*
 * main.c
 */

//Funciones
//----------------------------------------------------------------
void Set_Clk(char VEL);
void UART_SendString(const char *str);
void UARTprintCR(const char *frase);
int lee_ch(char canal);
void inicia_ADC(char canales);
int leer_ADC (int canal, int eje, int valor, int maxeje,
              int mineje, int maxvalor, int minvalor, int aumento);

//Variables
//-----------------------------------------------------------------
char * LeeFlash;
enum estados {ARRANQUE, SELEC_USER, PIN_US, MODO, MODO_ADMIN,
              INFORME, REINICIO };
char estados = ARRANQUE;
volatile char buffer[3] = { 0 }, PIN_in[5] = { 0 };
volatile int dia = 0, mes = 0, ano = 0, hora = 0, min = 0, seg = 0;
int j = 0, l = 0, intentos = 3, ejey = 512, y = 45, ya = 0;
char usuario[7], usuario_ant[7], acceso[90], PIN[4], fecha[15];
char usuario_bloq[7];
//0:Fuera del edificio
//1:Trabajando
//2:Pausa
//3:Bloqueo

unsigned int modo_ant[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned int modos[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int tiempos[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int tiempo_p[16] ={0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
volatile int fin = 0, tms = 0, fin_fecha = 0;
unsigned int pin_correcto = 0, t = 0, id = 0, i = 0, p = 0;

 Graphics_Context g_sContext;


 int main(void)
 {
     WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer


     BCSCTL2 = SELM_0 | DIVM_0 | DIVS_0;

     if (CALBC1_16MHZ != 0xFF) {
         __delay_cycles(100000);
         DCOCTL = 0x00;
         BCSCTL1 = CALBC1_16MHZ;     /* Set DCO to 16MHz */
         DCOCTL = CALDCO_16MHZ;}

     BCSCTL1 |= XT2OFF | DIVA_0;
     BCSCTL3 = XT2S_0 | LFXT1S_2 | XCAP_1;

     //Declaramos los LEDs del BoosterPack
     P2DIR |= BIT1 | BIT2 | BIT3 |BIT4;
     P2OUT &= ~BIT1;
     P2OUT &= ~BIT2;
     P2OUT &= ~BIT3;
     P2OUT &= ~BIT4;


     inicia_ADC(BIT0+BIT3);

     /*------ Pines de E/S involucrados:------------------------*/
     P1SEL2 = BIT1 | BIT2;  //P1.1 RX, P1.2: TX
     P1SEL = BIT1 | BIT2;
     P1DIR = BIT0 + BIT2;
     P1REN = BIT3;             //Boton en P1.3
     P1OUT = BIT3;

     //Declaro el boton del joystick
     P2DIR &= ~BIT5;
     P2OUT |= BIT5;

     /*------ Configuraci n de la USCI-A para modo UART:----------*/

     UCA0CTL1 |= UCSWRST; // Reset
     UCA0CTL1 = UCSSEL_2 | UCSWRST; //UCSSEL_2: SMCLK (16MHz) | Reset on
     UCA0CTL0 = 0; // 8bit, 1stop, sin paridad. NO NECESARIO
     UCA0BR0 = 139; // 16MHz/139=115108... //Velocidad
     UCA0CTL1 &= ~UCSWRST; /* Quita reset */
     IFG2 &= ~(UCA0RXIFG); /* Quita flag */
     IE2 |= UCA0RXIE; /* y habilita int. recepcion */

     //Temporizador de 10 ms
     TA1CTL = TASSEL_2 | ID_3 | MC_1;         //SMCLK, DIV=8 (2MHz), UP
     TA1CCR0 = 19999;       //periodo=20000: 10ms
     TA1CCTL0 = CCIE;      //CCIE=1

     //Inicialización de la pantalla
     Crystalfontz128x128_Init();
     Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);
     Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128);
     Graphics_setFont(&g_sContext, &g_sFontCm16b);
     Graphics_clearDisplay(&g_sContext);
     Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
     Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);

     UART_SendString("Introduzca la fecha y hora en formato DD/MM/AA HH:MM\r\n");

     // Mensaje por pantalla
     Graphics_drawString(&g_sContext, "Introduzca la", 13, 10, 40,
                         TRANSPARENT_TEXT);
     Graphics_drawString(&g_sContext, "fecha y hora", 13, 10, 60,
                         TRANSPARENT_TEXT);

     __bis_SR_register(GIE);

    while (1)
    {
        LPM0; //Bucle de bajo consumo

        if(fin_fecha == 1)
        {
            if (seg >= 60)
            {
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                sprintf(fecha, "%d/%d/%d  %d:%d", dia, mes, ano, hora, min);
                Graphics_drawString(&g_sContext, fecha, 20, 15, 5, TRANSPARENT_TEXT);

                for (p = 0; p < 15 ; p++)
                {
                    if (modos[p] == 1)
                    {
                        tiempos[p]++;
                    }

                    if (modos[p] == 0)
                    {
                        tiempos[p] = 0;
                        tiempo_p[p] = 0;
                    }

                    if (modos[p] == 2)
                    {
                        tiempo_p[p]++;
                    }
                }

                min++;
                seg = 0;

                if(min>=60)
                {
                    hora++;
                    min = 0;
                }
                if(hora>=24)
                {
                    dia++;
                    hora = 0;
                }


                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                sprintf(fecha, "%d/%d/%d  %d:%d", dia, mes, ano, hora, min);
                Graphics_drawString(&g_sContext, fecha, 20, 15, 5, TRANSPARENT_TEXT);
            }
        }

        switch (estados)
        {

        //Estado de arranque: introducimos la hora y pasamos a seleccionar usuario
        //-------------------------------------------------------------------------------------
        case ARRANQUE:
            //Parpadeo de LED ambar

            if (l >= 50)
            {
                P2OUT ^= (BIT1 | BIT2);
                l = 0;
            }
            //Variable que se pone a 1 al finalizar la introducción de la fecha
            if (fin == 1)
            {
                //Borro mensaje anterior
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                Graphics_drawString(&g_sContext, "Introduzca la", 13, 10, 40, TRANSPARENT_TEXT);
                Graphics_drawString(&g_sContext, "fecha y hora", 13, 10, 60, TRANSPARENT_TEXT);
                //Validación de fecha
                if (( dia > 31) || (mes > 12) || (hora > 23) || (min > 59))
                {
                    fin = 0;
                    UART_SendString("Error, introduzcala de nuevo \r\n");
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                    Graphics_drawString(&g_sContext, "Introduzca la", 13, 10, 40,
                                        TRANSPARENT_TEXT);
                    Graphics_drawString(&g_sContext, "fecha y hora", 13, 10, 60,
                                        TRANSPARENT_TEXT);
                    fin_fecha = 2;
                }
                else
                {
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                    sprintf(fecha, "%d/%d/%d  %d:%d", dia, mes, ano, hora, min);
                    Graphics_drawString(&g_sContext, fecha, 20, 15, 5, TRANSPARENT_TEXT);
                    UART_SendString("Fecha y horas introducidas con exito \r\n");
                    estados = SELEC_USER;
                    fin_fecha = 1;
                }
            }
            else
            {
                if(t>=1000)
                {
                    UART_SendString("Esperando \r\n");
                    t=0;
                }
                break;
            }

            break;
            //-------------------------------------------------------------------------


       //Estado de seleccion de usuario
       //-----------------------------------------------------------------------------------
        case SELEC_USER:

            //Encendemos el LED azul
            P2OUT |= BIT4;
            P2OUT &= ~(BIT1 | BIT2 | BIT3);

            //Pedimos seleccion de usuario
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
            Graphics_drawString(&g_sContext, "Selecccione ", 13, 10, 30, TRANSPARENT_TEXT);
            Graphics_drawString(&g_sContext, "usuario ", 13, 10, 50, TRANSPARENT_TEXT);
            LeeFlash= (char *) 0x1000;
            if (t >= 50) {
                for (i = 0; i <= 5; i++)  usuario_ant[i] = LeeFlash[i+j];
                usuario_ant[7] = '\0';

                j = leer_ADC (3, ejey, j, 800, 223, 90, 0, 6);
                id = leer_ADC(3, ejey, id, 800, 223, 15, 0 , 1);
                // Copiar el nuevo nombre desde la memoria Flash
                for (i = 0; i <= 5; i++)  usuario[i] = LeeFlash[i + j];  // Leer el nuevo nombre
                usuario[7] = '\0';
                // Borrar el nombre anterior (dibujar en negro)
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                Graphics_drawString(&g_sContext, usuario_ant, 6, 35, 100, TRANSPARENT_TEXT);
                // Dibujar el nuevo nombre (en blanco)
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                Graphics_drawString(&g_sContext, usuario, 6, 35, 100, TRANSPARENT_TEXT);
                t = 0;  // Reiniciar el temporizador
            }

            if(!(P2IN&BIT5))
            {
                if(t>=20)
                {
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                    Graphics_drawString(&g_sContext, "Selecccione ", 13, 10, 30,TRANSPARENT_TEXT);
                    Graphics_drawString(&g_sContext, "usuario ", 13, 10, 50,TRANSPARENT_TEXT);
                    fin=2;
                    intentos=3;
                    estados = PIN_US;
                    t=0;
                }
            }
            break;

            //-----------------------------------------------------------------------------------------

        //Estado en el que se introduce el PIN del usuario
        //-----------------------------------------------------------------------------------------
        case PIN_US:

            P2OUT|= (BIT3 | BIT1);

            if(intentos<3 && t>=100)
            {
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                Graphics_drawString(&g_sContext, "ERROR", 13, 30, 30,TRANSPARENT_TEXT);
            }

            if(intentos == 0 && t>=400)
            {
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                Graphics_drawString(&g_sContext, "Usuario", 13, 15, 50,TRANSPARENT_TEXT);
                Graphics_drawString(&g_sContext, "bloqueado", 13, 10, 70,TRANSPARENT_TEXT);
                t=0;
                estados = REINICIO;
            }

            LeeFlash = (char *)0x1060;

            for (i = 0; i < 4; i++) PIN[i] = LeeFlash[i+4*id];

            if(fin==2)
            {
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                Graphics_drawString(&g_sContext, "Elija el PIN", 13, 10, 30, TRANSPARENT_TEXT);
                fin = 3;
            }

            if(fin == 4)
            {
                for (i = 0; i < 4; i++) if (PIN_in[i] == PIN[i]) pin_correcto++;

                for (i = 0; i < 4; i++) PIN_in[i] = '0';

                if (pin_correcto == 4)
                {
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                    Graphics_drawString(&g_sContext, "Elija el PIN", 13, 10, 30, TRANSPARENT_TEXT);
                    Graphics_drawString(&g_sContext, "ERROR", 13, 30, 30,TRANSPARENT_TEXT);
                    Graphics_drawString(&g_sContext, "Introduzca de", 13, 10, 50,TRANSPARENT_TEXT);
                    Graphics_drawString(&g_sContext, "nuevo el PIN", 13, 15, 70,TRANSPARENT_TEXT);

                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);

                    Graphics_drawString(&g_sContext, "PIN correcto ", 13, 10, 30,TRANSPARENT_TEXT);

                    estados = MODO;
                }

                if (pin_correcto != 4)
                {
                    P2OUT &= ~BIT1;
                    P2OUT &= ~BIT4;
                    P2OUT &= ~BIT3;
                    intentos--;
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                    Graphics_drawString(&g_sContext, "Elija el PIN", 13, 10, 30, TRANSPARENT_TEXT);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                    Graphics_drawString(&g_sContext, "ERROR", 13, 30, 30,TRANSPARENT_TEXT);
                    Graphics_drawString(&g_sContext, "Introduzca de", 13, 10, 50,TRANSPARENT_TEXT);
                    Graphics_drawString(&g_sContext, "nuevo el PIN", 13, 15, 70,TRANSPARENT_TEXT);
                    UART_SendString("Introduzca el PIN de nuevo: \r\n");
                    fin=3;
                    t=0;
                    pin_correcto = 0;
                }

                if (intentos == 0)
                {
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                    Graphics_drawString(&g_sContext, "ERROR", 13, 30, 30,TRANSPARENT_TEXT);
                    Graphics_drawString(&g_sContext, "Introduzca de", 13, 10, 50,TRANSPARENT_TEXT);
                    Graphics_drawString(&g_sContext, "nuevo el PIN", 13, 15, 70,TRANSPARENT_TEXT);
                    pin_correcto = 0;

                    if (l >= 50)
                    {
                        P2OUT ^= BIT3;
                        l = 0;
                    }

                    if(id != 0)
                    {
                        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                        sprintf(acceso, "%s ha sido bloqueado, Hora:%d/%d/%d  %d:%d \r\n", usuario, dia, mes, ano, hora, min);
                        Graphics_drawString(&g_sContext, "Usuario", 13, 15, 50,TRANSPARENT_TEXT);
                        Graphics_drawString(&g_sContext, "bloqueado", 13, 10, 70,TRANSPARENT_TEXT);
                        UART_SendString(acceso);

                        modos[id] = 3;

                    }

                    if(id == 0)
                    {
                        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                        Graphics_drawString(&g_sContext, "Bloqueo", 13, 15, 50,TRANSPARENT_TEXT);
                        Graphics_drawString(&g_sContext, "total", 13, 10, 70,TRANSPARENT_TEXT);
                        UART_SendString("Bloqueo total");
                    }
                }
            }

            break;
            //----------------------------------------------------------------------------------------


        //En Modo seleccionamos qué pasará a hacer cada usuario
        //--------------------------------------------------------------------------------------------
        case MODO:
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
            Graphics_drawString(&g_sContext, "PIN correcto ", 13, 10, 30,TRANSPARENT_TEXT);



            //En caso de ser un usuario normal
            if (id != 0)
            {
                P2OUT &= ~(BIT1 | BIT4);
                P2OUT |= (BIT3 | BIT2);

                if (modos[id] != 1)
                {
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                    Graphics_drawString(&g_sContext, "ENTRAR", 13, 25, 55, TRANSPARENT_TEXT);
                    Graphics_fillCircle(&g_sContext, 15, 60, 5);

                    if (!(P2IN & BIT5) && modos[id]==0)
                    {
                        sprintf(acceso, "%s ha entrado, Hora:%d/%d/%d  %d:%d, Intento:%d \r\n", usuario, dia, mes, ano, hora, min, 3-intentos);
                        UART_SendString(acceso);

                        modos[id] = 1;
                        modo_ant[id] = modos[id];
                        estados = REINICIO;
                    }

                    if (!(P2IN & BIT5) && modos[id]==2)
                    {
                        sprintf(acceso, "%s ha entrado, Hora:%d/%d/%d  %d:%d, Tiempo trabajado: %d min, Tiempo en pausa: %d min \r\n", usuario, dia, mes, ano, hora, min, tiempos[id], tiempo_p[id]);
                        UART_SendString(acceso);
                        modos[id] = 1;
                        modo_ant[id] = modos[id];
                        estados = REINICIO;
                    }
                    break;
                }

                if (modos[id] == 1)
                {
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                    Graphics_drawString(&g_sContext, "PAUSA", 13, 25, 40, TRANSPARENT_TEXT);
                    Graphics_drawString(&g_sContext, "SALIR", 13, 25, 70, TRANSPARENT_TEXT);
                    Graphics_fillCircle(&g_sContext, 15, y, 5);

                    if (t >= 50)
                    {
                        y = leer_ADC (3, ejey, y, 800, 223, 75, 45, 30);
                        if (y != ya)
                        {
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                            Graphics_fillCircle(&g_sContext, 15, ya, 5);
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                            Graphics_fillCircle(&g_sContext, 15, y, 5);
                            ya = y;
                        }
                        t = 0;
                    }
                    if (!(P2IN & BIT5))
                    {
                        if (y == 75)
                        {

                            sprintf(acceso, "%s ha salido, Hora:%d/%d/%d  %d:%d, Tiempo trabajado: %d min  \r\n", usuario, dia, mes, ano, hora, min, tiempos[id]);
                            UART_SendString(acceso);
                            modos[id] = 0;
                            modo_ant[id] = modos[id];
                        }
                        if (y == 45)
                        {
                            sprintf(acceso, "%s ha ido a hacer una pausa, Hora:%d/%d/%d  %d:%d \r\n", usuario, dia, mes, ano, hora, min);
                            UART_SendString(acceso);
                            modos[id] = 2;
                            modo_ant[id] = modos[id];
                        }
                        estados = REINICIO;
                    }
                    break;
                }
            }

            //Si es el admin
                 if(id==0)
                {
                     P2OUT |= (BIT3 | BIT2 | BIT4 | BIT1);

                     if (l >= 100)
                     {
                         P2OUT ^= (BIT3 | BIT2 | BIT4 | BIT1);
                         l = 0;
                     }

                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                    Graphics_drawString(&g_sContext, "DESBLOQUEO", 13, 25, 40, TRANSPARENT_TEXT);
                    Graphics_drawString(&g_sContext, "INFORME", 13, 25, 70, TRANSPARENT_TEXT);
                    Graphics_fillCircle(&g_sContext, 15, y, 5);

                    if (t >= 50)
                    {
                        y = leer_ADC (3, ejey, y, 800, 223, 75, 45, 30);
                        if (y != ya)
                        {
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                            Graphics_fillCircle(&g_sContext, 15, ya, 5);
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                            Graphics_fillCircle(&g_sContext, 15, y, 5);
                            ya = y;
                        }
                        t = 0;
                    }

                    if (!(P2IN&BIT5))
                    {
                        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                        Graphics_drawString(&g_sContext, "DESBLOQUEO", 13, 25, 40, TRANSPARENT_TEXT);
                        Graphics_drawString(&g_sContext, "INFORME", 13, 25, 70, TRANSPARENT_TEXT);
                        Graphics_fillCircle(&g_sContext, 15, y, 5);

                        if (y == 75)
                        {
                            UART_SendString( "El manager ha pedido un informe\r\n");
                            estados = INFORME;
                        }

                        if (y == 45)
                        {

                                if (modos[i] == 3)
                                {

                                    UART_SendString("Los usuarios han sido desbloqueados\r\n");
                                }


                            for(i = 0; i < 16; i ++) modos[i] = modo_ant[i];
                            estados = REINICIO;
                        }
                    }
                }

            break;
            //-------------------------------------------------------------------------

            //Informe: se selecciona el usuario y se muestra el estado y tiempo trabajado por pantalla
            //-------------------------------------------------------------------------------
        case INFORME:
            LeeFlash= (char *) 0x1000;
            //Pedimos seleccion de usuario
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
            Graphics_drawString(&g_sContext, "Selecccione ", 13, 10, 30,TRANSPARENT_TEXT);
            Graphics_drawString(&g_sContext, "usuario para ", 13, 10, 50,TRANSPARENT_TEXT);
            Graphics_drawString(&g_sContext, "informe ", 13, 10, 70, TRANSPARENT_TEXT);


            if (t >= 50) {
                for (i = 0; i <= 5; i++)  usuario_ant[i] = LeeFlash[i+j];
                usuario_ant[7] = '\0';

                j = leer_ADC (3, ejey, j, 800, 223, 90, 0, 6);
                id = leer_ADC(3, ejey, id, 800, 223, 15, 0 , 1);
                // Copiar el nuevo nombre desde la memoria Flash
                for (i = 0; i <= 5; i++)  usuario[i] = LeeFlash[i + j];  // Leer el nuevo nombre
                usuario[7] = '\0';
                // Borrar el nombre anterior (dibujar en negro)
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                Graphics_drawString(&g_sContext, usuario_ant, 6, 35, 100, TRANSPARENT_TEXT);
                // Dibujar el nuevo nombre (en blanco)
                Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                Graphics_drawString(&g_sContext, usuario, 6, 35, 100, TRANSPARENT_TEXT);
                t = 0;  // Reiniciar el temporizador
            }

            if(!(P2IN&BIT5))
            {
                if(t>=20)
                {
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                    Graphics_drawString(&g_sContext, "Selecccione ", 13, 10, 30,TRANSPARENT_TEXT);
                    Graphics_drawString(&g_sContext, "usuario para ", 13, 10, 50,TRANSPARENT_TEXT);
                    Graphics_drawString(&g_sContext, "informe ", 13, 10, 70, TRANSPARENT_TEXT);

                    sprintf(acceso, "%s ,Estado:%d, Tiempo trabajado: %d min, Tiempo en pausa: %d  \r\n", usuario, modos[id], tiempos[id], tiempo_p[id]);
                    UART_SendString(acceso);
                    estados = REINICIO;
                    t = 0;
                }
            }

            break;
            //-------------------------------------------------------------------------


        //Volvemos a seleccion de usuario
        //-----------------------------------------------------------------------------
        case REINICIO:
            t=0;
            id=0;
            j=0;
            pin_correcto=0;
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
            Graphics_drawString(&g_sContext, "ENTRAR", 13, 25, 55,TRANSPARENT_TEXT);
            Graphics_drawString(&g_sContext, usuario, 6, 35, 100, TRANSPARENT_TEXT);
            Graphics_drawString(&g_sContext, "SALIR", 13, 25, 70,TRANSPARENT_TEXT);
            Graphics_drawString(&g_sContext, "PAUSA", 13, 25, 40,TRANSPARENT_TEXT);
            Graphics_fillCircle(&g_sContext, 15, y, 5);
            Graphics_fillCircle(&g_sContext, 15, 60, 5);
            estados=SELEC_USER;
        }
        //---------------------------------------------------------------------
    }
}

/*---- Interrupciones ----*/
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR_HOOK(void)
{

    static int index = 0;      //  Indice para controlar la posicion en el buffer
    static int num_dato = 0;

    if (num_dato == 0)
    {
        buffer[index] = UCA0RXBUF;       // Lee el car cter recibido
        index++;
        if (index >= 2)
        {
            buffer[2] = '\0';            // Termina la cadena con '\0'
            dia = atoi(buffer); // Convierte la cadena a un entero y almacena en dia
            num_dato = 1;
            index = 0;
        }

    }

    else if (num_dato == 1)
    {
        buffer[index] = UCA0RXBUF;       // Lee el car cter recibido
        index++;
        if (index >= 2)
        {
            buffer[2] = '\0';            // Termina la cadena con '\0'
            mes = atoi(buffer); // Convierte la cadena a un entero y almacena en dia
            num_dato = 2;
            index = 0;
        }

    }

    else if (num_dato == 2)
    {
        buffer[index] = UCA0RXBUF;       // Lee el car cter recibido
        index++;
        if (index >= 2)
        {
            buffer[2] = '\0';            // Termina la cadena con '\0'
            ano = atoi(buffer); // Convierte la cadena a un entero y almacena en dia
            num_dato = 3;
            index = 0;

        }
    }

    else if (num_dato == 3)
    {
        buffer[index] = UCA0RXBUF;       // Lee el car cter recibido
        index++;
        if (index >= 2)
        {
            buffer[2] = '\0';            // Termina la cadena con '\0'
            hora = atoi(buffer); // Convierte la cadena a un entero y almacena en dia
            num_dato = 4;
            index = 0;

        }
    }

    else if (num_dato == 4)
    {
        buffer[index] = UCA0RXBUF;       // Lee el car cter recibido
        index++;
        if (index >= 2)
        {
            buffer[2] = '\0';            // Termina la cadena con '\0'
            min = atoi(buffer); // Convierte la cadena a un entero y almacena en min
            num_dato = 5;
            index = 0;
            fin = 1;
        }
    }

    if(num_dato == 5 && fin_fecha == 2)
    {
        fin_fecha = 0;
        num_dato = 0;
    }

    if (fin == 3)
    {
        PIN_in[index] = UCA0RXBUF;       // Lee el car cter recibido
        index++;
        if (index >= 4)
        {
            PIN_in[4] = '\0';
            fin = 4;
            index = 0;
        }
    }
    LPM0_EXIT;
}

#pragma vector=ADC10_VECTOR
__interrupt void ConvertidorAD(void)
{
    LPM0_EXIT;
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void Interrupcion_T1(void)
{

    l++; // Usada para el parpadeo de los LEDs
    t++; // Usada para coordinar las acciones en el programa
    seg++; //Usada para actualizar el tiempo del sistema
    LPM0_EXIT;

}

/*---- Funciones ----*/

int leer_ADC (int canal, int eje, int valor, int maxeje, int mineje, int maxvalor, int minvalor, int aumento)
{
    eje = lee_ch(canal);
    if (eje <= maxeje) valor += aumento;
    if (eje >= mineje) valor -= aumento;
    if (valor > maxvalor) valor = minvalor;
    if (valor < minvalor) valor = maxvalor;
    return valor;
}

void UART_SendString(const char *str)
{
    while (*str != '\0')
    {
        while (!(IFG2 & UCA0TXIFG))
            ; // Espera a que el buffer est  listo
        UCA0TXBUF = *str;            // Env a el car cter actual
        str++;                       // Avanza al siguiente car cter
    }
}

    //funcion para activar el convertidor A->D
void inicia_ADC(char canales)
{
    ADC10CTL0 &= ~ENC;      //deshabilita ADC
    ADC10CTL0 = ADC10ON | ADC10SHT_3 | SREF_0 | ADC10IE; //enciende ADC, S/H lento, REF:VCC, con INT
    ADC10CTL1 = CONSEQ_0 | ADC10SSEL_0 | ADC10DIV_0 | SHS_0 | INCH_0;
    //Modo simple, reloj ADC, sin subdivision, Disparo soft, Canal 0
    ADC10AE0 = canales; //habilita los canales indicados
    ADC10CTL0 |= ENC; //Habilita el ADC
}

    int lee_ch(char canal)
{
    ADC10CTL0 &= ~ENC;                  //deshabilita el ADC
    ADC10CTL1 &= (0x0fff);                //Borra canal anterior
    ADC10CTL1 |= canal << 12;               //selecciona nuevo canal
    ADC10CTL0 |= ENC;                    //Habilita el ADC
    ADC10CTL0 |= ADC10SC;                 //Empieza la conversi n
    LPM0;                               //Espera fin en modo LPM0
    return (ADC10MEM);                   //Devuelve valor leido
}


