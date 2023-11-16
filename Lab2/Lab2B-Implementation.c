
#include <stdint.h>
#include <stdio.h>

//radix 8
void Bits2OctalString(uint8_t bits, char string[]){
    // printf("Bit Parameter: %u",bits);


    int octal[5]; 
    int i = 0;

    // convert bits var to octal
    while (bits > 0) {
        octal[i] = bits % 8;
        bits /= 8;
        i++;
    }

    // reverse order and store in the string var
    int length = i;
    for (i = 0; i < length; i++) {
        string[i] = octal[length - i - 1] + '0'; // Convert to character
    }

    string[length] = '\0'; // null the string
    
    //printf("\nOctal string: %s", string);
}

//radix 10
void Bits2UnsignedString(uint8_t bits, char string[]){

    int i = 0;

    // convert each digit to a char and store in the string
    do {
        string[i++] = (bits % 10) + '0';
        bits /= 10;
    } while (bits > 0);

    string[i] = '\0';

    // reverses the string
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = string[start];
        string[start] = string[end];
        string[end] = temp;
        start++;
        end--;
    }
    
    //printf("\nUnsigned string: %s", string);


}


//radix 16
void Bits2HexString(uint8_t bits, char string[]){
    // convert to char and store in the string
    static const char hex[] = "0123456789ABCDEF" ;

    int i = 0;

    //use the parameter of the hex string to convert to hex
    // note : only two digits bc decimal maxes out at 255
    int dig_1 = bits % 16;
    string[0] = hex[dig_1];

    int dig_2 = bits / 16;
    string[1] = hex[dig_2];
    
    // reverses order
    int start = 0;
    int end = 1;
    while (start < end) {
        char temp = string[start];
        string[start] = string[end];
        string[end] = temp;
        start++;
        end--;
    }
    string[2] = '\0';

    //printf("\nHex string: %s", string);
}


void Bits2TwosCompString(uint8_t bits, char string[]){
        if ((bits & 0b10000000) != 0)
        {// if negative
            string[0] = '-';
            bits = ~bits + 1 ; // flips bits and adds one
        }
        else string[0] = '+';  // if positive
        Bits2UnsignedString(bits, &string[1]);
        //printf("\n Twos Comp string: %s", string);

}

void Bits2SignMagString(uint8_t bits, char string[]){
    // assigning sign based on the msb of bits
        char sign = (bits & (1<<7)) != 0 ? '-' : '+';  

        unsigned mag = bits & ~(1<<7); // shift left over by 7, inverts, and assigns value 
        string[0] = sign ; 
        Bits2UnsignedString(mag, &string[1]);
        //printf("\n Sign mag string: %s", string);

}
