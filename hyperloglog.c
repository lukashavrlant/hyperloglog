#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(__APPLE__)
#  define COMMON_DIGEST_FOR_OPENSSL
#  include <CommonCrypto/CommonDigest.h>
#  define SHA1 CC_SHA1
#else
#  include <openssl/md5.h>
#endif


/************************************************

            SPOLECNE FUNKCE A DEFINICE

*************************************************/

typedef unsigned char byte;
typedef unsigned int uint;
typedef char*(GENERATOR)(FILE*);
const uint HASH_LENGTH = 32;
const int BITS_IN_BYTES = 8;
// const byte bitflags[] = {128, 64, 32, 16, 8, 4, 2, 1};
const byte bitflags[] = {1, 2, 4, 8, 16, 32, 64, 128};

unsigned char *str2md5(const char *str, int length) {
    int n;
    MD5_CTX c;
    unsigned char *digest = (unsigned char *)malloc(sizeof(unsigned char) * 16);

    MD5_Init(&c);

    while (length > 0) {
        if (length > 512) {
            MD5_Update(&c, str, 512);
        } else {
            MD5_Update(&c, str, length);
        }
        length -= 512;
        str += 512;
    }

    MD5_Final(digest, &c);

    return digest;
}

unsigned int max(unsigned int a, unsigned int b) {
  return a > b ? a : b;
}

uint rho(const unsigned char * buffer, uint nbits, uint bitfrom) {
    uint b = 0;
    uint byteIdx = 0;
    uint bitIdx = 0;
    for (b = bitfrom; b < nbits; b++) {
        byteIdx = b / 8;
        bitIdx  = b % 8;
        if ((buffer[byteIdx] & (0x1 << bitIdx)) != 0)
            return (b - bitfrom + 1);
        
    }
    
    return (nbits-bitfrom) + 1;

}

uint bIndex(byte *checksum, uint length) {
  uint index = 0;
  uint b;
  uint byteIdx = 0;
  uint bitIdx = 0;
  for (b = 0; b < length; b++) {
      byteIdx = b / 8;
      bitIdx  = b % 8;
      if ((checksum[byteIdx] & (1 << bitIdx)) != 0) {
        index += (1 << (length - b - 1));
      }
  }
  return index;
}

uint bucketIndex(byte *checksum, uint length) {
  int index = 0;
  int exponent;
  int bit;
  uint counter = 0;
  for (uint i = 0; counter < length; i++) {
    for (uint j = 0; j < BITS_IN_BYTES && counter < length; j++) {
      exponent = length - ((i * BITS_IN_BYTES) + j) - 1;
      bit = (checksum[i] & bitflags[j]) == bitflags[j];
      if (bit) {
        // printf("bucketIndex: %u, j: %u\n", exponent, j);
      }
      index += pow(2, exponent) * bit;
      counter++;
    }
  }
  return index;
}



void computeMaxes(uint b, uint checksumLength, GENERATOR generator, byte *M) {
  byte *checksum;
  char *word;
  uint j, jj, first1;
  uint  counter = 0;
  while ((word = generator(NULL)) != NULL) {
    checksum = str2md5(word, strlen(word));
    j = bIndex(checksum, b);
    first1 = rho(checksum, checksumLength, b);
    M[j] = max(M[j], first1);
    free(checksum);
  }
}

/************************************************

                      LOG LOG

*************************************************/

double computeLogLogCardinality(uint m, byte *M, double alpham) {
  double E = 0;
  double sum = 0, average;
  for (int j = 0; j < m; j++) {
    sum += M[j];
    // printf("%u,", M[j]);
  }
  average = sum / m;
  E = alpham * m * pow(2, average);
  // printf("\nsum %g, avg: %g, E: %g\n", sum, average, E);
  return E;
}

double loglog(uint b, uint checksumLength, GENERATOR generator) {
  uint m = (uint) pow(2, b);
  byte *M = (byte*) calloc(m, sizeof(byte));
  computeMaxes(b, checksumLength, generator, M);
  double cardinality = computeLogLogCardinality(m, M, 0.39701);
  return cardinality;
}



/************************************************

              HYPER LOG LOG

*************************************************/
double computeAlpha(unsigned int m) {
  return (0.7213/(1+1.079/m));
}

double computeHyperCardinality(uint m, byte *M, double alpham) {
  double E = 0;
  double sum = 0, average;
  for (int j = 0; j < m; j++) {
    sum += pow(2, -M[j]);
    // printf("%u, ", M[j]);
  }
  average = m / sum;
  E = alpham * m * average;
  return E;
}


double hyperloglog(uint b, uint checksumLength, GENERATOR generator) {
  uint m = (uint) pow(2, b);
  byte *M = (byte*) calloc(m, sizeof(byte));
  computeMaxes(b, checksumLength, generator, M);
  double cardinality = computeHyperCardinality(m, M, computeAlpha(m));
  return cardinality;
}


// Input stream of words
char *lines(FILE *fp) {
  static FILE *inputfile;
  static char *lineBuffer = (char*)malloc(128);
  static size_t read;
  static size_t len;

  if (fp) {
    inputfile = fp;
    return NULL;
  }

  read = getline(&lineBuffer, &len, inputfile);
  if (read != EOF) {
    if (lineBuffer[read - 1] == '\n') {
      lineBuffer[read - 1] = 0;
    }
    return lineBuffer;
  } else {
    fclose(inputfile);
    return NULL;
  }
}




void reverse(byte *arr, int length) {
  byte temp;
  for (int i = 0; i <= length / 2; i++) {
    temp = arr[i];
    arr[i] = arr[length - i - 1];
    arr[length - i - 1] = temp;
  }
}


int main(int argc, char **argv) {
  // int length = 4;
  // byte checksum[] = {158,147,209,155,};
  // printf("bucketIndex: %u\n", bucketIndex(checksum, length));
  // printf("bIndex:%u\n", bIndex(checksum, length));
    char *path = (char*)"1000000.txt";
    if (argc > 1) {
      path = argv[1];
    }
    
    FILE *fp = fopen(path, "r");
    lines(fp);
    printf("hyperloglog: %g\n", hyperloglog(12, 32, lines));
    fp = fopen(path, "r");
    lines(fp);
    printf("loglog:      %g\n", loglog(12, 32, lines));
    return 0;
}