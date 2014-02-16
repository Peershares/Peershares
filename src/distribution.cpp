#include "distribution.h"

const int64 PEERCOIN_TX_FEE = 10000;

void DividendDistributor::Distribute(int64 nDistributedAmount)
{
    BalanceMap mapRetainedBalance(mapBalance);
    bool bMustRedistribute = true;

    while (bMustRedistribute)
    {
        bMustRedistribute = false;

        nTotalDistributed = 0;
        nTotalFee = 0;
        vDistribution.clear();

        BalanceMap::iterator it;
        int64 nTotalBalance = 0;

        it = mapRetainedBalance.begin();
        while (it != mapRetainedBalance.end())
        {
            nTotalBalance += it->second;
            it++;
        }

        it = mapRetainedBalance.begin();
        while (it != mapRetainedBalance.end())
        {
            int64 nFee = PEERCOIN_TX_FEE;
            int64 nDistributed = it->second * nDistributedAmount / nTotalBalance - nFee;
            if (nDistributed <= 0)
            {
                mapRetainedBalance.erase(it++);
                bMustRedistribute = true;
                continue;
            }
            Distribution distribution(it->first, it->second, nDistributed, nFee);
            vDistribution.push_back(distribution);

            nTotalDistributed += nDistributed;
            nTotalFee += nFee;

            it++;
        }
    }
}


