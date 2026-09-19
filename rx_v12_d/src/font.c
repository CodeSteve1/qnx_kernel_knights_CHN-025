#include "font.h"

void get_font_bits(char c, uint8_t bits[5]) {
    if (c >= 'a' && c <= 'z') c -= 32;
    uint8_t b[5] = {0};
    switch(c) {
        case 'A': b[0]=2; b[1]=5; b[2]=7; b[3]=5; b[4]=5; break;
        case 'B': b[0]=6; b[1]=5; b[2]=6; b[3]=5; b[4]=6; break;
        case 'C': b[0]=3; b[1]=4; b[2]=4; b[3]=4; b[4]=3; break;
        case 'D': b[0]=6; b[1]=5; b[2]=5; b[3]=5; b[4]=6; break;
        case 'E': b[0]=7; b[1]=4; b[2]=6; b[3]=4; b[4]=7; break;
        case 'F': b[0]=7; b[1]=4; b[2]=6; b[3]=4; b[4]=4; break;
        case 'G': b[0]=3; b[1]=4; b[2]=5; b[3]=5; b[4]=3; break;
        case 'H': b[0]=5; b[1]=5; b[2]=7; b[3]=5; b[4]=5; break;
        case 'I': b[0]=7; b[1]=2; b[2]=2; b[3]=2; b[4]=7; break;
        case 'J': b[0]=1; b[1]=1; b[2]=1; b[3]=5; b[4]=2; break;
        case 'K': b[0]=5; b[1]=5; b[2]=6; b[3]=5; b[4]=5; break;
        case 'L': b[0]=4; b[1]=4; b[2]=4; b[3]=4; b[4]=7; break;
        case 'M': b[0]=5; b[1]=7; b[2]=5; b[3]=5; b[4]=5; break;
        case 'N': b[0]=5; b[1]=7; b[2]=7; b[3]=5; b[4]=5; break;
        case 'O': b[0]=2; b[1]=5; b[2]=5; b[3]=5; b[4]=2; break;
        case 'P': b[0]=6; b[1]=5; b[2]=6; b[3]=4; b[4]=4; break;
        case 'Q': b[0]=2; b[1]=5; b[2]=5; b[3]=6; b[4]=3; break;
        case 'R': b[0]=6; b[1]=5; b[2]=6; b[3]=5; b[4]=5; break;
        case 'S': b[0]=3; b[1]=4; b[2]=2; b[3]=1; b[4]=6; break;
        case 'T': b[0]=7; b[1]=2; b[2]=2; b[3]=2; b[4]=2; break;
        case 'U': b[0]=5; b[1]=5; b[2]=5; b[3]=5; b[4]=7; break;
        case 'V': b[0]=5; b[1]=5; b[2]=5; b[3]=2; b[4]=2; break;
        case 'W': b[0]=5; b[1]=5; b[2]=5; b[3]=7; b[4]=5; break;
        case 'X': b[0]=5; b[1]=5; b[2]=2; b[3]=5; b[4]=5; break;
        case 'Y': b[0]=5; b[1]=5; b[2]=2; b[3]=2; b[4]=2; break;
        case 'Z': b[0]=7; b[1]=1; b[2]=2; b[3]=4; b[4]=7; break;
        case '0': b[0]=7; b[1]=5; b[2]=5; b[3]=5; b[4]=7; break;
        case '1': b[0]=2; b[1]=6; b[2]=2; b[3]=2; b[4]=7; break;
        case '2': b[0]=7; b[1]=1; b[2]=7; b[3]=4; b[4]=7; break;
        case '3': b[0]=7; b[1]=1; b[2]=7; b[3]=1; b[4]=7; break;
        case '4': b[0]=5; b[1]=5; b[2]=7; b[3]=1; b[4]=1; break;
        case '5': b[0]=7; b[1]=4; b[2]=7; b[3]=1; b[4]=7; break;
        case '6': b[0]=7; b[1]=4; b[2]=7; b[3]=5; b[4]=7; break;
        case '7': b[0]=7; b[1]=1; b[2]=1; b[3]=1; b[4]=1; break;
        case '8': b[0]=7; b[1]=5; b[2]=7; b[3]=5; b[4]=7; break;
        case '9': b[0]=7; b[1]=5; b[2]=7; b[3]=1; b[4]=7; break;
        case ':': b[0]=0; b[1]=2; b[2]=0; b[3]=2; b[4]=0; break;
        case '%': b[0]=5; b[1]=1; b[2]=2; b[3]=4; b[4]=5; break;
        case '/': b[0]=1; b[1]=1; b[2]=2; b[3]=4; b[4]=4; break;
        case '{': b[0]=3; b[1]=4; b[2]=6; b[3]=4; b[4]=3; break;
        case '}': b[0]=6; b[1]=1; b[2]=3; b[3]=1; b[4]=6; break;
        case '[': b[0]=6; b[1]=4; b[2]=4; b[3]=4; b[4]=6; break;
        case ']': b[0]=3; b[1]=1; b[2]=1; b[3]=1; b[4]=3; break;
        case '(': b[0]=2; b[1]=4; b[2]=4; b[3]=4; b[4]=2; break;
        case ')': b[0]=2; b[1]=1; b[2]=1; b[3]=1; b[4]=2; break;
        case '"': b[0]=5; b[1]=5; b[2]=0; b[3]=0; b[4]=0; break;
        case ',': b[0]=0; b[1]=0; b[2]=0; b[3]=2; b[4]=4; break;
        case '.': b[0]=0; b[1]=0; b[2]=0; b[3]=0; b[4]=2; break;
        case '-': b[0]=0; b[1]=0; b[2]=7; b[3]=0; b[4]=0; break;
        case '|': b[0]=2; b[1]=2; b[2]=2; b[3]=2; b[4]=2; break;
        case '\\':b[0]=4; b[1]=4; b[2]=2; b[3]=1; b[4]=1; break;
        case ' ': default: break;
    }
    for(int i=0; i<5; i++) bits[i] = b[i];
}
