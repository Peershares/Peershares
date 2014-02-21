#include "base58.h"
#include "json/json_spirit_value.h"

class Distribution
{
protected:
    CBitcoinAddress addrPeershares;
    double nBalance;

    CBitcoinAddress addrPeercoin;
    double nDividendAmount;

public:
    Distribution(CBitcoinAddress addrPeershares, double nBalance, double nDividendAmount)
        : addrPeershares(addrPeershares), nBalance(nBalance), addrPeercoin(addrPeershares), nDividendAmount(nDividendAmount)
    {
    }

    const CBitcoinAddress &GetPeershareAddress() const
    {
        return addrPeershares;
    }

    double GetBalance() const
    {
        return nBalance;
    }

    const CBitcoinAddress &GetPeercoinAddress() const
    {
        return addrPeercoin;
    }

    double GetDividendAmount() const
    {
        return nDividendAmount;
    }
};

typedef std::map<const CBitcoinAddress, int64> BalanceMap;
typedef std::vector<Distribution> DistributionVector;

class DividendDistributor
{
protected:
    const BalanceMap& mapBalance;
    double nTotalDistributed;

    DistributionVector vDistribution;

public:
    DividendDistributor(const BalanceMap& mapBalance) : mapBalance(mapBalance), nTotalDistributed(0)
    {
    }

    void Distribute(double nDistributedAmount, double nMinimumPayout);
    void GenerateOutputs(int nTransactions, std::vector<json_spirit::Object> &vTransactionOuts);

    const DistributionVector& GetDistributions() const
    {
        return vDistribution;
    }

    const Distribution& GetDistribution(const CBitcoinAddress& addrPeershare) const
    {
        for (DistributionVector::const_iterator it = vDistribution.begin(); it != vDistribution.end(); ++it)
        {
            if (it->GetPeershareAddress() == addrPeershare)
                return *it;
        }
        throw std::runtime_error("Distribution not found");
    }

    double TotalDistributed() const
    {
        return nTotalDistributed;
    }
};
