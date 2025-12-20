#include "miner.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "sha3/sph_blake.h"
#include "sha3/sph_bmw.h"
#include "sha3/sph_groestl.h"
#include "sha3/sph_jh.h"
#include "sha3/sph_keccak.h"
#include "sha3/sph_skein.h"
#include "sha3/sph_luffa.h"
#include "sha3/sph_cubehash.h"
#include "sha3/sph_shavite.h"
#include "sha3/sph_simd.h"
#include "sha3/sph_echo.h"
#include "sha3/sph_sha2.h"    /* para SHA-256 del prevBlockHash */

/* ============================================================
 * Permutación X11R basada en prevBlockHash (32 bytes)
 * header layout (LE en el daemon, pero aquí trabajamos con los
 * mismos bytes que usa Miningcore / cpuminer para el hash).
 *
 * header80: [ version(4) | prevhash(32) | merkle(32)
 *             | time(4) | bits(4) | nonce(4) ]
 * ============================================================ */

static void x11r_get_permutation_from_prevhash(const unsigned char* header80,
                                               int perm_out[11])
{
    const unsigned char* prev = header80 + 4; /* empieza tras version */
    unsigned char seed[32];
    sph_sha256_context ctx_sha;
    int i;

    /* seed = SHA256(prevBlockHash) */
    sph_sha256_init(&ctx_sha);
    sph_sha256(&ctx_sha, prev, 32);
    sph_sha256_close(&ctx_sha, seed);

    /* leemos seed como 4 uint64_t little-endian */
    uint64_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;

    for(i = 0; i < 8; i++)
        s0 |= ((uint64_t) seed[0 + i]) << (8 * i);
    for(i = 0; i < 8; i++)
        s1 |= ((uint64_t) seed[8 + i]) << (8 * i);
    for(i = 0; i < 8; i++)
        s2 |= ((uint64_t) seed[16 + i]) << (8 * i);
    for(i = 0; i < 8; i++)
        s3 |= ((uint64_t) seed[24 + i]) << (8 * i);

    uint64_t state = s0 ^ s1 ^ s2 ^ s3;

    /* inicializamos perm = [0..10] */
    for(i = 0; i < 11; i++)
        perm_out[i] = i;

    /* Fisher–Yates shuffle con xorshift64 */
    for(i = 10; i > 0; --i)
    {
        /* xorshift64 */
        state ^= (state << 13);
        state ^= (state >> 7);
        state ^= (state << 17);

        /* r en [0, i] */
        uint64_t r = state % (uint64_t) (i + 1);
        int j = (int) r;

        int tmp    = perm_out[i];
        perm_out[i] = perm_out[j];
        perm_out[j] = tmp;
    }
}

/* ============================================================
 * X11R hash
 * - Mismas 11 funciones que X11
 * - Orden determinado por permutación basada en prevBlockHash
 * ============================================================ */

