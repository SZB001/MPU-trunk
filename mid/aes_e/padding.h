#ifndef MBEDTLS_PADDING_H
#define MBEDTLS_PADDING_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#define ERR_PADDING_FEATURE_UNAVAILABLE            -0x6080  /**< The selected feature is not available. */
#define ERR_PADDING_BAD_INPUT_DATA                 -0x6100  /**< Bad input parameters to function. */
#define ERR_PADDING_ALLOC_FAILED                   -0x6180  /**< Failed to allocate memory. */
#define ERR_PADDING_INVALID_PADDING                -0x6200  /**< Input data contains invalid padding and is rejected. */
#define ERR_PADDING_FULL_BLOCK_EXPECTED            -0x6280  /**< Decryption of block requires a full block. */
#define ERR_PADDING_AUTH_FAILED                    -0x6300  /**< Authentication failed (for AEAD modes). */

/*
 * PKCS7 (and PKCS5) padding: fill with ll bytes, with ll = padding_len
 */
 void add_pkcs_padding( unsigned char *output, size_t output_len,
        size_t data_len );

int get_pkcs_padding( unsigned char *input, size_t input_len,
        size_t *data_len );
	


#ifdef __cplusplus
}
#endif

#endif

