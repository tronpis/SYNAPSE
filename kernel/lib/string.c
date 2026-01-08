/* SYNAPSE SO - String Library */
/* Licensed under GPLv3 */

/* Get string length */
int strlen(const char* str) {
    int len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

/* Compare two strings */
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

/* Copy string */
char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}
