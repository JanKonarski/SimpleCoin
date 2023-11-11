#include <termios.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <iostream>
#include <string>
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/gcm.h>
#include <cryptopp/sha.h>
#include <cryptopp/secblock.h>
#include <cryptopp/pwdbased.h>

#include <iostream>
#include <string>
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/gcm.h>
#include <cryptopp/osrng.h>
#include <cryptopp/secblock.h>
#include <cryptopp/pwdbased.h>

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



// Function to generate a random IV
CryptoPP::SecByteBlock GenerateIV(size_t size) {
    CryptoPP::SecByteBlock iv(size);
    CryptoPP::AutoSeededRandomPool rnd;
    rnd.GenerateBlock(iv, size);
    return iv;
}

bool encryptPrivateKey(const std::string &privateKey, const std::string &password, std::string &encryptedPrivateKey, CryptoPP::SecByteBlock &iv)
{
    using namespace CryptoPP;

    try
    {
        SecByteBlock key(SHA256::DIGESTSIZE);
        iv = GenerateIV(AES::BLOCKSIZE);

        PKCS5_PBKDF2_HMAC<SHA256> pbkdf2;
        pbkdf2.DeriveKey(key, key.size(), 0, reinterpret_cast<const byte *>(password.data()), password.size(), nullptr, 0, 1000);

        GCM<AES>::Encryption encryption;
        encryption.SetKeyWithIV(key, key.size(), iv, iv.size());

        StringSource ss(privateKey, true,
            new AuthenticatedEncryptionFilter(encryption,
                new StringSink(encryptedPrivateKey)
            )
        );
        encryptedPrivateKey = encryptedPrivateKey + ":" + std::string(iv.begin(), iv.end());
        return true;
    }
    catch (const Exception &e)
    {
        std::cerr << "Crypto++ Error: " << e.what() << std::endl;
        return false;
    }
}

bool decryptPrivateKey(const std::string &encryptedPrivateKey, const std::string &password, std::string &privateKey)
{
    using namespace CryptoPP;

    try
    {
        std::string savedData = encryptedPrivateKey;
        size_t delimiterPos = savedData.find(":");
        std::string encryptedPrivateKey = savedData.substr(0, delimiterPos);
        std::string savedIV = savedData.substr(delimiterPos + 1);
        CryptoPP::SecByteBlock iv(reinterpret_cast<const byte*>(savedIV.data()), savedIV.size());

        SecByteBlock key(SHA256::DIGESTSIZE);

        PKCS5_PBKDF2_HMAC<SHA256> pbkdf2;
        pbkdf2.DeriveKey(key, key.size(), 0, reinterpret_cast<const byte *>(password.data()), password.size(), nullptr, 0, 1000);

        GCM<AES>::Decryption decryption;
        decryption.SetKeyWithIV(key, key.size(), iv, iv.size());

        StringSource ss(encryptedPrivateKey, true,
            new AuthenticatedDecryptionFilter(decryption,
                new StringSink(privateKey)
            )
        );

        return true;
    }
    catch (const Exception &e)
    {
        std::cerr << "Crypto++ Error: " << e.what() << std::endl;
        return false;
    }
}
