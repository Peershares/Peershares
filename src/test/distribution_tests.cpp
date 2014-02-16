#include <boost/test/unit_test.hpp>

#include "distribution.h"

BOOST_AUTO_TEST_SUITE(distribution_tests)

BOOST_AUTO_TEST_CASE( test_simple_distribution )
{
    BalanceMap mapBalance;

    mapBalance[CBitcoinAddress(1)] = 10;
    mapBalance[CBitcoinAddress(2)] = 30;

    DividendDistributor distributor(mapBalance);
    distributor.Distribute(1000000);

    const DistributionVector vDistribution = distributor.GetDistributions();
    DistributionVector::const_iterator it;

    BOOST_CHECK_EQUAL(240000, distributor.GetDistribution(CBitcoinAddress(1)).GetDividendAmount());
    BOOST_CHECK_EQUAL(10000, distributor.GetDistribution(CBitcoinAddress(1)).GetTransactionFee());

    BOOST_CHECK_EQUAL(740000, distributor.GetDistribution(CBitcoinAddress(2)).GetDividendAmount());
    BOOST_CHECK_EQUAL(10000, distributor.GetDistribution(CBitcoinAddress(2)).GetTransactionFee());

    BOOST_CHECK_EQUAL(980000, distributor.TotalDistributed());
    BOOST_CHECK_EQUAL(20000, distributor.TotalFee());
}

BOOST_AUTO_TEST_CASE( test_off_decimal_distribution )
{
    BalanceMap mapBalance;

    mapBalance[CBitcoinAddress(1)] = 1;
    mapBalance[CBitcoinAddress(2)] = 1;
    mapBalance[CBitcoinAddress(3)] = 1;

    DividendDistributor distributor(mapBalance);
    distributor.Distribute(1000000);

    const DistributionVector vDistribution = distributor.GetDistributions();
    DistributionVector::const_iterator it;

    BOOST_CHECK_EQUAL(323333, distributor.GetDistribution(CBitcoinAddress(1)).GetDividendAmount());
    BOOST_CHECK_EQUAL(10000, distributor.GetDistribution(CBitcoinAddress(1)).GetTransactionFee());

    BOOST_CHECK_EQUAL(323333, distributor.GetDistribution(CBitcoinAddress(2)).GetDividendAmount());
    BOOST_CHECK_EQUAL(10000, distributor.GetDistribution(CBitcoinAddress(2)).GetTransactionFee());

    BOOST_CHECK_EQUAL(323333, distributor.GetDistribution(CBitcoinAddress(3)).GetDividendAmount());
    BOOST_CHECK_EQUAL(10000, distributor.GetDistribution(CBitcoinAddress(3)).GetTransactionFee());

    BOOST_CHECK_EQUAL(969999, distributor.TotalDistributed());
    BOOST_CHECK_EQUAL(30000, distributor.TotalFee());
}

BOOST_AUTO_TEST_CASE( test_not_enough_dividends_to_pay_fee )
{
    BalanceMap mapBalance;

    mapBalance[CBitcoinAddress(1)] = 99;
    mapBalance[CBitcoinAddress(2)] = 1;

    DividendDistributor distributor(mapBalance);
    distributor.Distribute(100000);

    const DistributionVector vDistribution = distributor.GetDistributions();
    DistributionVector::const_iterator it;

    BOOST_CHECK_EQUAL(90000, distributor.GetDistribution(CBitcoinAddress(1)).GetDividendAmount());
    BOOST_CHECK_EQUAL(10000, distributor.GetDistribution(CBitcoinAddress(1)).GetTransactionFee());

    BOOST_CHECK_EQUAL(1, distributor.GetDistributions().size());

    BOOST_CHECK_EQUAL(90000, distributor.TotalDistributed());
    BOOST_CHECK_EQUAL(10000, distributor.TotalFee());
}

BOOST_AUTO_TEST_CASE( test_nobody_has_enough_funds )
{
    BalanceMap mapBalance;

    mapBalance[CBitcoinAddress(1)] = 1;
    mapBalance[CBitcoinAddress(2)] = 1;

    DividendDistributor distributor(mapBalance);
    distributor.Distribute(20000);

    const DistributionVector vDistribution = distributor.GetDistributions();

    BOOST_CHECK_EQUAL(0, distributor.GetDistributions().size());

    BOOST_CHECK_EQUAL(0, distributor.TotalDistributed());
    BOOST_CHECK_EQUAL(0, distributor.TotalFee());
}

BOOST_AUTO_TEST_SUITE_END()
