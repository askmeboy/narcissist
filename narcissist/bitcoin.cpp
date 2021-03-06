#include <cstdint>
#include <string>

#include <secp256k1.h>
#include <cryptopp/sha.h>
#include <cryptopp/ripemd.h>

#include <narcissist/bitcoin.hpp>

#include <narcissist/base58.h>
#include <narcissist/bech32.h>
#include <narcissist/narcissist.hpp>

static CryptoPP::SHA256 sha256;
static CryptoPP::RIPEMD160 ripemd160;

static inline void hash160(uint8_t *out, const uint8_t *in, size_t in_length)
{
	// ripemd160(sha256(x))

	uint8_t sha256digest[CryptoPP::SHA256::DIGESTSIZE];
	sha256.CalculateDigest(sha256digest, in, in_length);
	ripemd160.CalculateDigest(out, sha256digest, CryptoPP::SHA256::DIGESTSIZE);
}

static inline void sha256d(uint8_t *out, const uint8_t *in, size_t in_length)
{
	// sha256(sha256(x))

	sha256.CalculateDigest(out, in, in_length);
	sha256.CalculateDigest(out, out, CryptoPP::SHA256::DIGESTSIZE);
}

void Narcissist::derive_p2pkh(secp256k1_pubkey *pubkey, char *address,
	size_t *address_length, char network = 0x00)
{
	// serialize address
	uint8_t serialized_pubkey[33];
	size_t serialized_pubkey_length = 33;
	secp256k1_ec_pubkey_serialize(secp256k1ctx, serialized_pubkey,
		&serialized_pubkey_length, pubkey, SECP256K1_EC_COMPRESSED);

	// only first four bytes of this are used
	uint8_t checksum[CryptoPP::SHA256::DIGESTSIZE];

	// network + hash160 + 4 bytes of sha256d
	uint8_t addr[CryptoPP::RIPEMD160::DIGESTSIZE + 5] = { 0 };
	hash160(addr + 1, serialized_pubkey, serialized_pubkey_length);
	addr[0] = network;

	// hash of the network byte and the hash160
	sha256d(checksum, addr, CryptoPP::RIPEMD160::DIGESTSIZE + 1);

	// append first four bytes of checksum to end of address
	memcpy(addr + CryptoPP::RIPEMD160::DIGESTSIZE + 1, checksum, 4);

	// base58 encoding
	base58enc(address, address_length, addr,
		CryptoPP::RIPEMD160::DIGESTSIZE + 5);
}

void Narcissist::derive_bech32(secp256k1_pubkey *pubkey, char *address,
	size_t *address_length, bool testnet)
{
	// serialize address
	uint8_t serialized_pubkey[33];
	size_t serialized_pubkey_length = 33;
	secp256k1_ec_pubkey_serialize(secp256k1ctx, serialized_pubkey,
		&serialized_pubkey_length, pubkey, SECP256K1_EC_COMPRESSED);

	// only first four bytes of this are used
	uint8_t checksum[CryptoPP::SHA256::DIGESTSIZE];

	// network + hash160 + 4 bytes of sha256d
	uint8_t addr[CryptoPP::RIPEMD160::DIGESTSIZE] = { 0 };
	hash160(addr, serialized_pubkey, serialized_pubkey_length);

	segwit_addr_encode(address, testnet ? "tc" : "bc", 1, addr, CryptoPP::RIPEMD160::DIGESTSIZE);
}
