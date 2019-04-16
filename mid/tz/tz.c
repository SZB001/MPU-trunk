#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "ql_tzone.h"

#if 0
static unsigned char g_AESKey[] =
{
    0x00, 0xd5, 0x6e, 0x89, 0x6a, 0xdb, 0x76, 0xe5, 0xe4, 0xf6, 0x2d, 0xdd, 0xa3, 0xaf, 0x1b, 0x15,
    0x00, 0xd5, 0x6e, 0x89, 0x6a, 0xdb, 0x76, 0xe5, 0xe4, 0xf6, 0x2d, 0xdd, 0xa3, 0xaf, 0x1b, 0x15
}; // any data, must be 32 bytes

//openssl generate next modulus ,private Expoent, and private Exponent
static unsigned char g_modulus[] =
{
    0x00, 0xd5, 0x6e, 0x89, 0x6a, 0xdb, 0x76, 0xe5, 0xe4, 0xf6, 0x2d, 0xdd, 0xa3, 0xaf, 0x1b, 0x15,
    0xfe, 0x18, 0xff, 0x58, 0xb4, 0x14, 0xc2, 0xdd, 0x05, 0xe6, 0xa3, 0x74, 0x51, 0xee, 0x46, 0x1e,
    0xaf, 0xcf, 0x0b, 0x16, 0x02, 0xbb, 0xa5, 0x14, 0xcf, 0x89, 0x6d, 0x01, 0xd1, 0x07, 0x06, 0x72,
    0x53, 0x62, 0x89, 0xf8, 0x63, 0x85, 0xa7, 0xe9, 0xfe, 0xfa, 0x64, 0xe6, 0xc1, 0x10, 0x03, 0xeb,
    0xa8, 0xf2, 0xf3, 0xc2, 0xde, 0xa3, 0x9e, 0x2c, 0xae, 0x84, 0x74, 0x3b, 0x02, 0x31, 0xb8, 0x99,
    0x28, 0x3e, 0x46, 0x91, 0xc7, 0x15, 0xc8, 0x30, 0xab, 0x63, 0x26, 0x30, 0xc6, 0x76, 0x77, 0x22,
    0xc3, 0x43, 0xc3, 0x45, 0xbf, 0xb7, 0x28, 0xa4, 0x2b, 0x63, 0xb4, 0xa2, 0x4e, 0x06, 0xcf, 0xfc,
    0x0b, 0x34, 0x12, 0xc9, 0xbd, 0x0d, 0x3f, 0x48, 0x44, 0x61, 0xb6, 0xc7, 0x17, 0x52, 0x66, 0xcf,
    0xa7, 0x86, 0x92, 0xe1, 0xf0, 0x4c, 0x94, 0x77, 0xa8, 0x61, 0x40, 0x00, 0x9d, 0x9c, 0xd7, 0x99,
    0xf1, 0xdb, 0x43, 0x1a, 0xaf, 0x39, 0xc6, 0x3e, 0x93, 0xdc, 0x19, 0x28, 0x38, 0x91, 0x86, 0x85,
    0x75, 0x99, 0xd3, 0xdc, 0x9d, 0xef, 0x57, 0x68, 0x29, 0x03, 0xde, 0x60, 0x36, 0xc9, 0x0a, 0x36,
    0xbd, 0x8f, 0x4f, 0x90, 0x21, 0x91, 0x8b, 0x8f, 0x38, 0xee, 0xbe, 0xba, 0x18, 0x19, 0xf4, 0x21,
    0xc2, 0x8d, 0x46, 0x27, 0x18, 0x75, 0x13, 0x44, 0x95, 0x75, 0x1b, 0x8d, 0x78, 0xb6, 0x01, 0x80,
    0x45, 0xd5, 0x50, 0x0c, 0x59, 0xd8, 0xf0, 0x6d, 0x65, 0xaa, 0xe2, 0x36, 0x59, 0x8c, 0x64, 0x37,
    0x92, 0xdb, 0x65, 0x8c, 0x77, 0x8a, 0x6b, 0x43, 0x62, 0x72, 0x73, 0x82, 0x88, 0xa2, 0xe2, 0xfb,
    0x5f, 0xae, 0xda, 0x3d, 0x1e, 0x68, 0x13, 0x1a, 0x8d, 0xa2, 0x36, 0x8f, 0x11, 0x4b, 0xb5, 0xfb,
    0x79
};

