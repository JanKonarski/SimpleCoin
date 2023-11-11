#include <string>
#include "digital_identity.hpp"
#include "utils.hpp"
#include <fstream>

CryptoPP::SecByteBlock iv;
int menu()
{
    int choice;
    std::cout << "1. Create a new wallet" << std::endl;
    std::cout << "2. Load an existing wallet" << std::endl;
    std::cout << "3. Exit" << std::endl;
    std::cout << "Enter your choice: ";
    std::cin >> choice;
    return choice;
}

int create_wallet()
{
    // Create a DigitalIdentity instance with a wordlist file
    DigitalIdentity digitalIdentity("wordlist.txt");

    // Select 12 random words from the wordlist
    if (!digitalIdentity.SelectRandomWords(12)) {
        std::cerr << "Error selecting random words." << std::endl;
        return 1;
    }

    // Print the selected seed words for the user to write down
    std::cout << "Selected Seed Words (Write these down securely):\n";
    const std::vector<std::string>& selectedWords = digitalIdentity.GetSelectedWords();
    for (const std::string& word : selectedWords) {
        std::cout << word << " ";
    }
    std::cout << std::endl;

    // Derive a seed from the chosen words
    if (!digitalIdentity.DeriveSeedFromWords()) {
        std::cerr << "Error deriving seed from words." << std::endl;
        return 1;
    }

    // Generate an ECDSA key pair from the seed
    if (!digitalIdentity.GenerateECDSAKeyPair()) {
        std::cerr << "Error generating ECDSA key pair." << std::endl;
        return 1;
    }

    // Get the private key
    const std::string& privateKey = digitalIdentity.GetPrivateKey();

    std::string encryptedPrivateKey;
    std::string password;
    std::cout << "Enter a password to encrypt the private key: ";
    password = getPassword();
    if(encryptPrivateKey(privateKey, password, encryptedPrivateKey, iv))
    {
    // Save the encrypted private key to a file
    std::ofstream outputFile("encrypted_private_key.bin", std::ios::binary);
    if (!outputFile) {
        std::cerr << "Error opening output file." << std::endl;
        return 1;
    }
    outputFile.write(encryptedPrivateKey.c_str(), encryptedPrivateKey.size());
    outputFile.close();

    std::cout << "Wallet created successfully. Encrypted private key saved to 'encrypted_private_key.bin'." << std::endl;
    }
    else
    {
        std::cout << "Error encrypting private key" << std::endl;
    };

    return 0;
}
int load_wallet()
{
    // Read the encrypted data from the file
    std::ifstream inputFile("encrypted_private_key.bin", std::ios::binary);
    if (!inputFile) {
        std::cerr << "Error opening input file." << std::endl;
        return 1;
    }

    std::string encryptedData((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();
    std::cout << "Enter a password to decrypt the private key: ";
    std::string password = getPassword();
    std::cout << std::endl;
    std::string decryptedData;

    // Decrypt the encrypted data
    if (!decryptPrivateKey(encryptedData, password, decryptedData)) {
        std::cerr << "Error decrypting data." << std::endl;
        return 1;
    }
    std::cout << decryptedData << std::endl;
    return 0;
}

int main()
{
    switch(menu())
    {
        case 1:
            create_wallet();
            break;
        case 2:
            load_wallet();
            break;
        default:
            break;
    }

}