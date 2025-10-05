// Runtime library
#include <stdio.h>
#include <string.h>

extern "C" void writeInteger(int x) {
    printf("%d", x);
}

extern "C" void writeString(const char *s) {
    printf("%s", s);
}

extern "C" void writeChar(char c) {
    printf("%c",c);
}

extern "C" void writeByte(char c) {
    printf("%d",c);
}
 
extern "C" int readInteger() {
    int x;
    scanf("%d",&x);
    while (getchar()!='\n');
    return x;
}

extern "C" void readString(int n, char* str) {
    char c;
    int i;
    for (i=0; i<n-1; i++) {
        scanf("%c",&c);
        if (c!='\n') str[i] = c;
        else break;
    }
    str[i]='\0';
}

extern "C" char readChar() {
    char c;
    scanf("%c",&c);
    while (getchar()!='\n');
    return c;
}

extern "C" char readByte() {
    char c;
    scanf("%hhd", &c);  // %hhd matches a char*
    while (getchar()!='\n');
    return c;
}

extern "C" int extend(char c) {
    return (int)c;
}

extern "C" char shrink(int n) {
    return (char)n;
}
