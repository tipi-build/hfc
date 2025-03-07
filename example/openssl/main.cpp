#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <openssl/evp.h>

std::vector<unsigned char> compute_sha256(const std::string &input) {
  std::vector<unsigned char> hash(EVP_MAX_MD_SIZE);
  unsigned int hash_length = 0;

  // Get the digest method for SHA-256
  const EVP_MD *md = EVP_sha256();
  if (!md)
  {
    throw std::runtime_error("Error getting SHA-256 digest method");
  }

  // Create and initialize the digest context
  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  if (!mdctx)
  {
    throw std::runtime_error("Error creating digest context");
  }

  if (EVP_DigestInit_ex(mdctx, md, nullptr) != 1 ||
      EVP_DigestUpdate(mdctx, input.data(), input.size()) != 1 ||
      EVP_DigestFinal_ex(mdctx, hash.data(), &hash_length) != 1)
  {
    EVP_MD_CTX_free(mdctx);
    throw std::runtime_error("Error computing SHA-256 hash");
  }

  EVP_MD_CTX_free(mdctx);

  // Resize to the actual hash length (SHA-256 produces 32 bytes)
  hash.resize(hash_length);
  return hash;
}

void print_hex(const std::vector<unsigned char> &data) {
  for (unsigned char byte : data) {
    std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
  }
  std::cout << std::endl;
}

int main()
{
  try
  {
    std::string message = "Hello OpenSSL!";
    std::cout << "Input message: " << message << std::endl;

    std::vector<unsigned char> hash = compute_sha256(message);

    std::cout << "SHA-256 hash: ";
    print_hex(hash);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
