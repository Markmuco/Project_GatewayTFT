/*!
 * \file md5.h
 *
 * \brief This software module provides functions for md5 hashes.
 */
#ifndef _MD5_H_
#define _MD5_H_

#define MD5_DIGEST_SIZE   (16)

typedef struct
{
  uint32_t i[2]; // number of _bits_ handled mod 2^64
  uint32_t buf[4]; // scratch buffer
  uint8_t in[64]; // input buffer
  uint8_t digest[16]; // actual digest after MD5Final call
} MD5_CTX;

void MD5Init(MD5_CTX *mdContext);
void MD5Update(MD5_CTX *mdContext, uint8_t *inBuf, uint32_t inLen);
void MD5Final(MD5_CTX *mdContext);

#endif
