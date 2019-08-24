#include "stdafx.h"
#include "CryptoUtility.h"
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <algorithm>
// openssl/applink.c 内の fopen() への warning C4996 の抑制
#ifdef _WIN32
#ifdef WIN32
#define WIN32_PREDEFINED
#else
#define WIN32
#endif
#pragma warning(disable: 4996)
#endif
#include <openssl/applink.c>
#ifdef WIN32
#ifdef WIN32_PREDEFINED
#undef WIN32_PREDEFINED
#else
#undef WIN32
#endif
#pragma warning(default: 4996)
#endif
// warning C4996 の抑制はここまで

void ETLicense::InitializeOpenSSL()
{
    RAND_poll();
    while (RAND_status() == 0)
    {
        unsigned short rand_ret = rand() % 65536;
        RAND_seed(&rand_ret, sizeof(rand_ret));
    }
}


bool ETLicense::Encrypt(const char* data, const size_t datalen, const unsigned char* key, const unsigned char* iv, unsigned char* dest, size_t& destlen)
{
    EVP_CIPHER_CTX en;
    int i = 0;
    int f_len = 0;
    int c_len = static_cast<int>(destlen);

    memset(dest, 0x00, destlen);

    EVP_CIPHER_CTX_init(&en);
    EVP_EncryptInit_ex(&en, EVP_aes_256_cbc(), NULL, key, iv);

    EVP_EncryptUpdate(&en, dest, &c_len, (unsigned char*)data, static_cast<int>(datalen));
    EVP_EncryptFinal_ex(&en, (unsigned char *)(dest + c_len), &f_len);
    EVP_CIPHER_CTX_cleanup(&en);

    destlen = c_len + f_len;
    return true;
}


bool ETLicense::Decrypt(const unsigned char* data, const size_t datalen, const unsigned char* key, const unsigned char* iv, unsigned char* dest, size_t& destlen)
{
    EVP_CIPHER_CTX de;
    int f_len = 0;
    int p_len = static_cast<int>(datalen);

    memset(dest, 0x00, destlen);

    EVP_CIPHER_CTX_init(&de);
    EVP_DecryptInit_ex(&de, EVP_aes_256_cbc(), NULL, key, iv);

    EVP_DecryptUpdate(&de, (unsigned char *)dest, &p_len, data, static_cast<int>(datalen));
    EVP_DecryptFinal_ex(&de, (unsigned char *)(dest + p_len), &f_len);

    EVP_CIPHER_CTX_cleanup(&de);

    //printf("p_len: %d\n", p_len);
    //printf("f_len: %d\n", f_len);
    //printf("%s\n", dest);
    //PrintBytes(dest, destlen);
    
    destlen = p_len + f_len;

    return true;
}


bool ETLicense::MD5(
    const char* data,
    const size_t datalen,
    unsigned char hash[16])
{
    MD5_CTX c;
    MD5_Init(&c);
    MD5_Update(&c, data, datalen);
    MD5_Final(hash, &c);
    return true;
}


bool ETLicense::SHA256(
    const char* data,
    const size_t datalen,
    unsigned char hash[32])
{
    SHA256_CTX c;
    SHA256_Init(&c);
    SHA256_Update(&c, data, datalen);
    SHA256_Final(hash, &c);
    return 0;
}


void ETLicense::EncodeBase64(
    const unsigned char* data,
    const int datalen,
    std::string& str)
{
    BIO *bmem, *b64;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, data, datalen);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    char *buf = (char *)malloc(bptr->length);
    memcpy(buf, bptr->data, bptr->length - 1);
    buf[bptr->length - 1] = 0;
    BIO_free_all(b64);
    str = buf;
    free(buf);

    str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
}


void ETLicense::EncodeBase64(
    const std::string& data,
    std::string& str)
{
    BIO *bmem, *b64;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, data.c_str(), static_cast<int>(data.size()));
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    char *buf = (char *)malloc(bptr->length);
    memcpy(buf, bptr->data, bptr->length - 1);
    buf[bptr->length - 1] = 0;
    BIO_free_all(b64);
    str = buf;
    free(buf);

    str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
}


void ETLicense::DecodeBase64(
    const std::string& str,
    unsigned char*& data,
    int& datalen)
{
    data = (unsigned char*)malloc(str.size());
    datalen = static_cast<int>(str.size());
    BIO *bmem = BIO_new_mem_buf(str.c_str(), static_cast<int>(str.size()));
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_push(b64, bmem);

    long n = BIO_read(bmem, data, datalen);
    if (n > 0)
    {
        data[n] = 0;
        datalen = n;
    }
    else
    {
        data[0] = 0; // note: this is an error state.
    }
    BIO_free_all(bmem);
}


bool ETLicense::MakeSalt(Salt& salt)
{
    unsigned char tmp[32];
    int result = RAND_bytes(tmp, 32);

    std::string encoded;
    EncodeBase64(tmp, 32, encoded);
    salt = encoded;
    return true;
}
