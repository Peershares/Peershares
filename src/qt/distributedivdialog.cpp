#include "bitcoingui.h"
#include "distributedivdialog.h"
#include "ui_distributedivdialog.h"

#include <QFileDialog>
#include "QStandardItemModel"
#include <QMessageBox>
#include <ctime>

struct DummyDistribution
{
    CBitcoinAddress addrPS;
    int64 balPS;
    CBitcoinAddress addrPC;
    int64 dividend;

    static bool biggerBalance(DummyDistribution a,DummyDistribution b)
    {
        return a.balPS > b.balPS;
    }
};

void UpdateProgressForScanBalance(void*pProgressContext,int nPercent,bool fAbort)
{
    ((AddrBalanceScanner*)pProgressContext)->updateProgress(nPercent,fAbort);
}

static int nscans=0;
bool AddrBalanceScanner::GetAddressBalances()
{
    nscans++;

    srand((int)time(0));

    (string&)errorMsg="";
    mapAB.clear();
    //It's annoying to be stuck at 100% and waiting, so we add 5% for post-scan processing time.
    //Being stuck at 95% feels better.
    int total=105;

    vector<unsigned char> v2integers;
    v2integers.resize(2*sizeof(int));
    int*p2ints=(int*)(&v2integers[0]);

    for (int i=0;i<100;i++) {
        QThread::msleep(5);
        if (fUserCanceled) return false;

        if (0==(nscans&3) && i>50)
        { //error every 4th run
            (string&)errorMsg="error description text here";
            mapAB.clear();
            emit updateScanningProgress(i*100/total,true);
            fScanningError=true;
            return false;
        }
        emit updateScanningProgress(i*100/total,false);

        for (int j=0;j<100;j++) {
            p2ints[0]=rand();
            p2ints[1]=rand();
            CBitcoinAddress ba(v2integers);
            mapAB[ba]=(int64)rand();
        }
    }
    nSecsSinceCutoff=rand()*100;
    QThread::msleep(100);
    emit updateScanningProgress(100,false);
    return true;
}

void AddrBalanceScanner::run()
{

    bool GetAddressBalances(void*pProgressContext, unsigned int cutoffTime, volatile bool*pfUserCanceled,
                            int&nSecsSinceCutoff, map<CBitcoinAddress,int64>&ab,string&errMsg);

    GetAddressBalances(this,cutoffTime,&fUserCanceled, nSecsSinceCutoff,mapAB,errorMsg);
    //qDebug("thread starting");
    //GetAddressBalances();
    //qDebug("thread exiting");
}

void AddrBalanceScanner::Scan(unsigned int _cutoffTime)
{
    cutoffTime= _cutoffTime;
    fUserCanceled=fScanningError=false;

    connect(this, SIGNAL(updateScanningProgress(int,bool)),
            this, SLOT(receiveScanningProgress(int,bool)), Qt::QueuedConnection);
    connect(&prgDlg,SIGNAL(canceled()), this,SLOT(onProgressDialogCanceled()));

    mapAB.clear();
    prgDlg.setWindowFlags(prgDlg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
    prgDlg.reset();
    prgDlg.setWindowTitle("Please Wait");
    prgDlg.setLabelText("Scanning local blockchain");
    prgDlg.setMinimumDuration(0);
    prgDlg.setModal(true);
    prgDlg.setValue(0);
    start(QThread::IdlePriority);
    prgDlg.exec();
    wait(); //make sure the worker thread finishes
}


//-----------------------------
const char*DistributeDivDialog::columnHeadings[]={
    "Peershares Addr", "Balance", "Peercoin Addr", "Dividend"
};

DistributeDivDialog::DistributeDivDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DistributeDivDialog)
{
    ui->setupUi(this);

    setWindowFlags((Qt::WindowFlags)(~Qt::WindowContextHelpButtonHint)&windowFlags());

    QStandardItemModel *pm = new QStandardItemModel(0,4,this);

    QStringList sl;
    sl<<columnHeadings[0] << columnHeadings[1] << columnHeadings[2] << columnHeadings[3] ;
    pm->setHorizontalHeaderLabels(sl);
    ui->tableView->setModel(pm);
    ui->tableView->resizeColumnsToContents();
    ui->tableView->verticalHeader()->setVisible(false);
    ui->scanResultInfo->setText("");
}

DistributeDivDialog::~DistributeDivDialog()
{
    delete ui;
}

void DistributeDivDialog::resizeEvent(QResizeEvent *qre)
{   Q_UNUSED(qre);
    ui->tableView->resizeColumnsToContents();
}

static string formatPeercoinAmount(int64 a)
{   char buf[64], *p=buf;
    if (a<0)
    {
        a=-a;
        *p='-'; p++;
    }
    sprintf(p,"%d.%06d",(int)(a/1000000),(int)(a%1000000));
    return (string)buf;
}

