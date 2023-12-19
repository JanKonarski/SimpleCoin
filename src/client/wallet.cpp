#include <fstream>
#include <cryptopp/randpool.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/ecp.h>
#include <cryptopp/hex.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include "wallet.hpp"

Wallet::Wallet(const std::string &wordlistFileName)
{
    LoadWordList(wordlistFileName);
}

Wallet::~Wallet()
{
}

bool Wallet::LoadWordList(const std::string &filename)
{
    std::ifstream file(filename);

    if (!file)
    {
        return false;
    }

    std::string word;
    while (std::getline(file, word))
    {
        wordlist_.push_back(word);
    }

    return true;
}

bool Wallet::SelectRandomWords(size_t count)
{
    if (count > wordlist_.size())
    {
        return false;
    }

    CryptoPP::AutoSeededRandomPool rng;
    selectedWords_.resize(count);

    for (size_t i = 0; i < count; ++i)
    {
        size_t randomIndex = rng.GenerateWord32(0, wordlist_.size() - 1);
        selectedWords_[i] = wordlist_[randomIndex];
    }

    return true;
}

bool Wallet::DeriveSeedFromWords()
{
    std::string joinedWords;
    for (const std::string &word : selectedWords_)
    {
        joinedWords += word;
    }

    CryptoPP::SHA256 sha256;
    seed_.resize(sha256.DigestSize());
    sha256.CalculateDigest(seed_, reinterpret_cast<const CryptoPP::byte *>(joinedWords.data()), joinedWords.size());

    return true;
}

bool Wallet::GenerateECDSAKeyPair()
{
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256> ecdsa;

    CryptoPP::AutoSeededRandomPool rng;
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey privateKey;
    privateKey.Initialize(rng, CryptoPP::ASN1::secp256r1());

    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PublicKey publicKey;
    privateKey.MakePublicKey(publicKey);

    CryptoPP::ByteQueue queue;
    privateKey.Save(queue);
    privateKey_.resize(queue.CurrentSize());
    queue.Get(reinterpret_cast<CryptoPP::byte *>(&privateKey_[0]), privateKey_.size());

    queue.Clear();
    publicKey.Save(queue);
    publicKey_.resize(queue.CurrentSize());
    queue.Get(reinterpret_cast<CryptoPP::byte *>(&publicKey_[0]), publicKey_.size());

    return true;
}
void Wallet::sendTransaction(const Transaction &transaction)
{
    std::cout << "Transaction sent: " << transaction.toJson() << std::endl;
}

Transaction Wallet::sendToken(Transaction transaction)
{
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey privateKey;
    CryptoPP::ByteQueue queue;
    queue.Put(reinterpret_cast<const CryptoPP::byte *>(privateKey_.data()), privateKey_.size());
    queue.MessageEnd();
    privateKey.Load(queue);

    transaction.sign(privateKey);

    sendTransaction(transaction);
    return transaction;
}

double Wallet::CalculateBalance(const std::vector<std::vector<Transaction>> &transactions)
{
    double balance = 0.0;

    for (const auto &transactionBlock : transactions)
    {
        for (const auto &transaction : transactionBlock)
        {
            // Add the amounts from outputs that match the pubkey
            for (const auto &output : transaction.outputs)
            {
                if (output.pubkey == this->publicKey_)
                {
                    balance += output.amount;
                }
            }

            // Subtract the amount from input if it matches the pubkey
            if (transaction.input.pubkey == this->publicKey_)
            {
                balance -= transaction.input.amount;
            }
        }
    }
    this->balance_[std::time(nullptr)] = balance;
    return balance;
}

const std::string &Wallet::GetPrivateKey() const
{
    return privateKey_;
}

const std::string &Wallet::GetPublicKey() const
{
    return publicKey_;
}

int main()
{
    // Step 1: Create a Wallet instance
    Wallet myWallet("wordlist.txt"); // Replace with actual wordlist file name or modify the constructor

    // Step 2: Generate ECDSA key pair
    if (!myWallet.GenerateECDSAKeyPair())
    {
        std::cerr << "Failed to generate ECDSA key pair." << std::endl;
        return 1;
    }

    // Sample transaction data
    Transaction::Input input = {"publicKey1", 100.0};
    std::vector<Transaction::Output> outputs = {
        {"publicKey2", 50.0},
        {"publicKey3", 50.0}};

    // Step 3 and 4: Create and sign the transaction
    // Transaction transaction = Transaction(input, outputs);
    std::string transaction_json = "{\"input\": {\"pubkey\": \"publicKey1\",\"amount\": 100.000000},\"outputs\": [{\"pubkey\": \"publicKey2\",\"amount\": 50.000000},{\"pubkey\": \"publicKey3\",\"amount\": 50.000000}],\"timestamp\": 1702999667,\"signature\": \"94C3CB32FEA49E93088E74B7C3CE4B3D08C8F7672018D46EA0BC1FBECB68CD8B73AB8FAFD6A18107028A0E8FBBF5806684E2E26EA10D87895B83D12C4AB4CC24\"}";
    Transaction transaction(transaction_json);
    // Step 5: Dump transaction to JSON
    std::string json = transaction.toJson();
    std::ofstream jsonFile("transaction.json");
    if (jsonFile.is_open())
    {
        jsonFile << json;
        jsonFile.close();
        std::cout << "Transaction JSON has been written to 'transaction.json'" << std::endl;
    }
    else
    {
        std::cerr << "Unable to open file for writing." << std::endl;
    }

    return 0;
}