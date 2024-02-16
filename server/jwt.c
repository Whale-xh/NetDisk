#include "l8w8jwt/encode.h"
#include "l8w8jwt/decode.h"
#include <func.h>

void createToken(char *username, char *token)
{
    char *jwt;
    size_t jwt_length;

    struct l8w8jwt_encoding_params params;
    l8w8jwt_encoding_params_init(&params);

    params.alg = L8W8JWT_ALG_HS512;

    params.sub = "myNetdisk";
    params.iss = "whale";
    params.aud = "whale99";

    params.iat = 0;
    params.exp = 0x7fffffff; /* Set to expire after 10 minutes (600 seconds). */

    params.secret_key = (unsigned char *)username;
    params.secret_key_length = strlen(params.secret_key);
    printf("len     =%ld\n", params.secret_key_length);
    params.out = &jwt;
    params.out_length = &jwt_length;

    int r = l8w8jwt_encode(&params);
    if (r == L8W8JWT_SUCCESS)
    {
        strcpy(token, jwt);
    }
    printf("\n l8w8jwt example HS512 token: %s \n", r == L8W8JWT_SUCCESS ? jwt : " (encoding failure) ");

    /* Always free the output jwt string! */
    l8w8jwt_free(jwt);
}

int verify(char *username, char *token)
{
    struct l8w8jwt_decoding_params params;
    l8w8jwt_decoding_params_init(&params);

    params.alg = L8W8JWT_ALG_HS512;

    params.jwt = (char *)token;
    params.jwt_length = strlen(token);

    params.verification_key = (unsigned char *)username;
    params.verification_key_length = strlen(username);

    /*
     * Not providing params.validate_iss_length makes it use strlen()
     * Only do this when using properly NUL-terminated C-strings!
     */
    params.validate_iss = "whale";
    params.validate_sub = "myNetdisk";

    /* Expiration validation set to false here only because the above example token is already expired! */
    params.validate_exp = 0;
    params.exp_tolerance_seconds = 60;

    params.validate_iat = 0;
    params.iat_tolerance_seconds = 60;

    enum l8w8jwt_validation_result validation_result;

    int decode_result = l8w8jwt_decode(&params, &validation_result, NULL, NULL);

    if (decode_result == L8W8JWT_SUCCESS && validation_result == L8W8JWT_VALID)
    {
        printf("\n Example HS512 token validation successful! \n");
        return 1;
    }
    else
    {
        printf("\n Example HS512 token validation failed! \n");
        return 0;
    }
    /*
     * decode_result describes whether decoding/parsing the token succeeded or failed;
     * the output l8w8jwt_validation_result variable contains actual information about
     * JWT signature verification status and claims validation (e.g. expiration check).
     *
     * If you need the claims, pass an (ideally stack pre-allocated) array of struct l8w8jwt_claim
     * instead of NULL,NULL into the corresponding l8w8jwt_decode() function parameters.
     * If that array is heap-allocated, remember to free it yourself!
     */
}