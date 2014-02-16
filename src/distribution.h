#include "base58.h"

class Distribution
{
protected:
    CBitcoinAddress addrPeershares;
    int64 nBalance;

    CBitcoinAddress addrPeercoin;
    int64 nDividendAmount;

    int64 nTransactionFee;

public:
    Distribution(CBitcoinAddress addrPeershares, int64 nBalance, int64 nDividendAmount, int64 nTransactionFee)
        : addrPeershares(addrPeershares), nBalance(nBalance), addrPeercoin(addrPeershares), nDividendAmount(nDividendAmount), nTransactionFee(nTransactionFee)
    {
    }

    const CBitcoinAddress &GetPeershareAddress() const
    {
        return addrPeershares;
    }

    int64 GetBalance() const
    {
        return nBalance;
    }

    const CBitcoinAddress &GetPeercoinAddress() const
    {
        return addrPeercoin;
    }

    int64 GetDividendAmount() const
    {
        return nDividendAmount;
    }

    int64 GetTransactionFee() const
    {
        return nTransactionFee;
    }
};

typedef std::map<const CBitcoinAddress, int64> BalanceMap;
typedef std::vector<Distribution> DistributionVector;

extern const int64 PEERCOIN_TX_FEE;

class DividendDistributor
{
protected:
    const BalanceMap& mapBalance;
    int64 nTotalDistributed;
    int64 nTotalFee;

    DistributionVector vDistribution;

public:
    DividendDistributor(const BalanceMap& mapBalance) : mapBalance(mapBalance), nTotalDistributed(0), nTotalFee(0)
    {
    }

    void Distribute(int64 nDistributedAmount);

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

    int64 TotalDistributed() const
    {
        return nTotalDistributed;
    }

    int64 TotalFee() const
    {
        return nTotalFee;
    }
};
