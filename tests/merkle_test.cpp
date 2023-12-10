#include "../src/blockchain/merkle.hpp"
#include <cassert>

int main()
{
    std::vector<std::string> transactionData = {"tx1", "tx2", "tx3", "tx4", "tx6", "tx7", "tx8"};
    MerkleTree merkleTree(transactionData);

    std::cout << "Merkle Tree before:" << std::endl;
    merkleTree.printTree();

    std::string rootHash = merkleTree.getRootHash();

    merkleTree.addTransaction("tx5");
    std::cout << "Merkle Tree after:" << std::endl;
    merkleTree.printTree();

    std::string newRootHash = merkleTree.getRootHash();

    assert(rootHash != newRootHash);

    // Test verification path
    try
    {
        std::vector<std::string> path = merkleTree.getVerificationPath("tx5");
        assert(!path.empty());
        std::cout << "Verification Path for tx5: ";
        for (const auto &hash : path)
        {
            std::cout << hash.substr(0, 8) << " ";
        }
        std::cout << std::endl;
    }
    catch (const std::runtime_error &e)
    {
        std::cout << e.what() << std::endl;
        assert(false); // Should not reach here
    }

    // Test for non-existing transaction
    try
    {
        std::vector<std::string> path = merkleTree.getVerificationPath("non_existent_tx");
        assert(false); // Should not reach here
    }
    catch (const std::runtime_error &e)
    {
        // Expected exception
    }

    MerkleTree merkleTree_empty;
    std::cout << "Merkle Tree before:" << std::endl;
    merkleTree_empty.printTree();
    merkleTree_empty.addTransaction("tx1");
    std::cout << "Merkle Tree after:" << std::endl;
    merkleTree_empty.printTree();

    // Test verification path
    try
    {
        std::vector<std::string> path = merkleTree_empty.getVerificationPath("tx1");
        assert(!path.empty());
        std::cout << "Verification Path for tx5: ";
        for (const auto &hash : path)
        {
            std::cout << hash.substr(0, 8) << " ";
        }
        std::cout << std::endl;
    }
    catch (const std::runtime_error &e)
    {
        std::cout << e.what() << std::endl;
        assert(false); // Should not reach here
    }

    // Test for non-existing transaction
    try
    {
        std::vector<std::string> path = merkleTree_empty.getVerificationPath("non_existent_tx");
        assert(false); // Should not reach here
    }
    catch (const std::runtime_error &e)
    {
        // Expected exception
    }

    return 0;
}