static unsigned char g_publicExponent[] = {0x00, 0x01, 0x00, 0x01};//65537

static unsigned char g_privateExponent[] =
{
    0x4d, 0x2b, 0x7a, 0x64, 0x38, 0x95, 0xf4, 0xe9, 0xa4, 0x59, 0x51, 0x1a, 0x2d, 0xe6, 0x0d, 0x3d,
    0xa2, 0xac, 0x30, 0xd6, 0x6b, 0xeb, 0x49, 0x1f, 0x29, 0x15, 0x50, 0xa9, 0x8f, 0x0d, 0xab, 0x6a,
    0xc5, 0xe1, 0xac, 0x43, 0xb0, 0xb5, 0x2f, 0xa1, 0x1a, 0x7f, 0x5f, 0x78, 0xc4, 0xa1, 0x80, 0x14,
    0xc6, 0x3c, 0x3c, 0xa3, 0x3f, 0x60, 0x47, 0x2a, 0xfa, 0x3e, 0x21, 0x71, 0x0c, 0xe9, 0x5d, 0xfb,
    0xe8, 0x5c, 0x5a, 0xc9, 0x45, 0x3f, 0x9b, 0xc8, 0x3d, 0xfc, 0x08, 0x99, 0xf2, 0x80, 0x5c, 0x60,
    0x40, 0xa4, 0x23, 0x71, 0x68, 0x5e, 0xc9, 0xba, 0x2e, 0x4f, 0x50, 0xb3, 0x71, 0x82, 0x01, 0xff,
    0xb8, 0x30, 0x92, 0xb7, 0x8e, 0xf2, 0x12, 0xe9, 0xdd, 0x53, 0x22, 0x9c, 0x33, 0xba, 0x5f, 0xd7,
    0x6d, 0x2c, 0x9d, 0xe4, 0xcd, 0x35, 0x64, 0x5b, 0xd5, 0x38, 0xd1, 0x23, 0x8d, 0x97, 0xbe, 0xdc,
    0x9a, 0x7a, 0xe2, 0xe7, 0x90, 0x53, 0x2c, 0x17, 0x91, 0x43, 0x6d, 0xf7, 0x53, 0x2d, 0x2c, 0x40,
    0x68, 0x77, 0x3e, 0x19, 0x53, 0xf9, 0x2b, 0x03, 0xa3, 0x20, 0x6e, 0x35, 0x30, 0xb6, 0xd5, 0x40,
    0x86, 0x99, 0x81, 0x45, 0x9a, 0xdc, 0xe1, 0x70, 0x07, 0x81, 0xdc, 0xdf, 0xcb, 0x13, 0x1e, 0xf6,
    0xbb, 0x8d, 0x08, 0x4c, 0xea, 0x02, 0x1f, 0xcf, 0x5b, 0x8b, 0x0e, 0x50, 0x0f, 0xe5, 0x0a, 0x06,
    0x32, 0xb1, 0x51, 0x80, 0x7b, 0x25, 0x44, 0x4c, 0x72, 0xba, 0x0d, 0x19, 0x03, 0x0f, 0x97, 0x86,
    0x75, 0x67, 0x22, 0xe3, 0x9d, 0x2b, 0x5b, 0xef, 0x25, 0xf0, 0x8d, 0x79, 0x9d, 0x32, 0x95, 0x6b,
    0x6c, 0xf0, 0x19, 0x54, 0xd2, 0xed, 0xf9, 0x14, 0x2a, 0xb9, 0x26, 0x94, 0xbe, 0x89, 0x1e, 0x3b,
    0x30, 0xb9, 0x94, 0xb7, 0x2a, 0xf1, 0x6b, 0x5c, 0xc7, 0xe1, 0x32, 0x09, 0x3f, 0xab, 0xfd, 0x01
};

