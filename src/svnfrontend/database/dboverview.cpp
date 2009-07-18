#include "dboverview.h"
#include "src/svnqt/cache/ReposConfig.hpp"
#include "src/svnqt/cache/LogCache.hpp"
#include "src/svnqt/cache/ReposLog.hpp"
#include "src/svnqt/cache/DatabaseException.hpp"
#include "src/svnfrontend/fronthelpers/createdlg.h"
#include "src/svnqt/client.hpp"
#include "helpers/stringhelper.h"

#include <QStringListModel>
#include <QItemSelectionModel>

#include <KDebug>
#include <KMessageBox>
#include <KLocale>

class DbOverViewData
{

public:
    QStringListModel*repo_model;
    svn::Client*_Client;

    DbOverViewData()
    {
        repo_model = new QStringListModel();
        _Client = 0;
    }
    ~DbOverViewData()
    {
        delete repo_model;
    }
};

DbOverview::DbOverview(QWidget *parent, const char *name)
    : QWidget(parent)
{
    setupUi(this);
    setObjectName(name);
    enableButtons(false);
    _data = new DbOverViewData;

    try {
        _data->repo_model->setStringList(svn::cache::LogCache::self()->cachedRepositories());
    }catch (const svn::cache::DatabaseException&e) {
        kDebug()<<e.msg()<<endl;
    }

    m_ReposListView->setModel(_data->repo_model);
    QItemSelectionModel * _sel = m_ReposListView->selectionModel();
    if (_sel) {
        connect(_sel,SIGNAL(selectionChanged (const QItemSelection&,const QItemSelection&)),this,SLOT(itemActivated(const QItemSelection&,const QItemSelection&)));
    }
}

DbOverview::~DbOverview()
{
    delete _data;
}

void DbOverview::showDbOverview(svn::Client*aClient)
{
    DbOverview*ptr = 0;
    static const char*cfg_text = "db_overview_dlg";
    KConfigGroup _kc(Kdesvnsettings::self()->config(),cfg_text);
    KDialog*dlg = createDialog(&ptr,QString(i18n("Overview about cache database content")),false,"DatabaseOverview",cfg_text);
    ptr->setClient(aClient);
    dlg->restoreDialogSize(_kc);
    dlg->exec();
    dlg->saveDialogSize(_kc);
    _kc.sync();
    delete dlg;
}

void DbOverview::setClient(svn::Client*aClient)
{
    _data->_Client = aClient;
}

void DbOverview::enableButtons(bool how)
{
    m_DeleteCacheButton->setEnabled(how);
    m_DeleteRepositoryButton->setEnabled(how);
}

void DbOverview::itemActivated(const QItemSelection&indexes,const QItemSelection&deindexes)
{
    Q_UNUSED(deindexes);

    enableButtons(false);
    QModelIndexList _indexes = indexes.indexes();
    if (_indexes.count()!=1) {
        kDebug()<<"Handle only with single selection"<<endl;
        return;
    }
    genInfo(_indexes[0].data().toString());
    enableButtons(true);
}

void DbOverview::genInfo(const QString&repo)
{
    svn::cache::ReposLog rl(_data->_Client,repo);
    const static QString info(i18n("Log cache holds %1 logentries and consumes %2 on disk."));
    QString msg = info.arg(rl.count()).arg(helpers::ByteToString()(rl.fileSize()));
    m_RepostatusBrowser->setText(msg);
}

QString DbOverview::selectedRepository()const
{
    QModelIndexList _indexes = m_ReposListView->selectionModel()->selectedIndexes();
    if (_indexes.size()!=1) {
        return QString();
    }
    return _indexes[0].data().toString();
}

void DbOverview::deleteCacheItems()
{
    int i = KMessageBox::questionYesNo(this,i18n("Realy clean cache for repository\n%1?").arg(selectedRepository()),i18n("Clean repository cache"));
    if (i != KMessageBox::Yes) {
        return;
    }
    try
    {
        svn::cache::ReposLog rl(_data->_Client,selectedRepository());
        rl.cleanLogEntries();
    }
    catch (const svn::cache::DatabaseException&e)
    {
        kDebug()<<e.msg();
    }
    genInfo(selectedRepository());
}

void DbOverview::deleteRepository()
{
}

#include "dboverview.moc"