void DistributeDivDialog::on_calcDividendsButton_clicked()
{
    bool ok;
    double d=ui->totalDividend->text().toDouble(&ok);
    if (!(ok && d>0))
    {
        QMessageBox::critical(this,"Invalid Total Dividend","Please enter a valid total dividend value.");
        ui->totalDividend->setFocus();
        return;
    }

    int64 sum=0;
    for (unsigned int i=0;i<dist.size();i++) sum+=dist[i].balPS;
    if (0==sum)
    {
        QMessageBox::about(this,"You need to get list of account balances first.","Info");
        return;
    }

    double v=d/(double)sum;
    //int64 paysum=0;
    QStandardItemModel *pm = (QStandardItemModel*)ui->tableView->model();
    QString s;
    for (unsigned int i=0;i<dist.size();i++)
    {
        int64 pay=(int64)(dist[i].balPS *v*1e6+0.5);
        dist[i].dividend=pay;
        pm->setItem(i,3,new QStandardItem(formatPeercoinAmount(pay).c_str()));
    }
    ui->tableView->resizeColumnsToContents();
}


void DistributeDivDialog::updateTableDisplay()
{
    QStandardItemModel *pm = (QStandardItemModel*)ui->tableView->model();
    pm->removeRows(0,pm->rowCount());
    pm->setRowCount(dist.size());
    QString s;
    for (unsigned int i=0;i<dist.size();i++) {
        pm->setItem(i,0,new QStandardItem(dist[i].addrPS.ToString().c_str()));
        pm->setItem(i,1,new QStandardItem(formatPeercoinAmount(dist[i].balPS).c_str()));
        pm->setItem(i,2,new QStandardItem(dist[i].addrPC.ToString().c_str()));
    }
    ui->tableView->resizeColumnsToContents();

    s.sprintf("Best block since end of record date: %.2f days",(float)nSecsSinceCutoff/(24*60*60));
    ui->scanResultInfo->setText((s));
}

void DistributeDivDialog::on_getShareholdsListButton_clicked()
{
    QDate qd=ui->recordDate->date();
    tm t;
    memset(&t,0,sizeof(t));
    t.tm_mday= qd.day();
    t.tm_mon= qd.month()-1;
    t.tm_year=qd.year()-1900;
    t.tm_isdst=-1;
    time_t cutoffTime=mktime(&t);

    cutoffTime+=24*60*60; //actual cutoff at beginning of next day

    AddrBalanceScanner scanner(this);

    scanner.Scan((unsigned int)cutoffTime);

    if (scanner.fScanningError)
    {
        QMessageBox::critical(this,"Scanning Error",scanner.errorMsg.c_str());
        return;
    }

    if (scanner.fUserCanceled)
    {
        //QMessageBox::information(this,"Scanning","Scanning stopped by user.");
        return;
    }

    if (0==scanner.mapAB.size())
    {   //From scanner's point of view, this is not an error.
        //But we don't want to just clear the list view here.
        QMessageBox::information(this,"Scanning Result","No address found by that record date.");
        return;
    }

    //copy results
    nSecsSinceCutoff=scanner.nSecsSinceCutoff;

    dist.resize(scanner.mapAB.size());
    map<CBitcoinAddress,int64>::const_iterator it=scanner.mapAB.begin();
    for (unsigned int i=0;i<scanner.mapAB.size();i++,it++)
    {
        dist[i].addrPS=it->first;
        dist[i].balPS=it->second;
        dist[i].addrPC=it->first; //just copy Peershares addr for now
        dist[i].dividend=0;
    }

    sort(dist.begin(),dist.end(),DummyDistribution::biggerBalance);

    updateTableDisplay();
}

void DistributeDivDialog::on_exportButton_clicked()
{
    if (0==dist.size())
    {
        QMessageBox::about(this,"Nothing to export.","No shareholders list to export.");
        return;
    }

    QString fn = QFileDialog::getSaveFileName(this, tr("Save As ..."), "",
                        "CSV files (*.csv);;All files (*.*)");

    FILE*fp=fopen(fn.toStdString().c_str(),"wt");
    if (!fp) {
        QMessageBox::critical(this,"File save error","Failed to open file for writing:\n  "+fn);
        return;
    }
    fprintf(fp,"%s,%s,%s,%s\n",columnHeadings[0],columnHeadings[1],columnHeadings[2],columnHeadings[3]);

    for (unsigned int i=0;i<dist.size();i++)
    {
        fprintf(fp,"%s,%s,%s,%s\n",
                dist[i].addrPS.ToString().c_str(),formatPeercoinAmount(dist[i].balPS).c_str(),
                dist[i].addrPC.ToString().c_str(),formatPeercoinAmount(dist[i].dividend).c_str()
                );
    }

    fclose(fp);
    QMessageBox::about(this,"OK","Successfully save to file: "+fn);
}

void DistributeDivDialog::on_buttonBox_accepted()
{
    QMessageBox::about(this,"OK","Pay dividend.");
}