static void print_array(unsigned char *buff, unsigned int size)
{
    #if 1
    int i = 0;

    for (i = 0; i < size; i++)
    {
        if (i && !(i % 8))
        {
            printf("  ");
        }

        if (i && !(i % 16))
        {
            printf("\n");
        }

        printf("%02X ", buff[i]);
    }

    printf("\n\n");
    #endif
}
#endif

/**************************************
*/
static int tz_fd = -1;
int tz_open(void)
{
    tz_fd = open(QL_TRUSTZONE_DEV, O_RDWR);
    //log_o(LOG_MID, "tz_fd = %d", tz_fd);

    if (tz_fd < 0)
    {
        //log_e(LOG_MID, "Fail to open tzone");
        return -1;
    }

    return 0;
}

int tz_close(void)
{
    int fd = tz_fd;
    tz_fd = -1;

    return close(fd);
}

/************************************************
input:
@size: [1,512] byte(s)
output:
@rnd_buff size: [1,512] byte(s)
*/
int tz_random_gen(unsigned char *rnd_buff, unsigned int size)
{
    int ret = -1;
    ql_tz_random_data_t random_data;

    if ((size == 0) || (size > 512))
    {
        return -1;
    }

    random_data.data_size = size; // bytes
    random_data.data_ptr = rnd_buff;

    ret = ioctl(tz_fd, QL_TZ_CRYPTO_SVC_RANDOM_DATA_GEN, &random_data);

    //print_array(rnd_buff, size);

    return ret;
}

/****************** AES *****************************
output:
@aes_key_buff size: 56 bytes
*/
int tz_aes_gen_key(unsigned char *aes_key_buff)
{
    int ret = -1;
    ql_tz_aes_generate_key_t keyblob;

    keyblob.key_size = 256; // bits
    keyblob.key_ptr = aes_key_buff;

    ret = ioctl(tz_fd, QL_TZ_CRYPTO_SVC_AES_KEY_GEN, &keyblob); // keyblob.key_size=56(bytes)

    //print_array(aes_key_buff, keyblob.key_size);

    return ret;
}

/*
input:
@input_aes_buff size: 32 bytes
output:
@aes_key_buff size: 56 bytes
*/
int tz_aes_import_key(unsigned char *input_aes_buff, unsigned char *aes_key_buff)
{
    int ret = -1;
    ql_tz_aes_import_key_t ikeyblob;

    ikeyblob.input_aes_size = 32; // bytes
    ikeyblob.input_aes_ptr = input_aes_buff;

    ikeyblob.key_size = 56; // bytes
    ikeyblob.key_ptr = aes_key_buff;

    ret = ioctl(tz_fd, QL_TZ_CRYPTO_SVC_AES_KEY_IMPORT, &ikeyblob);

    //print_array(input_aes_buff, ikeyblob.input_aes_size);
    //print_array(aes_key_buff, ikeyblob.key_size);

    return ret;
}

/*
input:
@aes_key_buff size:
56 bytes
@plaintext_buff size: must >=1 byte and <= 2048 bytes
@plaintext_size:  must >=1 and <= 2048
output:
@ciphertext_buff size: plaintext_buff size + 24 // ciphertext will grow longer 24 bytes
*/
int tz_aes_encrypt_data(unsigned char *aes_key_buff,
                        unsigned char *plaintext_buff, unsigned int plaintext_size,
                        unsigned char *ciphertext_buff)
{
    int ret = -1;
    ql_tz_aes_endecrypt_data_t enc_data;

    if ((plaintext_size == 0) || (plaintext_size > 2048))
    {
        return -1;
    }

    enc_data.key_size = 56; // bytes
    enc_data.key_ptr = aes_key_buff;
    enc_data.input_data_size = plaintext_size; // bytes
    enc_data.input_data_ptr = plaintext_buff;

    enc_data.output_data_size = plaintext_size + 24; // bytes
    enc_data.output_data_ptr = ciphertext_buff;

    ret = ioctl(tz_fd, QL_TZ_CRYPTO_SVC_AES_DATA_ENCRYPT, &enc_data);

    //print_array(plaintext_buff, plaintext_size);
    //print_array(ciphertext_buff, plaintext_size + 24);

    return ret;
}

