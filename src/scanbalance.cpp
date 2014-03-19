#include "db.h"
#include "walletdb.h"
#include "net.h"
#include "init.h"
#include "util.h"

#ifndef WIN32
#include <signal.h>
#endif

using namespace std;
using namespace boost;

typedef map<CBitcoinAddress,int64> AddrBalance;

static void ScanTransactionInputs(CTxDB&txdb,const CTransaction&t,AddrBalance&ab)
{
    if (t.IsCoinBase()) return;

    BOOST_FOREACH(const CTxIn& txi, t.vin )
    {
        CTransaction ti;
        if (!txdb.ReadDiskTx(txi.prevout,ti))
        {
            string s=strprintf("When scanning input of transaction:\n"
                               "  %s\n"
                               "Failed to load transaction:\n"
                               "  %s\n",
                               t.GetHash().ToString().c_str(), txi.ToStringShort().c_str());
            throw runtime_error(s);
        }
        if (txi.prevout.n>=ti.vout.size())
        {
            string s=strprintf("When scanning input of transaction:\n"
                               "  %s\n"
                               "Invalid index in:\n"
                               "  %s\n",
                               t.GetHash().ToString().c_str(), txi.ToStringShort().c_str());
            throw runtime_error(s);
        }


        CTxOut prevOut=ti.vout[txi.prevout.n];
        CBitcoinAddress addr;
        ExtractAddress(prevOut.scriptPubKey,addr);
        fprintf(stdout,"  I from=%s amt=%"PRI64d"\n", addr.ToString().c_str(),prevOut.nValue);
        if (prevOut.nValue>0) //0 in genesis block
        {
            if (0==ab.count(addr)) {
                string s=strprintf("When scanning input of transaction:\n"
                                   "  %s\n"
                                   "Encountered missing input address:\n"
                                   "  %s\n",
                                   t.GetHash().ToString().c_str(), txi.ToStringShort().c_str());
                throw runtime_error(s);
            }
            if (prevOut.nValue>ab[addr]) {
                string s=strprintf("When scanning input of transaction:\n"
                                   "  %s\n"
                                   "Input address would have negative balance:\n"
                                   "  %s\n",
                                   t.GetHash().ToString().c_str(), txi.ToStringShort().c_str());
                throw runtime_error(s);
            }
            ab[addr]-=prevOut.nValue;
            if (0==ab[addr]) ab.erase(addr);
        }
    } //BOOST_FOREACH(const CTxIn& txi, t.vin )
}

static void ScanTransactionOutputs(const CTransaction&t,AddrBalance&ab)
{
    BOOST_FOREACH(const CTxOut& txo, t.vout )
    {
        if (txo.nValue>0 && !txo.scriptPubKey.empty())
        {
            CBitcoinAddress addr;
            ExtractAddress(txo.scriptPubKey, addr);
            ab[addr]+=txo.nValue;
            //fprintf(stdout,"  O addr=%s val=%"PRI64d"\n", addr.ToString().c_str(),txo.nValue);
        }
    }
}

#ifdef QT_GUI
//This is is distributeDivDialog.cpp
void UpdateProgressForScanBalance(void*pProgressContext,int nPercent,bool fAbort);
#else
void UpdateProgressForScanBalance(void*pProgressContext,int nPercent,bool fAbort)
{
    if (pProgressContext)
        fprintf(stdout,"*** addr balance scanning progress update: %p %d%% abort=%c\n",
                pProgressContext,nPercent, fAbort?'Y':'N');
}
#endif

