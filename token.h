#ifndef NOLLI_TOKEN_H
#define NOLLI_TOKEN_H

typedef union token {
    bool b;
    char c;
    long i;
    double r;
    char* s;
} token_t;

#endif /* NOLLI_TOKEN_H */