/*
input:
@aes_key_buff size:
56 bytes
@ciphertext_buff size: must >= 1+24 bytes and <= 2048+24 bytes
@ciphertext_size: must >= 1+24 and <= 2048+24
output:
@plaintext_buff size: ciphertext_size - 24
*/
int tz_aes_decrypt_data(unsigned char *aes_key_buff,
                        unsigned char *ciphertext_buff, unsigned int ciphertext_size,
                        unsigned char *plaintext_buff)
{
    int ret = -1;
    ql_tz_aes_endecrypt_data_t dec_data;

    if ((ciphertext_size < 1 + 24) || (ciphertext_size > 2048 + 24))
    {
        return -1;
    }

    dec_data.key_size = 56; // bytes
    dec_data.key_ptr = aes_key_buff;
    dec_data.input_data_size = ciphertext_size; // bytes
    dec_data.input_data_ptr = ciphertext_buff;

    dec_data.output_data_size = ciphertext_size - 24; // bytes
    dec_data.output_data_ptr = plaintext_buff;

    ret = ioctl(tz_fd, QL_TZ_CRYPTO_SVC_AES_DATA_DECRYPT, &dec_data);

    //print_array(ciphertext_buff, ciphertext_size);
    //print_array(plaintext_buff, ciphertext_size - 24);

    return ret;
}


/************************** RSA **********************************
input:
@modulus_size: QL_TZ_RSA_MODULUS_1024_BITS or QL_TZ_RSA_MODULUS_2048_BITS
@padding_type: QL_TZ_RSA_PKCS115_SHA2_256 or QL_TZ_RSA_PSS_SHA2_256
output:
@rsa_key_buff size: 1656 bytes
*/
int tz_rsa_generate_key(ql_tz_rsa_modulus_size_t modulus_size,
                        ql_tz_crypto_rsa_digest_pad_t padding_type, ql_crypto_rsa_key_blob_t *rsa_key_buff)
{
    int ret = -1;
    ql_tz_rsa_generate_key_t rsa_key_blob_req;

    rsa_key_blob_req.modulus_size = modulus_size * 8; // bits
    rsa_key_blob_req.public_exponent = 0x010001;//65537
    rsa_key_blob_req.digest_pad_type = padding_type;

    rsa_key_blob_req.rsa_key_blob = rsa_key_buff;

    ret = ioctl(tz_fd, QL_TZ_CRYPTO_SVC_RSA_KEYPAIR_GEN, &rsa_key_blob_req);

    //print_array((unsigned char *)rsa_key_buff, sizeof(ql_crypto_rsa_key_blob_t));

    return ret;
}

/*
have a bug : pub_exp_size only eq 4
input:
@modulus:
@modulus_size:
@pub_exp_buff:
@pub_exp_size:
@pri_exp_buff:
@pri_exp_size:
@padding_type:
output:
@rsa_key_buff:
*/
int tz_rsa_import_keypair(unsigned char *modulus, unsigned int modulus_size,
                          unsigned char *pub_exp_buff, unsigned int pub_exp_size,
                          unsigned char *pri_exp_buff, unsigned int pri_exp_size,
                          ql_tz_crypto_rsa_digest_pad_t padding_type, ql_crypto_rsa_key_blob_t *rsa_key_buff)
{
    int ret = -1;
    ql_tz_rsa_keypair_import_t import_keypair;

    import_keypair.in_modulus = modulus;
    import_keypair.in_modulusSize = modulus_size; // bytes
    import_keypair.in_pubExp = pub_exp_buff;
    import_keypair.in_pubExpSize = pub_exp_size; // bytes
    import_keypair.in_privExp = pri_exp_buff;
    import_keypair.in_privExpSize = pri_exp_size; // bytes
    import_keypair.in_padding = padding_type;
    import_keypair.rsa_key_blob = rsa_key_buff;

    ret = ioctl(tz_fd, QL_TZ_CRYPTO_SVC_RSA_KEYPAIR_IMPORT, &import_keypair);

    //print_array(import_keypair.in_modulus, import_keypair.in_modulusSize);
    //print_array(import_keypair.in_pubExp, import_keypair.in_pubExpSize);
    //print_array(import_keypair.in_privExp, import_keypair.in_privExpSize);
    //print_array((unsigned char *)rsa_key_buff, sizeof(ql_crypto_rsa_key_blob_t));
    //printf("ret=%d\n", ret);

    return ret;
}

