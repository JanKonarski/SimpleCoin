#include <boost/json.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

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

    std::string toString() const
    {
        std::string result = "Transaction:\n";

        result += "Inputs:\n";
        for (const auto &input : inputs)
        {
            result += "PrevTxId: " + input.getPrevTxId() + ", Vout: " + std::to_string(input.getVout()) + ", ScriptSig: " + input.getScriptSig() + "\n";
        }

        result += "Outputs:\n";
        for (const auto &output : outputs)
        {
            result += "Value: " + std::to_string(output.getValue()) + ", ScriptPubKey: " + output.getScriptPubKey() + "\n";
        }

        return result;
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

    std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    boost::json::value jv = boost::json::parse(str);
    boost::json::object jsonData = jv.as_object();

    std::vector<TransactionInput> inputs;
    std::vector<TransactionOutput> outputs;

    for (const auto &inputVal : jsonData["inputs"].as_array())
    {
        boost::json::object input = inputVal.as_object();
        std::string prevTxId = input["prevTxId"].as_string().c_str();
        int vout = input["vout"].as_int64();
        std::string scriptSig = input["scriptSig"].as_string().c_str();
        inputs.emplace_back(prevTxId, vout, scriptSig);
    }

    for (const auto &outputVal : jsonData["outputs"].as_array())
    {
        boost::json::object output = outputVal.as_object();
        double value = output["value"].as_double();
        std::string scriptPubKey = output["scriptPubKey"].as_string().c_str();
        outputs.emplace_back(value, scriptPubKey);
    }

    return Transaction(inputs, outputs);
}
