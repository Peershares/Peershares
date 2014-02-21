#include "distribution.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_utils.h"
#include <boost/foreach.hpp>

using namespace std;
using namespace json_spirit;

void DividendDistributor::Distribute(double nDistributedAmount, double nMinimumPayout)
{
    BalanceMap mapRetainedBalance(mapBalance);
    bool bMustRedistribute = true;

    while (bMustRedistribute)
    {
        bMustRedistribute = false;

        nTotalDistributed = 0;
        vDistribution.clear();

        BalanceMap::iterator it;
        double nTotalBalance = 0;

        it = mapRetainedBalance.begin();
        while (it != mapRetainedBalance.end())
        {
            nTotalBalance += it->second;
            it++;
        }

        it = mapRetainedBalance.begin();
        while (it != mapRetainedBalance.end())
        {
            double nDistributed = it->second * nDistributedAmount / nTotalBalance;
            if (nDistributed < nMinimumPayout)
            {
                mapRetainedBalance.erase(it++);
                bMustRedistribute = true;
                continue;
            }
            Distribution distribution(it->first, it->second, nDistributed);
            vDistribution.push_back(distribution);

            nTotalDistributed += nDistributed;

            it++;
        }
    }
}

void DividendDistributor::GenerateOutputs(int nTransactions, vector<Object> &vTransactionOuts)
{
    if (nTransactions <= 0)
        throw runtime_error("Invalid transaction count");

    vTransactionOuts.assign(nTransactions, Object());

    if (nTransactions > vDistribution.size())
        throw runtime_error("Output split in too many transactions");

    int nTransactionIndex = 0;

    BOOST_FOREACH(const Distribution &distribution, vDistribution)
    {
        double amount = distribution.GetDividendAmount();
        string address = distribution.GetPeercoinAddress().ToString();
        Object &out = vTransactionOuts[nTransactionIndex];

        out.push_back(Pair(address, (double)amount));
        nTransactionIndex = (nTransactionIndex + 1) % nTransactions;
    }
}

