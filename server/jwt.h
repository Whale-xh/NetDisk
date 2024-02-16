#ifndef __JWT_H__
#define __JWT_H__

#include <func.h>
#include "l8w8jwt/encode.h"
#include "l8w8jwt/decode.h"

/*jwt.c*/
void createToken(char *username, char *token);
int verify(char *username, char *token);


#endif