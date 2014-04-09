#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(__APPLE__)
#  define COMMON_DIGEST_FOR_OPENSSL
#  include <CommonCrypto/CommonDigest.h>
#else
#  include <openssl/md5.h>
#endif


/************************************************
            SPOLECNE FUNKCE A DEFINICE
*************************************************/

typedef unsigned char byte;
typedef unsigned int uint;
const int BITS_IN_BYTE = 8;

typedef struct {
    uint loglog;
    uint hyperloglog;
} cardinalities;

unsigned char *str2md5(const char *str, int length) {
    MD5_CTX c;
    unsigned char *digest = (unsigned char *)malloc(sizeof(unsigned char) * 16);
    const int blocklength = 512;
    MD5_Init(&c);

    while (length > 0) {
        if (length > blocklength) {
            MD5_Update(&c, str, blocklength);
        } else {
            MD5_Update(&c, str, length);
        }
        length -= blocklength;
        str += blocklength;
    }

    MD5_Final(digest, &c);
    return digest;
}

uint max(uint a, uint b) {
    return a > b ? a : b;
}

/*
Vrati pozici nejlevejsiho bitu, ktery je roven 1.
Zacina hledat od index bitfrom. Indexuje se od 1.
rho(1001000010, 10, 4) = 6
                        ^
*/
uint rho(const byte *digest, uint bitlength, uint bitfrom) {
    uint b = 0;
    uint byteIdx = 0;
    uint bitIdx = 0;
    for (b = bitfrom; b < bitlength; b++) {
        byteIdx = b / BITS_IN_BYTE;
        bitIdx  = (BITS_IN_BYTE - 1) - b % BITS_IN_BYTE;
        if ((digest[byteIdx] & (1 << bitIdx)) != 0) {
            return (b - bitfrom + 1);
        }      
    }
    
    return (bitlength-bitfrom) + 1;
}

/*
Prevede prvnich bucketBitLength bitu na cislo
bucketIndex(1001000010, 4) = 1001 = 9
*/
uint bucketIndex(byte *digest, uint bucketBitLength) {
    uint index;
    uint bytesHashLength = 4;
    byte temparray[bytesHashLength];
    for (int i = 0; i < bytesHashLength; i++) {
        temparray[bytesHashLength - i - 1]  = digest[i];
    }
    memcpy(&index, temparray, sizeof(uint));
    index = index >> (32 - bucketBitLength);
    return index;
}

// Precte jeden radek ze souboru, odstrani \n z konce radku
char *readline(FILE *fp, char *buffer, int max) {
    char *res;
    res = fgets(buffer, max, fp);
    if (res) {
        buffer[strlen(buffer) - 1] = 0;
        return buffer;
    } else {
        return NULL;
    }
}

// Spocita rho hodnoty pro vsechny vstupni retezce a ulozi je do spravnych buckets
byte *computeMaxes(uint b, uint digestBitLength, FILE *fp, uint m) {
    byte *digest;
    char *word;
    uint j, first1;
    const uint bufferlength = 256; // maximalni delka slova na radku = 255
    char buffer[bufferlength];
    byte *M = (byte*) calloc(m, sizeof(byte));
    while ((word = readline(fp, buffer, bufferlength)) != NULL) {
        digest = str2md5(word, (int)strlen(word));
        j = bucketIndex(digest, b);
        first1 = rho(digest, digestBitLength, b);
        M[j] = max(M[j], first1);
        free(digest);
    }
    return M;
}



/************************************************
                    LOG LOG
*************************************************/

double computeLogLogCardinality(uint m, byte *M, double alpham) {
    double E = 0;
    double sum = 0, arithmeticMean;
    for (int j = 0; j < m; j++) {
        sum += M[j];
    }
    arithmeticMean = sum / m; 
    E = alpham * m * pow(2, arithmeticMean);
    return E;
}

double loglog(uint b, uint digestBitLength, FILE *fp) {
    uint m = (uint) pow(2, b);
    byte *M = computeMaxes(b, digestBitLength, fp, m);
    double cardinality = computeLogLogCardinality(m, M, 0.39701);
    free(M);
    return cardinality;
}



/************************************************
                HYPER LOG LOG
*************************************************/
double computeHyperAlpha(unsigned int m) {
    return 0.7213 / (1 + 1.079 / m);
}

double computeHyperCardinality(uint m, byte *M, double alpham) {
    double E = 0;
    double sum = 0, harmonicMean;
    for (int j = 0; j < m; j++) {
        sum += pow(2, -M[j]);
    }
    harmonicMean = m / sum; 
    E = alpham * m * harmonicMean;
    return E;
}

// ToDo:
// double applyCorrections(double E, uint m, byte *M) {
//   uint V = 0;
//   double Estar = E;

//   if (E <= ((5 / 2) * m)) {
//     for (uint i = 0; i < m; i++) {
//       if (M[i] == 0) {
//         V++;
//       }
//     }
//     if (V != 0) {
//       Estar = 
//     }
//   }
// }

double hyperloglog(uint b, uint digestBitLength, FILE *fp) {
    uint m = (uint) pow(2, b);
    byte *M = computeMaxes(b, digestBitLength, fp, m);
    double cardinality = computeHyperCardinality(m, M, computeHyperAlpha(m));
    free(M);
    // ToDo: applyCorrections
    return cardinality;
}

cardinalities bothlog(uint b, uint digestBitLength, FILE *fp) {
    cardinalities card;
    uint m = (uint) pow(2, b);
    byte *M = computeMaxes(b, digestBitLength, fp, m);
    card.loglog = (uint)computeLogLogCardinality(m, M, 0.39701);
    card.hyperloglog = (uint)computeHyperCardinality(m, M, computeHyperAlpha(m));
    free(M);
    return card;
}


int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("Bylo predano malo parametru.\n");
        return EXIT_FAILURE;
    }
    uint b = 12;
    char *path = argv[1];
    if (argc > 2) {
        b = (uint)atoi(argv[2]);
    }
    
    // ToDo: osetreni chyb pri otevirani a cteni ze souboru
    FILE *fp = fopen(path, "r");
    // printf("hyperloglog: %u\n", (uint)hyperloglog(b, 32, fp));
    // printf("loglog:      %u\n", (uint)loglog(b, 32, fp));
    cardinalities card = bothlog(b, 32, fp);
    printf("loglog:      %u\nhyperloglog: %u\n", card.loglog, card.hyperloglog);
    return EXIT_SUCCESS;
}