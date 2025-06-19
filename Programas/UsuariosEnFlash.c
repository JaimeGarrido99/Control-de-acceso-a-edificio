#include <msp430.h>

#define MAX_USERS 16
#define NAME_LENGTH 6
#define PASSWORD_LENGTH 4

// IDs de usuarios (0 a 15)
//unsigned char user_ids[MAX_USERS] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
// Nombres de los usuarios
char user_names[MAX_USERS][NAME_LENGTH] = { "0JLKOP", "1RZQMA", "2LVCOW", "3PZMRT", "4WNVKS", "5KJRQE", "6VCNZR", "7QXJLO", "8QSVTM", "9BNQWR", "10LJPT", "11QMVS", "12ZWPL", "13VTNM", "14WZXC", "15KJDA" };
// Contraseñas de los usuarios (4 caracteres por usuario)
char user_passwords[MAX_USERS][PASSWORD_LENGTH] = { "4821", "7194", "5638", "2347", "8412", "1926", "5703", "8039","1145", "7620", "2184", "9072", "3371", "6249", "1185", "4269" };
void conf_reloj(char VEL);
void guarda_flash(char *dato, char *address, int length);
void borrar_flash(char *address1, char *address2);
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;
    conf_reloj(1);
    /*------ Configuración de la USCI-A para modo SPI:----------*/
    UCB0CTL1 |= UCSWRST; // Resetea la USCI
    UCB0CTL0 = UCCKPH | UCMSB | UCMST | UCMODE_0 | UCSYNC;
    UCB0CTL1 = UCSSEL_2 | UCSWRST;
    UCB0BR0 = 2;
    UCB0CTL1 &= ~UCSWRST;
    //timer 1s
    TA0CCTL0 = CM_0 | CCIS_0 | OUTMOD_0 | CCIE;
    TA0CTL = TASSEL_1 | ID_0 | MC_1;
    TA0CCR0 = 11999;

    __bis_SR_register(GIE);
    borrar_flash((char *) 0x1000, (char *)0x10A0);
    //guarda_flash((char *)user_ids, (char*) 0x1000, MAX_USERS);
    guarda_flash((char *)user_names, (char*) 0x1000, MAX_USERS * NAME_LENGTH);
    guarda_flash((char *)user_passwords, (char*) 0x1060, MAX_USERS * PASSWORD_LENGTH);
    while (1)
    {
        __bis_SR_register(LPM3_bits);

    }
}
void borrar_flash(char *address1, char *address2){
    for (; address1 < address2; address1++){
        FCTL1 = FWKEY + ERASE;
        FCTL3 = FWKEY;
        *(address1) = 0xFF;
        FCTL1 = FWKEY;
        FCTL3 = FWKEY + LOCK;
    }
}
void guarda_flash(char *dato, char *address, int length)
{
    unsigned int i;
    for (i = 0; i < length; i++)
    {
        if (*(address+i) == 0xFF)
        {
            FCTL3 = FWKEY;
            FCTL1 = FWKEY + WRT;
            *(address+i) = *(dato+i);
            FCTL1 = FWKEY;
            FCTL3 = FWKEY + LOCK;
        }
        else
        {
            FCTL1 = FWKEY + ERASE;
            FCTL3 = FWKEY;
            *(address+i) = 0xFF;
            FCTL1 = FWKEY + WRT;
            *(address+i) = *(dato+i);
            FCTL1 = FWKEY;
            FCTL3 = FWKEY + LOCK;
        }
    }
}
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR_HOOK(void){__bic_SR_register_on_exit(LPM3_bits);}

void conf_reloj(char VEL){
    BCSCTL2 = SELM_0 | DIVM_0 | DIVS_0;
    switch (VEL){
    case 1:
        if (CALBC1_1MHZ != 0xFF){
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_1MHZ; /* Set DCO to 1MHz */
            DCOCTL = CALDCO_1MHZ;}
        break;
    case 8:

        if (CALBC1_8MHZ != 0xFF){
            __delay_cycles(100000);
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_8MHZ; /* Set DCO to 8MHz */
            DCOCTL = CALDCO_8MHZ;}
        break;
    case 12:
        if (CALBC1_12MHZ != 0xFF){
            __delay_cycles(100000);
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_12MHZ; /* Set DCO to 12MHz */
            DCOCTL = CALDCO_12MHZ;}
        break;
    case 16:
        if (CALBC1_16MHZ != 0xFF){
            __delay_cycles(100000);
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_16MHZ; /* Set DCO to 16MHz */
            DCOCTL = CALDCO_16MHZ;}
        break;
    default:
        if (CALBC1_1MHZ != 0xFF){
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_1MHZ; /* Set DCO to 1MHz */
            DCOCTL = CALDCO_1MHZ;}
        break;}
    BCSCTL1 |= XT2OFF | DIVA_0;
    BCSCTL3 = XT2S_0 | LFXT1S_2 | XCAP_1;}