/*
input:
@rsa_key_buff size: 1656 bytes
output:
@pub_key_buff
*/
int tz_rsa_export_pubkey(ql_crypto_rsa_key_blob_t *rsa_key_buff,
                         rsa_public_key_t *pub_key_buff)
{
    int ret = -1;
    ql_tz_rsa_public_key_export_t export_pubkey;

    export_pubkey.rsa_key_blob = rsa_key_buff;
    export_pubkey.modulus_data_size = pub_key_buff->modulus_size; // bytes
    export_pubkey.modulus_data_ptr = pub_key_buff->modulus_data;
    export_pubkey.public_exponent_size = pub_key_buff->public_exponent_size; // bytes
    export_pubkey.public_exponent_ptr = pub_key_buff->public_exponent;

    ret = ioctl(tz_fd, QL_TZ_CRYPTO_SVC_RSA_PUBKEY_EXPORT, &export_pubkey);

    //print_array((unsigned char *)pub_key_buff, sizeof(rsa_public_key_t));

    return ret;
}

/*
input:
@rsa_key_buff:
@plaintext_buff
@plaintext_size
output:
@sign_buff
*/
int tz_rsa_sign_data(ql_crypto_rsa_key_blob_t *rsa_key_buff,
                     unsigned char *plaintext_buff, unsigned int plaintext_size,
                     unsigned char *sign_buff)
{
    int    ret = -1;
    ql_tz_rsa_sign_verify_data_t rsa_sign_data;

    if ((plaintext_size == 0) || (plaintext_size > 2048))
    {
        return -1;
    }

    rsa_sign_data.rsa_key_blob = rsa_key_buff;
    rsa_sign_data.input_data_size = plaintext_size;
    rsa_sign_data.input_data_ptr = plaintext_buff;
    rsa_sign_data.signature_length = 256; // bytes
    rsa_sign_data.signature_ptr = sign_buff;

    ret = ioctl(tz_fd, QL_TZ_CRYPTO_SVC_RSA_DATA_SIGN, &rsa_sign_data);

    //print_array(sign_buff, 256);

    return ret;
}

/*
input:
@rsa_key_buff:
@plaintext_buff
@plaintext_size
@sign_buff
*/
int tz_rsa_verify_sign(ql_crypto_rsa_key_blob_t *rsa_key_buff,
                       unsigned char *plaintext_buff, unsigned int plaintext_size,
                       unsigned char *sign_buff)
{
    int     ret = -1;
    ql_tz_rsa_sign_verify_data_t verify_data;

    if ((plaintext_size == 0) || (plaintext_size > 2048))
    {
        return -1;
    }

    verify_data.rsa_key_blob = rsa_key_buff;
    verify_data.input_data_size = plaintext_size;
    verify_data.input_data_ptr = plaintext_buff;
    verify_data.signature_length = 256; // bytes
    verify_data.signature_ptr = sign_buff;

    //print_array(sign_buff, 256);

    ret = ioctl(tz_fd, QL_TZ_CRYPTO_SVC_RSA_SINDATA_VERIFY, &verify_data);

    //printf("%s: ret=%d\n", __func__, ret);

    return ret;
}

