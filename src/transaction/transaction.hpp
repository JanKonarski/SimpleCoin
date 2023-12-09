#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

class TransactionInput
{
public:
    TransactionInput(std::string prevTxId, int vout, std::string scriptSig)
        : prevTxId(prevTxId), vout(vout), scriptSig(scriptSig) {}

    std::string getPrevTxId() const
    {
        return prevTxId;
    }

    int getVout() const
    {
        return vout;
    }

    std::string getScriptSig() const
    {
        return scriptSig;
    }

private:
    std::string prevTxId;
    int vout;
    std::string scriptSig;
};

class TransactionOutput
{
public:
    TransactionOutput(double value, std::string scriptPubKey)
        : value(value), scriptPubKey(scriptPubKey) {}

    double getValue() const
    {
        return value;
    }

    std::string getScriptPubKey() const
    {
        return scriptPubKey;
    }

private:
    double value;
    std::string scriptPubKey;
};

class Transaction
{
public:
    Transaction(std::vector<TransactionInput> inputs, std::vector<TransactionOutput> outputs)
        : inputs(inputs), outputs(outputs) {}

    std::vector<TransactionInput> getInputs() const
    {
        return inputs;
    }

    std::vector<TransactionOutput> getOutputs() const
    {
        return outputs;
    }

    double calculateTotalInputValue() const
    {
        double totalInputValue = 0.0;
        for (const auto &input : inputs)
        {
            throw std::runtime_error("Not implemented");
        }
        return totalInputValue;
    }

    double calculateTotalOutputValue() const
    {
        double totalOutputValue = 0.0;
        for (const auto &output : outputs)
        {
            totalOutputValue += output.getValue();
        }
        return totalOutputValue;
    }

private:
    std::vector<TransactionInput> inputs;
    std::vector<TransactionOutput> outputs;
};

Transaction parseTransactionFromJson(const std::string &jsonFilePath)
{
    std::ifstream file(jsonFilePath);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open JSON file.");
    }

    json jsonData;
    file >> jsonData;

    std::vector<TransactionInput> inputs;
    std::vector<TransactionOutput> outputs;

    for (const auto &input : jsonData["inputs"])
    {
        std::string prevTxId = input["prevTxId"];
        int vout = input["vout"];
        std::string scriptSig = input["scriptSig"];
        inputs.emplace_back(prevTxId, vout, scriptSig);
    }

    for (const auto &output : jsonData["outputs"])
    {
        double value = output["value"];
        std::string scriptPubKey = output["scriptPubKey"];
        outputs.emplace_back(value, scriptPubKey);
    }

    return Transaction(inputs, outputs);
}