/*Returns number of seconds from cutoff date to the best block time at the time of call.

Q: Do we need to sync access to pindexGenesisBlock & pindexBest ?

This function can be made fater by building a in-memory transaction map as we scan blocks
instead of using CTxDB.
*/
static int GetAddressBalancesInternal(unsigned int cutoffTime, volatile bool*pfUserCanceled,
                                      AddrBalance & ab,void*pProgressContext)
{
    CBlockIndex*pblk0=pindexGenesisBlock, *pblk1=pindexBest;
    if (!pblk0) throw runtime_error("No genesis block.");
    if (!pblk1) throw runtime_error("No best block chain.");

    if (cutoffTime>pblk1->nTime) throw runtime_error("Cutoff date later than most recent block.");

    int nSecsSinceCutoff=(int)(pblk1->nTime - cutoffTime);

    unsigned int nGenesisBlockTime=pblk0->nTime;
    float dScanTimeSpan= cutoffTime-nGenesisBlockTime;
    if (dScanTimeSpan<1) dScanTimeSpan=1; //just in case
    dScanTimeSpan*=1.02f; //add 2% for post-processing time
    int nLastProgressPercentage=-1;

    CTxDB txdb("r");
    int nBlks=0;
    while (pblk0!=pblk1)
    { //for each block
        if (*pfUserCanceled) return 0;
        if (pblk0->nTime>=cutoffTime) break;
        CBlock b;
        b.ReadFromDisk(pblk0, true);

        BOOST_FOREACH(const CTransaction& t, b.vtx)
        {
            ScanTransactionInputs(txdb,t,ab);
            ScanTransactionOutputs(t,ab);
        }
        float dV=100.0f*(pblk0->nTime - nGenesisBlockTime)/dScanTimeSpan;
        int nPercent=(int)(dV+0.5f);
        if (nPercent==100) nPercent=99; //just in case, setting progress to 100% will cause QProgressDialog to close
        if (nPercent!=nLastProgressPercentage)
        { //Update progress only when integer percentage value changes.
            nLastProgressPercentage=nPercent;
            UpdateProgressForScanBalance(pProgressContext,nPercent,false);
        }
        pblk0=pblk0->pnext;
        nBlks++;
    }
    return nSecsSinceCutoff;
}


bool GetAddressBalances(void*pProgressContext, unsigned int cutoffTime, volatile bool*pfUserCanceled,
                        int&nSecsSinceCutoff, AddrBalance&ab,string&errMsg)
{   bool ret=true;
    bool bFALSE=false;

    if (!pfUserCanceled) pfUserCanceled=&bFALSE;

    try {
        nSecsSinceCutoff=GetAddressBalancesInternal(cutoffTime,pfUserCanceled,ab,pProgressContext);
        errMsg="";
        if (!(*pfUserCanceled)) UpdateProgressForScanBalance(pProgressContext,100,false);
    } catch (std::exception&e)
    {
        errMsg=e.what();
        ret=false;
    } catch (...)
    {
        errMsg="Unknown exception when scanning blockchain.";
        ret=false;
    }
    if (!ret) UpdateProgressForScanBalance(pProgressContext,-1,true);
    return ret;
}

//To test balance scanning, call this function during initialization from AppInit2().
//It checks for command line option -addrbalance=yyyymmdd
void CmdLine_GetAddrBalances()
{
    string ymd=GetArg("-addrbalance","");
    if (ymd.length()==0) return;

    fprintf(stdout,"******************** gettting balances of all addresss\n");
    fprintf(stdout,"specified local cutoff date: %s\n",ymd.c_str());

    int i=atoi(ymd.c_str());
    tm t;
    memset(&t,0,sizeof(t));
    t.tm_mday= i % 100; i/=100; //no error checking on these values
    t.tm_mon= (i % 100) -1; i/=100;
    t.tm_year=i-1900;
    t.tm_isdst=-1;
    time_t cutoffTime=mktime(&t);
    cutoffTime+=24*60*60; //actual cutoff at beginning of next day

    int nSecsSinceCutoff;
    AddrBalance ab;
    string errMsg;
    bool ok=GetAddressBalances((void*)GetAddressBalances, (unsigned int)cutoffTime, NULL,
                               nSecsSinceCutoff,ab,errMsg);
    
    if (ok)
    {
        fprintf(stdout,"GetAddressBalances() returned OK\n");
        fprintf(stdout,"best block secs since cutoff: %u, days since: %.3f\n",
                nSecsSinceCutoff,(float)nSecsSinceCutoff/(24*60*60));
        fprintf(stdout,"addresses with nonzero balance: %u\n",ab.size());
        fprintf(stdout,"*************** address & balance map ******************\n");
        BOOST_FOREACH(const PAIRTYPE(CBitcoinAddress,int64)&a, ab)
        {
            fprintf(stdout,"%s %s\n",a.first.ToString().c_str(),FormatMoney(a.second).c_str());
        }
    }
    else fprintf(stdout,"GetAddressBalances() returned error msg: %s\n", errMsg.c_str() );

    fprintf(stdout,"cutoff day inclusive: y=%d m=%d d=%d\n",t.tm_year+1900,t.tm_mon+1,t.tm_mday);
    fprintf(stdout,"actual cutoff epoch=%u, UTC time: %s\n",
            (unsigned int)cutoffTime, asctime(gmtime(&cutoffTime)) );

}
