#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <sstream>

struct TransactionOutput
{
    double value;
    std::string scriptPubKey;
};
struct TransactionInput
{
    double value;
    std::string scriptSig;
};

// single input, one or more outputs
class Transaction
{
private:
    const double fee = 0.1;
    bool isFinalized = false;
    bool isAdjusted = false;

    void adjustOutputsForFee()
    {
        isAdjusted = true;
        if (vout.empty())
        {
            std::cerr << "No outputs to adjust for fee." << std::endl;
            return;
        }

        double feePerOutput = fee / vout.size();

        for (auto &output : vout)
        {
            if (output.value > feePerOutput)
            {
                output.value -= feePerOutput;
            }
            else
            {
                std::cerr << "Output value is too small to cover the fee portion." << std::endl;
                return;
            }
        }
    }

public:
    std::string txid;
    TransactionInput vin;
    std::vector<TransactionOutput> vout;
    Transaction(const std::string &tx_id, double inputValue, const std::string &inputScript)
        : txid(tx_id), vin{inputValue, inputScript} {}

    void addOutput(const double value, const std::string &scriptPubKey)
    {
        TransactionOutput newOutput{value, scriptPubKey};
        vout.push_back(newOutput);
    }

    bool validate()
    {
        double totalOutputValue = 0.0;
        for (const auto &output : vout)
        {
            totalOutputValue += output.value;
        }

        // Check if the input amount is enough to cover the outputs and the fee
        if (vin.value < totalOutputValue + fee)
        {
            std::cerr << "Invalid transaction: Input amount is insufficient." << std::endl;
            return false;
        }

        // Transaction is valid
        return true;
    }

    void finalize_transaction()
    {
        if (isFinalized)
        {
            std::cerr << "Transaction is already finalized." << std::endl;
            return;
        }

        adjustOutputsForFee();
        isFinalized = true;
    }

    std::string toString()
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(4);
        ss << "Transaction ID: " << txid << "\n";
        ss << "Input: " << vin.value << " (" << vin.scriptSig << ")\n";
        ss << "Outputs:\n";
        for (const auto &output : vout)
        {
            ss << " - " << output.value << " (" << output.scriptPubKey << ")\n";
        }
        return ss.str();
    }
};
