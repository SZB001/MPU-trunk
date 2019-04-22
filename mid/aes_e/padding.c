#include <stdio.h>
#include "padding.h"


void add_pkcs_padding( unsigned char *output, size_t output_len,
        size_t data_len )
{
    size_t padding_len = output_len - data_len;
    unsigned char i;
    //printf( "\n [add_pkcs_padding]padding len:input_len:%u, %u \n", padding_len,data_len); 

    for( i = 0; i < padding_len; i++ )
        output[data_len + i] = (unsigned char) padding_len;
}

int get_pkcs_padding( unsigned char *input, size_t input_len,
        size_t *data_len )
{
    size_t i, pad_idx;
    unsigned char padding_len, bad = 0;

    if( NULL == input || NULL == data_len )
        return( ERR_PADDING_BAD_INPUT_DATA );

    //printf( "\n [get_pkcs_padding]padding len:input_len:%u, %u \n", input_len,input[input_len - 1]); 
    padding_len = input[input_len - 1];
    *data_len = input_len - padding_len;

    /* Avoid logical || since it results in a branch */
    bad |= padding_len > input_len;
    bad |= padding_len == 0;

    /* The number of bytes checked must be independent of padding_len,
     * so pick input_len, which is usually 8 or 16 (one block) */
    pad_idx = input_len - padding_len;
    for( i = 0; i < input_len; i++ )
        bad |= ( input[i] ^ padding_len ) * ( i >= pad_idx );

    return( ERR_PADDING_INVALID_PADDING * ( bad != 0 ) );
}

