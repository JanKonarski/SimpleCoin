#include <termios.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <openssl/evp.h>

std::string getPassword()
{
    struct termios oldt, newt;
    std::string password;

    // Disable terminal echo
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // Input password
    std::cin.ignore();
    std::getline(std::cin, password);

    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    return password;
}

// Function to encrypt a private key using a password
bool encryptPrivateKey(const std::string &privateKey, const std::string &password, std::string &encryptedPrivateKey)
{
    // Initialize the OpenSSL library
    OpenSSL_add_all_algorithms();

    // Derive a key from the password
    const EVP_CIPHER *cipher = EVP_aes_256_cbc();
    const int keyLength = EVP_CIPHER_key_length(cipher);
    const int ivLength = EVP_CIPHER_iv_length(cipher);

    unsigned char key[keyLength];
    unsigned char iv[ivLength];

    if (EVP_BytesToKey(cipher, EVP_sha256(), nullptr,
                       reinterpret_cast<const unsigned char *>(password.c_str()), password.length(),
                       1, key, iv) != keyLength)
    {
        std::cerr << "Error deriving key." << std::endl;
        return false;
    }

    // Create an encryption context
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        std::cerr << "Error creating encryption context." << std::endl;
        return false;
    }

    // Initialize the encryption operation
    if (EVP_EncryptInit_ex(ctx, cipher, nullptr, key, iv) != 1)
    {
        std::cerr << "Error initializing encryption." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Perform the encryption
    int ciphertextLen = 0;
    int maxCiphertextLen = privateKey.length() + EVP_CIPHER_block_size(cipher);
    encryptedPrivateKey.resize(maxCiphertextLen);

    if (EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char *>(&encryptedPrivateKey[0]), &ciphertextLen,
                          reinterpret_cast<const unsigned char *>(privateKey.c_str()), privateKey.length()) != 1)
    {
        std::cerr << "Error performing encryption." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int finalCiphertextLen = 0;
    if (EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char *>(&encryptedPrivateKey[0]) + ciphertextLen, &finalCiphertextLen) != 1)
    {
        std::cerr << "Error finalizing encryption." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    ciphertextLen += finalCiphertextLen;
    encryptedPrivateKey.resize(ciphertextLen);

    EVP_CIPHER_CTX_free(ctx);
    return true;
}

// Function to decrypt a private key using a password
bool decryptPrivateKey(const std::string &encryptedPrivateKey, const std::string &password, std::string &privateKey)
{
    // Initialize the OpenSSL library
    OpenSSL_add_all_algorithms();

    // Derive a key from the password
    const EVP_CIPHER *cipher = EVP_aes_256_cbc();
    const int keyLength = EVP_CIPHER_key_length(cipher);
    const int ivLength = EVP_CIPHER_iv_length(cipher);

    unsigned char key[keyLength];
    unsigned char iv[ivLength];

    if (EVP_BytesToKey(cipher, EVP_sha256(), nullptr,
                       reinterpret_cast<const unsigned char *>(password.c_str()), password.length(),
                       1, key, iv) != keyLength)
    {
        std::cerr << "Error deriving key." << std::endl;
        return false;
    }

    // Create a decryption context
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        std::cerr << "Error creating decryption context." << std::endl;
        return false;
    }

    // Initialize the decryption operation
    if (EVP_DecryptInit_ex(ctx, cipher, nullptr, key, iv) != 1)
    {
        std::cerr << "Error initializing decryption." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Perform the decryption
    int plaintextLen = 0;
    int maxPlaintextLen = encryptedPrivateKey.length() + EVP_CIPHER_block_size(cipher);
    privateKey.resize(maxPlaintextLen);

    if (EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char *>(&privateKey[0]), &plaintextLen,
                          reinterpret_cast<const unsigned char *>(encryptedPrivateKey.c_str()), encryptedPrivateKey.length()) != 1)
    {
        std::cerr << "Error performing decryption." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int finalPlaintextLen = 0;
    if (EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char *>(&privateKey[0]) + plaintextLen, &finalPlaintextLen) != 1)
    {
        std::cerr << "Error finalizing decryption." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    plaintextLen += finalPlaintextLen;
    privateKey.resize(plaintextLen);

    EVP_CIPHER_CTX_free(ctx);
    return true;
}