#if 0
int main(int argc, char *argv[])
{
    int ret = -1;
    tz_open();

    // tz_random_gen
    unsigned char rnd_buff[512] = {0};

    // tz_aes_gen_key
    // tz_aes_import_key
    unsigned char aes_key_buff[56] = {0};

    // tz_aes_encrypt_data
    // tz_aes_decrypt_data
    unsigned char plaintext_buff[2048] = {0};
    unsigned char ciphertext_buff[2048 + 24] = {0};

    // tz_rsa_generate_key
    // tz_rsa_import_keypair
    ql_crypto_rsa_key_blob_t rsa_key_buff = {0};
    // tz_rsa_export_pubkey
    rsa_public_key_t pub_key_buff = {0};

    // tz_rsa_sign_data
    // tz_rsa_verify_sign
    unsigned char sign_buff[256] = {0};

    if (argc >= 2)
    {
        const char *op = argv[1];

        if (!strcasecmp(op, "random_gen"))
        {
            ret = tz_random_gen(rnd_buff, 512);
        }
        else if (!strcasecmp(op, "aes_gen_key"))
        {
            ret = tz_aes_gen_key(aes_key_buff);
        }
        else if (!strcasecmp(op, "aes_import_key"))
        {
            ret = tz_aes_import_key(g_AESKey, aes_key_buff);
        }
        else if (!strcasecmp(op, "aes_import_random_key"))
        {
            ret = tz_random_gen(rnd_buff, 32);
            ret |= tz_aes_import_key(rnd_buff, aes_key_buff);
        }
        else if (!strcasecmp(op, "aes_enc"))
        {
            ret = tz_aes_import_key(g_AESKey, aes_key_buff);
            ret |= tz_aes_encrypt_data(aes_key_buff, g_AESKey, 32, ciphertext_buff);
        }
        else if (!strcasecmp(op, "aes_dec"))
        {
            ret = tz_aes_import_key(g_AESKey, aes_key_buff);
            ret |= tz_aes_encrypt_data(aes_key_buff, g_AESKey, 32, ciphertext_buff);
            ret |= tz_aes_decrypt_data(aes_key_buff, ciphertext_buff, 32 + 24, plaintext_buff);
        }
        else if (!strcasecmp(op, "rsa_gen_key"))
        {
            ret = tz_rsa_generate_key(QL_TZ_RSA_MODULUS_2048_BITS, QL_TZ_RSA_PKCS115_SHA2_256, &rsa_key_buff);
        }
        else if (!strcasecmp(op, "rsa_import_keypair"))
        {
            ret = tz_rsa_import_keypair(g_modulus, sizeof(g_modulus),
                                        g_publicExponent, sizeof(g_publicExponent),
                                        g_privateExponent, sizeof(g_privateExponent),
                                        QL_TZ_RSA_PKCS115_SHA2_256, &rsa_key_buff);
        }
        else if (!strcasecmp(op, "rsa_export_pubkey"))
        {
            ret = tz_rsa_generate_key(QL_TZ_RSA_MODULUS_2048_BITS, QL_TZ_RSA_PKCS115_SHA2_256, &rsa_key_buff);
            pub_key_buff.modulus_size = rsa_key_buff.modulus_size;
            pub_key_buff.public_exponent_size = rsa_key_buff.public_exponent_size;
            ret |= tz_rsa_export_pubkey(&rsa_key_buff, &pub_key_buff);
        }
        else if (!strcasecmp(op, "rsa_sign"))
        {
            ret = tz_rsa_generate_key(QL_TZ_RSA_MODULUS_2048_BITS, QL_TZ_RSA_PKCS115_SHA2_256, &rsa_key_buff);
            ret |= tz_rsa_sign_data(&rsa_key_buff, g_AESKey, 32, sign_buff);
        }
        else if (!strcasecmp(op, "rsa_verify"))
        {
            ret = tz_rsa_generate_key(QL_TZ_RSA_MODULUS_2048_BITS, QL_TZ_RSA_PKCS115_SHA2_256, &rsa_key_buff);
            ret |= tz_rsa_sign_data(&rsa_key_buff, g_AESKey, 32, sign_buff);
            ret |= tz_rsa_verify_sign(&rsa_key_buff, g_AESKey, 32, sign_buff);
        }
        else
        {
            printf("unknow op=%s\n", op);
            return -1;
        }

        tz_close();
    }
    else
    {
        printf("./tz random_gen\n");
        printf("./tz aes_gen_key\n");
        printf("./tz aes_import_key\n");
        printf("./tz aes_import_random_key\n");
        printf("./tz aes_enc\n");
        printf("./tz aes_dec\n");
        printf("./tz rsa_gen_key\n");
        printf("./tz rsa_import_keypair\n");
        printf("./tz rsa_export_pubkey\n");
        printf("./tz rsa_sign\n");
        printf("./tz rsa_verify\n");
    }

    return ret;
}
#endif

