#ifndef STRING_OBFUSCATOR_HXX
#define STRING_OBFUSCATOR_HXX

#include <array>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <memory>

class IHashAlgorithm {
public:
    virtual ~IHashAlgorithm() = default;
    virtual void update(const unsigned char* message, size_t len) = 0;
    virtual void finalize(std::array<unsigned char, 64>& digest) = 0;
    virtual std::string hash(const std::string& input) = 0;
};

class SHA512 : public IHashAlgorithm {
public:
    static constexpr size_t DIGEST_SIZE = 64;
    static constexpr size_t BLOCK_SIZE = 128;
    static constexpr size_t STATE_SIZE = 8;

    SHA512() {
        init();
    }

    void update(const unsigned char* message, size_t len) override {
        size_t block_nb, new_len, rem_len, tmp_len;
        const unsigned char* shifted_message;

        tmp_len = BLOCK_SIZE - m_len;
        rem_len = len < tmp_len ? len : tmp_len;

        std::memcpy(&m_block[m_len], message, rem_len);

        if (m_len + len < BLOCK_SIZE) {
            m_len += len;
            return;
        }

        new_len = len - rem_len;
        block_nb = new_len / BLOCK_SIZE;
        shifted_message = message + rem_len;

        transform(m_block.data(), 1);
        transform(shifted_message, block_nb);

        rem_len = new_len % BLOCK_SIZE;
        std::memcpy(m_block.data(), &shifted_message[block_nb * BLOCK_SIZE], rem_len);

        m_len = rem_len;
        m_tot_len += (block_nb + 1) * BLOCK_SIZE;
    }

    void finalize(std::array<unsigned char, DIGEST_SIZE>& digest) override {
        size_t block_nb, pm_len, len_b;

        block_nb = 1 + ((BLOCK_SIZE - 17) < (m_len % BLOCK_SIZE));
        len_b = (m_tot_len + m_len) * 8;
        pm_len = block_nb * BLOCK_SIZE;

        std::memset(m_block.data() + m_len, 0, pm_len - m_len);
        m_block[m_len] = 0x80;
        unpack32(static_cast<uint32_t>(len_b), m_block.data() + pm_len - 4);

        transform(m_block.data(), block_nb);

        for (size_t i = 0; i < STATE_SIZE; i++) {
            unpack64(reinterpret_cast<const unsigned char*>(&m_h[i]), reinterpret_cast<uint64_t*>(&digest[i * 8]));
        }
    }

    std::string hash(const std::string& input) override {
        std::array<unsigned char, DIGEST_SIZE> digest;
        digest.fill(0);

        update(reinterpret_cast<const unsigned char*>(input.c_str()), input.length());
        finalize(digest);

        std::ostringstream oss;
        for (const auto& byte : digest) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }

        return oss.str();
    }

private:
    std::array<uint64_t, STATE_SIZE> m_h;
    std::array<unsigned char, BLOCK_SIZE> m_block;
    size_t m_len;
    size_t m_tot_len;

    static constexpr std::array<uint64_t, 80> k = {
        0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
        0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
        0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
        0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
        0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
        0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
        0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
        0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
        0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
        0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
        0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
        0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
        0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
        0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
        0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
        0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
        0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
        0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
        0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
        0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
    };

    void init() {
        m_h = {
            0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
            0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL, 0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
        };
        m_len = 0;
        m_tot_len = 0;
    }

    void transform(const unsigned char* message, size_t block_nb) {
        uint64_t w[80], wv[8], t1, t2;

        for (size_t i = 0; i < block_nb; i++) {
            const unsigned char* sub_block = message + (i * BLOCK_SIZE);

            for (size_t j = 0; j < 16; j++) {
                unpack64(sub_block + j * 8, &w[j]);
            }

            for (size_t j = 16; j < 80; j++) {
                w[j] = SHA512_F4(w[j - 2]) + w[j - 7] + SHA512_F3(w[j - 15]) + w[j - 16];
            }

            for (size_t j = 0; j < STATE_SIZE; j++) {
                wv[j] = m_h[j];
            }

            for (size_t j = 0; j < 80; j++) {
                t1 = wv[7] + SHA512_F2(wv[4]) + SHA2_CH(wv[4], wv[5], wv[6]) + k[j] + w[j];
                t2 = SHA512_F1(wv[0]) + SHA2_MAJ(wv[0], wv[1], wv[2]);
                wv[7] = wv[6];
                wv[6] = wv[5];
                wv[5] = wv[4];
                wv[4] = wv[3] + t1;
                wv[3] = wv[2];
                wv[2] = wv[1];
                wv[1] = wv[0];
                wv[0] = t1 + t2;
            }

            for (size_t j = 0; j < STATE_SIZE; j++) {
                m_h[j] += wv[j];
            }
        }
    }

    static uint64_t SHA512_F1(uint64_t x) { return (ROTR(x, 28) ^ ROTR(x, 34) ^ ROTR(x, 39)); }
    static uint64_t SHA512_F2(uint64_t x) { return (ROTR(x, 14) ^ ROTR(x, 18) ^ ROTR(x, 41)); }
    static uint64_t SHA512_F3(uint64_t x) { return (ROTR(x, 1) ^ ROTR(x, 8) ^ (x >> 7)); }
    static uint64_t SHA512_F4(uint64_t x) { return (ROTR(x, 19) ^ ROTR(x, 61) ^ (x >> 6)); }
    static uint64_t SHA2_CH(uint64_t x, uint64_t y, uint64_t z) { return ((x & y) ^ (~x & z)); }
    static uint64_t SHA2_MAJ(uint64_t x, uint64_t y, uint64_t z) { return ((x & y) ^ (x & z) ^ (y & z)); }

    static uint64_t ROTR(uint64_t x, uint64_t n) { return ((x >> n) | (x << (64 - n))); }

    void unpack64(const unsigned char* input, uint64_t* output) {
        *output = 0;
        for (size_t i = 0; i < 8; ++i) {
            *output |= (static_cast<uint64_t>(input[i]) << ((7 - i) * 8));
        }
    }

    void unpack32(uint32_t val, unsigned char* buf) {
        for (size_t i = 0; i < 4; ++i) {
            buf[i] = static_cast<unsigned char>((val >> ((3 - i) * 8)) & 0xff);
        }
    }
};

class IStringObfuscator {
public:
    virtual ~IStringObfuscator() = default;
    virtual std::string obfuscate(const std::string& input) = 0;
    virtual std::string dynamicObfuscate(const std::string& input) = 0;
};

class StringObfuscator : public IStringObfuscator {
public:
    StringObfuscator() {
        setDefaultHashAlgorithm();
    }
    std::string obfuscate(const std::string& input) override {
        if (!shaAlgorithm) {
            throw std::runtime_error("Hash algorithm not set.");
        }
        return shaAlgorithm->hash(input);
    }
    std::string dynamicObfuscate(const std::string& input) override {
        std::string processedString = preProcess(input);
        std::string obfuscated = obfuscate(processedString);
        return obfuscated;
    }
    void setHashAlgorithm(std::unique_ptr<IHashAlgorithm> sha) {
        shaAlgorithm = std::move(sha);
    }

private:
    void setDefaultHashAlgorithm() {
        shaAlgorithm = std::make_unique<SHA512>();
    }
    std::string preProcess(const std::string& input) {
        std::string processed = input;
        for (size_t i = 0; i < processed.length(); ++i) {
            processed[i] = processed[i] ^ 0x5A;
        }
        return processed;
    }
    std::unique_ptr<IHashAlgorithm> shaAlgorithm;
};

#endif