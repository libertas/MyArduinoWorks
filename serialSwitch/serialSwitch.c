// Running on an Atmega 16a

#include <avr/io.h>
#include <util/delay.h>

void writeEEPROM(unsigned int addr, unsigned char data)
{
    while(EECR & (1 << EEWE));
    EEAR = addr;
    EEDR = data;
    EECR |= 1 << EEMWE;
    EECR |= 1 << EEWE;
}

unsigned char readEEPROM(unsigned int addr)
{
    while(EECR & (1 << EEWE));
    EEAR = addr;
    EECR |= 1 << EERE;
    return EEDR;
}

void sendUSART(unsigned char data)
{
    while(!(UCSRA & (1 << UDRE)));
    UDR = data;
}

unsigned char receiveUSART()
{
    while(!(UCSRA & (1 << RXC)));
    return UDR;
}

void print(char *s)
{
    while(*s)
    {
        sendUSART(*s);
        s++;
    }
}

void initUSART()
{
    UBRRH = 0;
    UBRRL = 103; // 9600Hz on 16MHz F_CPU
    UCSRB = (1 << RXEN) | (1 << TXEN);
    UCSRC = (1 << URSEL) | (3 << UCSZ0);
}

int main()
{
    unsigned char codeUSART[3];
    unsigned char i, j, addr;

    DDRA = 0xff;
    DDRC = 0xff;
    PORTA = 0x00;
    PORTC = 0x00;

    for(i = 'a'; i < 'a' + 8; i++)
    {
        PORTA = PORTA >> 1;
        PORTA |= ((readEEPROM(i) - '0') & 0x01) << 7;
    }

    for(i = 'a' + 8; i < 'a' + 16; i++)
    {
        PORTC = PORTC >> 1;
        PORTC |= ((readEEPROM(i) - '0') & 0x01) << 7;
    }

    initUSART();

    while(1)
    {
        for(i = 0; i < 3; i++)
        {
            codeUSART[i] = receiveUSART();
            if(codeUSART[i] == 19 || codeUSART[i] == '\n')
            {
                for(j = 0; j < 3; j++)
                    codeUSART[i] = 0x00;
                break;
            }
        }

        if(codeUSART[0])
        {
            print("\nReceived:\n");
            for(i = 0; i < 3; i++)
                sendUSART(codeUSART[i]);
            print("\n");
        }

        addr = codeUSART[1] - '0';
        switch(codeUSART[0])
        {
            case 'A':  // control port a
                if(codeUSART[2] - '0')
                {
                    writeEEPROM('a' + addr, '1');
                    PORTA |= 1 << addr;
                }
                else
                {
                    writeEEPROM('a' + addr, '0');
                    PORTA &= ~(1 << addr);
                }
                break;
            case 'C':  // control port c
                if(codeUSART[2] - '0')
                {
                    writeEEPROM('a' + 8 + addr, '1');
                    PORTC |= 1 << addr;
                }
                else
                {
                    writeEEPROM('a' + 8 + addr, '0');
                    PORTC &= ~(1 << addr);
                }
                break;
            case 'E':  // set eeprom data
                if(codeUSART[2] == 'i')
                    for(i = 0; i < 255; i++)
                        writeEEPROM(i, codeUSART[1]);
                else if(codeUSART[2] == 'f')
                    for(i = 0; i < 255; i++)
                        writeEEPROM(i, 0xff);
                else if(codeUSART[2] == 'r')
                    PORTA = ~readEEPROM(codeUSART[1]);
                else
                    writeEEPROM(codeUSART[1], codeUSART[2]);
                break;
            default:
                break;
        }
    }
    return 0;
}