void x11rhash(void *output, const void *input)
{
    sph_blake512_context     ctx_blake;
    sph_bmw512_context       ctx_bmw;
    sph_groestl512_context   ctx_groestl;
    sph_skein512_context     ctx_skein;
    sph_jh512_context        ctx_jh;
    sph_keccak512_context    ctx_keccak;
    sph_luffa512_context     ctx_luffa1;
    sph_cubehash512_context  ctx_cubehash1;
    sph_shavite512_context   ctx_shavite1;
    sph_simd512_context      ctx_simd1;
    sph_echo512_context      ctx_echo1;

    /* estos uint512 en el código original son arrays de uint32 */
    uint32_t _ALIGN(64) hashA[16], hashB[16];
    uint32_t* cur;
    uint32_t* nxt;
    int perm[11];
    int r;

    /* input es el header de 80 bytes (como en x11hash) */
    const unsigned char* header80 = (const unsigned char*) input;

    x11r_get_permutation_from_prevhash(header80, perm);

    /* ===== primera ronda: desde el header completo (80 bytes) ===== */
    cur = hashA;

    switch(perm[0])
    {
        case 0: /* BLAKE */
            sph_blake512_init(&ctx_blake);
            sph_blake512(&ctx_blake, input, 80);
            sph_blake512_close(&ctx_blake, cur);
            break;

        case 1: /* BMW */
            sph_bmw512_init(&ctx_bmw);
            sph_bmw512(&ctx_bmw, input, 80);
            sph_bmw512_close(&ctx_bmw, cur);
            break;

        case 2: /* GROESTL */
            sph_groestl512_init(&ctx_groestl);
            sph_groestl512(&ctx_groestl, input, 80);
            sph_groestl512_close(&ctx_groestl, cur);
            break;

        case 3: /* SKEIN */
            sph_skein512_init(&ctx_skein);
            sph_skein512(&ctx_skein, input, 80);
            sph_skein512_close(&ctx_skein, cur);
            break;

        case 4: /* JH */
            sph_jh512_init(&ctx_jh);
            sph_jh512(&ctx_jh, input, 80);
            sph_jh512_close(&ctx_jh, cur);
            break;

        case 5: /* KECCAK */
            sph_keccak512_init(&ctx_keccak);
            sph_keccak512(&ctx_keccak, input, 80);
            sph_keccak512_close(&ctx_keccak, cur);
            break;

        case 6: /* LUFFA */
            sph_luffa512_init(&ctx_luffa1);
            sph_luffa512(&ctx_luffa1, input, 80);
            sph_luffa512_close(&ctx_luffa1, cur);
            break;

        case 7: /* CUBEHASH */
            sph_cubehash512_init(&ctx_cubehash1);
            sph_cubehash512(&ctx_cubehash1, input, 80);
            sph_cubehash512_close(&ctx_cubehash1, cur);
            break;

        case 8: /* SHAVITE */
            sph_shavite512_init(&ctx_shavite1);
            sph_shavite512(&ctx_shavite1, input, 80);
            sph_shavite512_close(&ctx_shavite1, cur);
            break;

        case 9: /* SIMD */
            sph_simd512_init(&ctx_simd1);
            sph_simd512(&ctx_simd1, input, 80);
            sph_simd512_close(&ctx_simd1, cur);
            break;

        case 10: /* ECHO */
            sph_echo512_init(&ctx_echo1);
            sph_echo512(&ctx_echo1, input, 80);
            sph_echo512_close(&ctx_echo1, cur);
            break;

        default: /* por seguridad, BLAKE */
            sph_blake512_init(&ctx_blake);
            sph_blake512(&ctx_blake, input, 80);
            sph_blake512_close(&ctx_blake, cur);
            break;
    }

    /* ===== rondas 2..11: siempre 64 bytes desde el hash anterior ===== */
    for(r = 1; r < 11; r++)
    {
        nxt = (cur == hashA) ? hashB : hashA;

        switch(perm[r])
        {
            case 0: /* BLAKE */
                sph_blake512_init(&ctx_blake);
                sph_blake512(&ctx_blake, cur, 64);
                sph_blake512_close(&ctx_blake, nxt);
                break;

            case 1: /* BMW */
                sph_bmw512_init(&ctx_bmw);
                sph_bmw512(&ctx_bmw, cur, 64);
                sph_bmw512_close(&ctx_bmw, nxt);
                break;

            case 2: /* GROESTL */
                sph_groestl512_init(&ctx_groestl);
                sph_groestl512(&ctx_groestl, cur, 64);
                sph_groestl512_close(&ctx_groestl, nxt);
                break;

            case 3: /* SKEIN */
                sph_skein512_init(&ctx_skein);
                sph_skein512(&ctx_skein, cur, 64);
                sph_skein512_close(&ctx_skein, nxt);
                break;

            case 4: /* JH */
                sph_jh512_init(&ctx_jh);
                sph_jh512(&ctx_jh, cur, 64);
                sph_jh512_close(&ctx_jh, nxt);
                break;

            case 5: /* KECCAK */
                sph_keccak512_init(&ctx_keccak);
                sph_keccak512(&ctx_keccak, cur, 64);
                sph_keccak512_close(&ctx_keccak, nxt);
                break;

            case 6: /* LUFFA */
                sph_luffa512_init(&ctx_luffa1);
                sph_luffa512(&ctx_luffa1, cur, 64);
                sph_luffa512_close(&ctx_luffa1, nxt);
                break;

            case 7: /* CUBEHASH */
                sph_cubehash512_init(&ctx_cubehash1);
                sph_cubehash512(&ctx_cubehash1, cur, 64);
                sph_cubehash512_close(&ctx_cubehash1, nxt);
                break;

            case 8: /* SHAVITE */
                sph_shavite512_init(&ctx_shavite1);
                sph_shavite512(&ctx_shavite1, cur, 64);
                sph_shavite512_close(&ctx_shavite1, nxt);
                break;

            case 9: /* SIMD */
                sph_simd512_init(&ctx_simd1);
                sph_simd512(&ctx_simd1, cur, 64);
                sph_simd512_close(&ctx_simd1, nxt);
                break;

            case 10: /* ECHO */
                sph_echo512_init(&ctx_echo1);
                sph_echo512(&ctx_echo1, cur, 64);
                sph_echo512_close(&ctx_echo1, nxt);
                break;

            default: /* seguridad: BLAKE */
                sph_blake512_init(&ctx_blake);
                sph_blake512(&ctx_blake, cur, 64);
                sph_blake512_close(&ctx_blake, nxt);
                break;
        }

        cur = nxt;
    }

    /* trim256: primeros 32 bytes del último uint512 */
    memcpy(output, cur, 32);
}

/* ============================================================
 * scanhash_x11r: igual que scanhash_x11 pero llamando a x11rhash
 * ============================================================ */

int scanhash_x11r(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done)
{
    uint32_t _ALIGN(128) hash[8];
    uint32_t _ALIGN(128) endiandata[20];
    uint32_t *pdata   = work->data;
    uint32_t *ptarget = work->target;

    const uint32_t Htarg = ptarget[7];
    const uint32_t first_nonce = pdata[19];
    uint32_t nonce = first_nonce;
    volatile uint8_t *restart = &(work_restart[thr_id].restart);
    int k;

    if (opt_benchmark)
        ptarget[7] = 0x0cff;

    /* header a big-endian como en x11 */
    for (k = 0; k < 19; k++)
        be32enc(&endiandata[k], pdata[k]);

    do {
        be32enc(&endiandata[19], nonce);
        x11rhash(hash, endiandata);

        if (hash[7] <= Htarg && fulltest(hash, ptarget)) {
            work_set_target_ratio(work, hash);
            pdata[19] = nonce;
            *hashes_done = pdata[19] - first_nonce;
            return 1;
        }
        nonce++;

    } while (nonce < max_nonce && !(*restart));

    pdata[19] = nonce;
    *hashes_done = pdata[19] - first_nonce + 1;
    return 0;
}
