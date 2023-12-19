//
// Created by Jan Konarski on 01/11/2023.
//

#ifndef SIMPLE_COIN_MERKLETREE_HPP
#define SIMPLE_COIN_MERKLETREE_HPP

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/sha.h>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <sstream>

class MerkleTree
{
private:
    struct Node
    {
        std::string data;
        Node *left;
        Node *right;
        Node *parent;
    };

    Node *root;
    std::vector<Node *> nodes;

    std::string calculateHash(std::string data)
    {
        std::string hash;
        CryptoPP::SHA256 sha256;

        CryptoPP::StringSource(
            data, true,
            new CryptoPP::HashFilter(
                sha256, new CryptoPP::HexEncoder(new CryptoPP::StringSink(hash))));

        return hash;
    }

    Node *newNode(std::string data)
    {
        Node *node = new Node;
        node->data = calculateHash(data);
        node->left = nullptr;
        node->right = nullptr;
        node->parent = nullptr;

        return node;
    }

    Node *constructMerkle(std::vector<Node *> nodes)
    {
        std::vector<Node *> newNodes;
        for (int i = 0; i < nodes.size(); i += 2)
        {
            std::string data1 = nodes[i]->data;
            std::string data2 = i + 1 < nodes.size() ? nodes[i + 1]->data : "";
            Node *node = newNode(data1 + data2);
            node->left = nodes[i];
            nodes[i]->parent = node; // Set parent of left child
            if (i + 1 < nodes.size())
            {
                node->right = nodes[i + 1];
                nodes[i + 1]->parent = node; // Set parent of right child
            }
            newNodes.push_back(node);
        }
        if (newNodes.size() == 1)
        {
            return newNodes[0];
        }
        return constructMerkle(newNodes);
    }

    void printTree(Node *node, int indent = 0)
    {
        if (node != nullptr)
        {
            if (node->right)
            {
                printTree(node->right, indent + 4);
            }
            if (indent)
            {
                std::cout << std::setw(indent) << ' ';
            }
            if (node->right)
                std::cout << " /\n"
                          << std::setw(indent) << ' ';
            std::cout << node->data.substr(0, 8) << "\n ";
            if (node->left)
            {
                std::cout << std::setw(indent) << ' ' << " \\\n";
                printTree(node->left, indent + 4);
            }
        }
    }

public:
    MerkleTree(std::vector<std::string> transactionData)
    {
        for (std::string data : transactionData)
        {
            auto x = newNode(data);
            nodes.push_back(x);
        }
        root = constructMerkle(nodes);
    }

    MerkleTree()
    {
        boost::random::mt19937 gen(static_cast<unsigned int>(std::time(0)));

        boost::random::uniform_int_distribution<> dist(0, 999999);

        std::ostringstream ss;
        ss << dist(gen);
        std::string randomData = ss.str();

        auto x = newNode(randomData);
        nodes.push_back(x);

        root = constructMerkle(nodes);
    }

    void addTransaction(std::string transactionData)
    {
        auto x = newNode(transactionData);
        nodes.push_back(x);
        root = constructMerkle(nodes);
    }
    std::vector<std::string> getVerificationPath(std::string transactionData)
    {
        Node *targetNode = findNode(root, calculateHash(transactionData));
        if (targetNode == nullptr)
        {
            throw std::runtime_error("Transaction not found in Merkle Tree");
        }
        return constructPath(targetNode);
    }
    Node *findNode(Node *node, std::string data)
    {
        if (node == nullptr)
        {
            return nullptr;
        }

        if (node->data == data)
        {
            return node;
        }

        Node *foundNode = findNode(node->left, data);
        if (foundNode == nullptr)
        {
            foundNode = findNode(node->right, data);
        }
        return foundNode;
    }

    std::vector<std::string> constructPath(Node *node)
    {
        std::vector<std::string> path;
        path.push_back(node->data);
        while (node != root)
        {
            Node *sibling = getSibling(node);
            path.push_back(sibling->data);
            node = getParent(node);
        }
        path.push_back(root->data);
        return path;
    }

    Node *getSibling(Node *node)
    {
        if (node == nullptr || node->parent == nullptr)
        {
            return nullptr;
        }
        if (node == node->parent->left)
        {
            return node->parent->right;
        }
        else
        {
            return node->parent->left;
        }
    }

    Node *getParent(Node *node)
    {
        return node != nullptr ? node->parent : nullptr;
    }

    std::string getRootHash() { return root->data; }

    void printTree() { printTree(root); }

    ~MerkleTree() { deleteTree(root); }

    void deleteTree(Node *node)
    {
        if (node)
        {
            deleteTree(node->left);
            deleteTree(node->right);
            delete node;
        }
    }
};

#endif // SIMPLE_COIN_MERKLETREE_HPP