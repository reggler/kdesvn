/***************************************************************************
 *   Copyright (C) 2005-2007 by Rajko Albrecht                             *
 *   ral@alwins-world.de                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
#include "kdesvn-config.h"
#include "kiosvn.h"
#include "kiolistener.h"

#include "src/svnqt/svnqttypes.hpp"
#include "src/svnqt/dirent.hpp"
#include "src/svnqt/url.hpp"
#include "src/svnqt/status.hpp"
#include "src/svnqt/targets.hpp"
#include "src/svnqt/info_entry.hpp"
#include "src/svnqt/client_parameter.hpp"
#include "src/settings/kdesvnsettings.h"
#include "src/helpers/sub2qt.h"
#include "src/helpers/sshagent.h"
#include "kdesvndinterface.h"

#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kdemacros.h>
#include <kmessagebox.h>
#include <kcomponentdata.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kurl.h>
#include <ktempdir.h>
#include <kmimetype.h>
#include <krun.h>

namespace KIO
{

class KioSvnData
{
public:
    KioSvnData(kio_svnProtocol*);
    virtual ~KioSvnData();

    void reInitClient();

    KioListener m_Listener;
    bool first_done;
    bool dispProgress;
    svn::ContextP m_CurrentContext;
    svn::Client* m_Svnclient;

    svn::Revision urlToRev(const KUrl&);

};

KioSvnData::KioSvnData(kio_svnProtocol*par)
    : m_Listener(par),first_done(false)
{
    m_Svnclient=svn::Client::getobject(0,0);
    m_CurrentContext = 0;
    dispProgress = false;
    reInitClient();
}

void KioSvnData::reInitClient()
{
    if (first_done) {
        return;
    }
    SshAgent ag;
    ag.querySshAgent();

    first_done = true;
    m_CurrentContext = new svn::Context();
    m_CurrentContext->setListener(&m_Listener);
    m_Svnclient->setContext(m_CurrentContext);
}

KioSvnData::~KioSvnData()
{
    m_Listener.setCancel(true);
    /* wait a little bit */
    sleep(1);
    delete m_Svnclient;
    m_CurrentContext->setListener(0L);
    m_CurrentContext = 0;
}

svn::Revision KioSvnData::urlToRev(const KUrl&url)
{
    QMap<QString,QString> q = url.queryItems();

    /* we try to check if it is ssh and try to get a password for it */
    QString proto = url.protocol();

    if (proto.indexOf("ssh")!=-1) {
        SshAgent ag;
        ag.addSshIdentities();
    }

    svn::Revision rev,tmp;
    rev = svn::Revision::UNDEFINED;
    if (q.find("rev")!=q.end()) {
        QString v = q["rev"];
        m_Svnclient->url2Revision(v,rev,tmp);
    }
    return rev;
}

kio_svnProtocol::kio_svnProtocol(const QByteArray &pool_socket, const QByteArray &app_socket)
    : SlaveBase("kio_ksvn", pool_socket, app_socket),StreamWrittenCb()
{
    m_pData=new KioSvnData(this);
    KGlobal::locale()->insertCatalog("kdesvn");
}

kio_svnProtocol::~kio_svnProtocol()
{
    delete m_pData;
}

}

extern "C"
{
    KDESVN_EXPORT int kdemain(int argc, char **argv);
}

int kdemain(int argc, char **argv)
{
    KComponentData componentData("kio_ksvn");
    kDebug(7101) << "*** Starting kio_ksvn " << endl;

    if (argc != 4) {
        kDebug(7101) << "Usage: kio_ksvn  protocol domain-socket1 domain-socket2" << endl;
        exit(-1);
    }
    KIO::kio_svnProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();
    kDebug(7101) << "*** kio_ksvn Done" << endl;
    return 0;
}

namespace KIO
{

void kio_svnProtocol::listSendDirEntry(const svn::DirEntry&direntry)
{
        QDateTime dt = svn::DateTime(direntry.time());
        KIO::UDSEntry entry;
        if (direntry.name().isEmpty()) {
            kDebug(9510)<<"Skipping empty entry!"<<endl;
            return;
        }
        if (createUDSEntry(direntry.name(),
            direntry.lastAuthor(),
            direntry.size(),
            direntry.kind()==svn_node_dir?true:false,
            dt.toTime_t(),
            entry) ) {
                listEntry(entry,false);
            }
}

/*!
    \fn kio_svnProtocol::listDir (const KUrl&url)
 */
void kio_svnProtocol::listDir(const KUrl&url)
{
    kDebug(9510) << "kio_svn::listDir(const KUrl& url) : " << url.url() << endl ;
    svn::DirEntries dlist;
    svn::Revision rev = m_pData->urlToRev(url);
    if (rev == svn::Revision::UNDEFINED) {
        rev = svn::Revision::HEAD;
    }

    try {
        // we ignoring the result 'cause it is done via kiolistener for a smoother insert of items.
        dlist = m_pData->m_Svnclient->list(makeSvnUrl(url),rev,rev,svn::DepthImmediates,false);
    } catch (const svn::ClientException&e) {
        QString ex = e.msg();
        kDebug(9510)<<ex<<endl;
        error(KIO::ERR_SLAVE_DEFINED,ex);
        return;
    }
    listEntry(KIO::UDSEntry(), true );
    finished();
    kDebug(9510)<<"Listing finished"<<endl;
}

void kio_svnProtocol::stat(const KUrl& url)
{
    kDebug(9510)<<"kio_svn::stat "<< url << endl;
    svn::Revision rev = m_pData->urlToRev(url);
    if (rev == svn::Revision::UNDEFINED) {
        rev = svn::Revision::HEAD;
    }
    svn::Revision peg = rev;
    bool dummy = false;
    svn::InfoEntries e;
    try {
         e = m_pData->m_Svnclient->info(makeSvnUrl(url),svn::DepthEmpty,rev,peg);
    } catch  (const svn::ClientException&e) {
        QString ex = e.msg();
        kDebug(9510)<<ex<<endl;
        error( KIO::ERR_SLAVE_DEFINED,ex);
        return;
    }

    if (e.count()==0) {
        dummy = true;
    }

    KIO::UDSEntry entry;
    QDateTime dt;
    if (dummy) {
        createUDSEntry(url.fileName(),"",0,true,dt.toTime_t(),entry);
    } else {
        dt = svn::DateTime(e[0].cmtDate());
        if (e[0].kind()==svn_node_file) {
            createUDSEntry(url.fileName(),"",0,false,dt.toTime_t(),entry);
        } else {
            createUDSEntry(url.fileName(),"",0,true,dt.toTime_t(),entry);
        }
    }
    statEntry(entry);
    finished();
}

void kio_svnProtocol::get(const KUrl& url)
{
    kDebug(9510)<<"kio_svn::get "<< url << endl;
    if (m_pData->m_Listener.contextCancel()) {
        finished();
        return;
    }
    svn::Revision rev = m_pData->urlToRev(url);
    if (rev == svn::Revision::UNDEFINED) {
        rev = svn::Revision::HEAD;
    }
    KioByteStream dstream(this,url.fileName());
    try {
        m_pData->m_Svnclient->cat(dstream,makeSvnUrl(url),rev,rev);
    } catch (const svn::ClientException&e) {
        QString ex = e.msg();
        kDebug(9510)<<ex<<endl;
        error( KIO::ERR_SLAVE_DEFINED,"Subversion error "+ex);
        return;
    }
    totalSize(dstream.written());
    data(QByteArray()); // empty array means we're done sending the data
    finished();
}

void kio_svnProtocol::mkdir(const KUrl &url, int)
{
    kDebug(9510)<<"kio_svn::mkdir "<< url << endl;
    svn::Revision rev = m_pData->urlToRev(url);
    if (rev == svn::Revision::UNDEFINED) {
        rev = svn::Revision::HEAD;
    }
    try {
        svn::Path p(makeSvnUrl(url));
        m_pData->m_Svnclient->mkdir(p,getDefaultLog());
    }catch (const svn::ClientException&e) {
        error( KIO::ERR_SLAVE_DEFINED,e.msg());
        kDebug(9510)<<"kio_svn::mkdir aborted" << url << endl;
        return;
    }
    kDebug(9510)<<"kio_svn::mkdir finished " << url << endl;
    finished();
}

void kio_svnProtocol::mkdir(const KUrl::List &urls, int)
{
    svn::Pathes p;
    for (KUrl::List::const_iterator it = urls.begin(); it != urls.end() ; ++it ) {
        p.append((*it).path());
    }
    try {
        m_pData->m_Svnclient->mkdir(svn::Targets(p),getDefaultLog());
    } catch (const svn::ClientException&e) {
        error(KIO::ERR_SLAVE_DEFINED,e.msg());
        return;
    }
    finished();
}

void kio_svnProtocol::rename(const KUrl&src,const KUrl&target,KIO::JobFlags flags)
{
    kDebug(9510)<<"kio_svn::rename "<< src << " to " << target <<  endl;
    Q_UNUSED(flags);
    //bool force  = flags&KIO::Overwrite;
    QString msg;
    m_pData->m_CurrentContext->setLogMessage(getDefaultLog());
    try {
        m_pData->m_Svnclient->move(makeSvnUrl(src),makeSvnUrl(target),false);
    } catch (const svn::ClientException&e) {
        if (e.apr_err()==SVN_ERR_ENTRY_EXISTS) {
            error(KIO::ERR_DIR_ALREADY_EXIST,e.msg());
        } else {
            error(KIO::ERR_SLAVE_DEFINED,e.msg());
        }
        kDebug(9510)<<"kio_svn::rename aborted" <<  endl;
        return;
    }
    kDebug(9510)<<"kio_svn::rename finished" <<  endl;
    finished();
}

void kio_svnProtocol::copy(const KUrl&src,const KUrl&dest,int permissions,KIO::JobFlags flags)
{
    Q_UNUSED(permissions);
    Q_UNUSED(flags);
    //bool force = flags&KIO::Overwrite;

    kDebug(9510)<<"kio_svn::copy "<< src << " to " << dest <<  endl;
    svn::Revision rev = m_pData->urlToRev(src);
    if (rev == svn::Revision::UNDEFINED) {
        rev = svn::Revision::HEAD;
    }
    m_pData->dispProgress=true;
    m_pData->m_CurrentContext->setLogMessage(getDefaultLog());
    try {
        m_pData->m_Svnclient->copy(makeSvnUrl(src),rev,makeSvnUrl(dest));
    }catch (const svn::ClientException&e) {
        if (e.apr_err()==SVN_ERR_ENTRY_EXISTS) {
            error(KIO::ERR_DIR_ALREADY_EXIST,e.msg());
        } else {
            error(KIO::ERR_SLAVE_DEFINED,e.msg());
        }
        kDebug(9510)<<"kio_svn::copy aborted" <<  endl;
        return;
    }
    m_pData->dispProgress=false;
    kDebug(9510)<<"kio_svn::copy finished" <<  endl;
    finished();
}

void kio_svnProtocol::del(const KUrl&src,bool isfile)
{
    Q_UNUSED(isfile);
    kDebug(9510)<<"kio_svn::del "<< src << endl;
    //m_pData->reInitClient();
    svn::Revision rev = m_pData->urlToRev(src);
    if (rev == svn::Revision::UNDEFINED) {
        rev = svn::Revision::HEAD;
    }
    m_pData->m_CurrentContext->setLogMessage(getDefaultLog());
    try {
        svn::Targets target(makeSvnUrl(src));
        m_pData->m_Svnclient->remove(target,false);
    } catch (const svn::ClientException&e) {
        QString ex = e.msg();
        kDebug(9510)<<ex<<endl;
        error( KIO::ERR_SLAVE_DEFINED,ex);
        kDebug(9510)<<"kio_svn::del aborted" << endl;
        return;
    }
    kDebug(9510)<<"kio_svn::del finished" << endl;
    finished();
}

bool kio_svnProtocol::getLogMsg(QString&t)
{
    svn::CommitItemList _items;
    return m_pData->m_Listener.contextGetLogMessage(t,_items);
}

bool kio_svnProtocol::checkWc(const KUrl&url)
{
    if (url.isEmpty()||!url.isLocalFile()) return false;
    svn::Revision peg(svn_opt_revision_unspecified);
    svn::Revision rev(svn_opt_revision_unspecified);
    svn::InfoEntries e;
    try {
        e = m_pData->m_Svnclient->info(url.prettyUrl(),svn::DepthEmpty,rev,peg);
    } catch (const svn::ClientException&e) {
        if (SVN_ERR_WC_NOT_DIRECTORY==e.apr_err())
        {
            return false;
        }
        return true;
    }
    return false;
}

QString kio_svnProtocol::makeSvnUrl(const KUrl&url,bool check_Wc)
{
    QString res;
    QString proto = svn::Url::transformProtokoll(url.protocol());
    if (proto=="file" && check_Wc)
    {
        if (checkWc(url))
        {
            return url.path();
        }
    }

    QStringList s = res.split("://");
    QString base = url.path();
    QString host = url.host();
    QString user = (url.hasUser()?url.user()+(url.hasPass()?':'+url.pass():""):"");
    if (host.isEmpty()) {
        res=proto+"://"+base;
    } else {
        res = proto+"://"+(user.isEmpty()?"":user+"@")+host+base;
    }
    if (base.isEmpty()) {
        throw svn::ClientException(QString("'")+res+QString("' is not a valid subversion url"));
    }
    return res;
}

bool kio_svnProtocol::createUDSEntry( const QString& filename, const QString& user, long long int size, bool isdir, time_t mtime, KIO::UDSEntry& entry)
{
    entry.insert(KIO::UDSEntry::UDS_NAME,filename);
    entry.insert(KIO::UDSEntry::UDS_FILE_TYPE,isdir ? S_IFDIR : S_IFREG);
    entry.insert(KIO::UDSEntry::UDS_SIZE,size);
    entry.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME,mtime);
    entry.insert(KIO::UDSEntry::UDS_USER,user);
    return true;
}

void kio_svnProtocol::special(const QByteArray& data)
{
    kDebug(9510)<<"kio_svnProtocol::special"<<endl;
    QByteArray tmpData(data);
    QDataStream stream(&tmpData,QIODevice::ReadOnly);
    int tmp;
    stream >> tmp;
    kDebug(9510) << "kio_svnProtocol::special " << tmp << endl;
    switch (tmp) {
        case SVN_CHECKOUT:
        {
            KUrl repository, wc;
            int revnumber;
            QString revkind;
            stream >> repository;
            stream >> wc;
            stream >> revnumber;
            stream >> revkind;
            kDebug(0) << "kio_svnProtocol CHECKOUT from " << repository.url() << " to " << wc.url() << " at " << revnumber << " or " << revkind << endl;
            checkout( repository, wc, revnumber, revkind );
            break;
        }
        case SVN_UPDATE:
        {
            KUrl wc;
            int revnumber;
            QString revkind;
            stream >> wc;
            stream >> revnumber;
            stream >> revkind;
            kDebug(0) << "kio_svnProtocol UPDATE " << wc.url() << " at " << revnumber << " or " << revkind << endl;
            update(wc, revnumber, revkind );
            break;
        }
        case SVN_COMMIT:
        {
            KUrl::List wclist;
            while ( !stream.atEnd() ) {
                KUrl tmp;
                stream >> tmp;
                wclist << tmp;
            }
            kDebug(0) << "kio_svnProtocol COMMIT" << endl;
            commit( wclist );
            break;
        }
        case SVN_LOG:
        {
            kDebug(0) << "kio_svnProtocol LOG" << endl;
            int revstart, revend;
            QString revkindstart, revkindend;
            KUrl::List targets;
            stream >> revstart;
            stream >> revkindstart;
            stream >> revend;
            stream >> revkindend;
            while ( !stream.atEnd() ) {
                KUrl tmp;
                stream >> tmp;
                targets << tmp;
            }
            svnlog( revstart, revkindstart, revend, revkindend, targets );
            break;
        }
        case SVN_IMPORT:
        {
            KUrl wc,repos;
            stream >> repos;
            stream >> wc;
            kDebug(0) << "kio_ksvnProtocol IMPORT" << endl;
            import(repos,wc);
            break;
        }
        case SVN_ADD:
        {
            KUrl wc;
            kDebug(0) << "kio_ksvnProtocol ADD" << endl;
            stream >> wc;
            add(wc);
            break;
        }
        case SVN_DEL:
        {
            KUrl::List wclist;
            while ( !stream.atEnd() ) {
                KUrl tmp;
                stream >> tmp;
                wclist << tmp;
            }
            wc_delete(wclist);
            break;
        }
        case SVN_REVERT:
        {
            KUrl::List wclist;
            while ( !stream.atEnd() ) {
                KUrl tmp;
                stream >> tmp;
                wclist << tmp;
            }
            kDebug(7128) << "kio_svnProtocol REVERT" << endl;
            revert(wclist);
            break;
        }
        case SVN_STATUS:
        {
            KUrl wc;
            bool checkRepos=false;
            bool fullRecurse=false;
            stream >> wc;
            stream >> checkRepos;
            stream >> fullRecurse;
            kDebug(0) << "kio_svnProtocol STATUS" << endl;
            status(wc,checkRepos,fullRecurse);
            break;
        }
        case SVN_MKDIR:
        {
            KUrl::List list;
            stream >> list;
            kDebug(0) << "kio_svnProtocol MKDIR" << endl;
            this->mkdir(list,0);
            break;
        }
        case SVN_RESOLVE:
        {
            KUrl url;
            bool recurse;
            stream >> url;
            stream >> recurse;
            kDebug(7128) << "kio_svnProtocol RESOLVE" << endl;
            wc_resolve(url,recurse);
            break;
        }
        case SVN_SWITCH:
        {
            KUrl wc,url;
            bool recurse;
            int revnumber;
            QString revkind;
            stream >> wc;
            stream >> url;
            stream >> recurse;
            stream >> revnumber;
            stream >> revkind;
            kDebug(7128) << "kio_svnProtocol SWITCH" << endl;
            wc_switch(wc,url,recurse,revnumber,revkind);
            break;
        }
        case SVN_DIFF:
        {
            KUrl url1,url2;
            int rev1, rev2;
            bool recurse;
            QString revkind1, revkind2;
            stream >> url1;
            stream >> url2;
            stream >> rev1;
            stream >> revkind1;
            stream >> rev2;
            stream >> revkind2;
            stream >> recurse;
            diff(url1,url2,rev1,revkind1,rev2,revkind2,recurse);
            break;
        }
        default:
            {kDebug(9510)<<"Unknown special" << endl;}
    }
    finished();
}

void kio_svnProtocol::update(const KUrl&url,int revnumber,const QString&revkind)
{
    svn::Revision where(revnumber,revkind);
    /* update is always local - so make a path instead URI */
    svn::Path p(url.path());
    try {
        svn::Targets pathes(p.path());
        // always update externals, too. (third last parameter)
        // no unversioned items allowed (second last parameter)
        // sticky depth (last parameter)
        m_pData->m_Svnclient->update(pathes, where,svn::DepthInfinity,false,false,true);
    } catch (const svn::ClientException&e) {
        error(KIO::ERR_SLAVE_DEFINED,e.msg());
    }
}

void kio_svnProtocol::status(const KUrl&wc,bool cR,bool rec)
{
    svn::StatusEntries dlist;
    svn::StatusParameter params(wc.path());
    try {
        dlist = m_pData->m_Svnclient->status(params.depth(rec?svn::DepthInfinity:svn::DepthEmpty).all(false).update(cR).noIgnore(false).revision(svn::Revision::UNDEFINED));
    } catch (const svn::ClientException&e) {
        error(KIO::ERR_SLAVE_DEFINED,e.msg());
        return;
    }
    kDebug(9510)<<"Status got " << dlist.count() << " entries." << endl;
    for (long j=0;j<dlist.count();++j) {
        if (!dlist[j]) {
            continue;
        }
        //QDataStream stream(params, QIODevice::WriteOnly);
        setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+"path",dlist[j]->path());
        setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+"text",QString::number(dlist[j]->textStatus()));
        setMetaData(QString::number(m_pData->m_Listener.counter() ).rightJustified( 10,'0' )+ "prop",
                    QString::number(dlist[j]->propStatus()));
        setMetaData(QString::number(m_pData->m_Listener.counter() ).rightJustified( 10,'0' )+ "reptxt",
                    QString::number(dlist[j]->reposTextStatus()));
        setMetaData(QString::number(m_pData->m_Listener.counter() ).rightJustified( 10,'0' )+ "repprop",
                    QString::number(dlist[j]->reposPropStatus()));
        setMetaData(QString::number(m_pData->m_Listener.counter() ).rightJustified( 10,'0' )+ "rev",
                    QString::number(dlist[j]->entry().cmtRev()));
        m_pData->m_Listener.incCounter();
    }
}

void kio_svnProtocol::commit(const KUrl::List&url)
{
    /// @todo replace with direct call to kdesvn?
    QString msg;

    OrgKdeKdesvndInterface kdesvndInterface( "org.kde.kded", "/modules/kdesvnd", QDBusConnection::sessionBus() );
    if(!kdesvndInterface.isValid())
    {
       kWarning() << "Communication with KDED:KdeSvnd failed";
       return;
    }

    QDBusReply<QStringList> res = kdesvndInterface.get_logmsg();
    if ( !res.isValid() )
    {
        kWarning() << "Unexpected reply type";
        return;
    }
    QStringList lt = res;

    if (lt.count()!=1) {
        msg = "Wrong or missing log (may cancel pressed).";
        kDebug(9510)<< msg << endl;
        return;
    }
    msg = lt[0];
    svn::Pathes targets;
    for (long j=0; j<url.count();++j) {
        targets.push_back(svn::Path(url[j].path()));
    }
    svn::Revision nnum=svn::Revision::UNDEFINED;
    try {
        nnum = m_pData->m_Svnclient->commit(svn::Targets(targets),msg,svn::DepthInfinity,false);
    } catch (const svn::ClientException&e) {
        error(KIO::ERR_SLAVE_DEFINED,e.msg());
    }
    for (long j=0;j<url.count();++j) {
        QString userstring;
        if (nnum!=svn::Revision::UNDEFINED) {
            userstring = i18n( "Committed revision %1.",nnum.toString());
        } else {
            userstring = i18n ( "Nothing to commit." );
        }
        setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+ "path", url[j].path() );
        setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+ "action", "0" );
        setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+ "kind", "0" );
        setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+ "mime_t", "" );
        setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+ "content", "0" );
        setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+ "prop", "0" );
        setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+ "rev" , QString::number(nnum) );
        setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+ "string", userstring );
        m_pData->m_Listener.incCounter();
    }
}

void kio_svnProtocol::checkout(const KUrl&src,const KUrl&target,const int rev, const QString&revstring)
{
    svn::Revision where(rev,revstring);
    svn::Revision peg = svn::Revision::UNDEFINED;
    svn::Path _target(target.path());
    try {
        KUrl _src = makeSvnUrl(src);
        m_pData->m_Svnclient->checkout(_src.url(),_target,where,peg,svn::DepthInfinity,false,false);
    } catch (const svn::ClientException&e) {
        error(KIO::ERR_SLAVE_DEFINED,e.msg());
    }
}

void kio_svnProtocol::svnlog(int revstart,const QString&revstringstart,int revend, const QString&revstringend, const KUrl::List&urls)
{
    svn::Revision start(revstart,revstringstart);
    svn::Revision end(revend,revstringend);

    for (long j = 0; j<urls.count();++j) {
        svn::LogEntriesMap logs;
        try {
            m_pData->m_Svnclient->log(makeSvnUrl(urls[j]),start,end,logs,svn::Revision::UNDEFINED,true,true,0);
        } catch (const svn::ClientException&e) {
            error(KIO::ERR_SLAVE_DEFINED,e.msg());
            break;
        }
        if (logs.size()==0) {
            setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified(10,'0')+"path",urls[j].path());
            setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified(10,'0')+"string",
                i18n("Empty logs"));
            m_pData->m_Listener.incCounter();
            continue;
        }

        svn::LogEntriesMap::const_iterator it = logs.begin();
        for (;it!=logs.end();++it) {
            setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+ "path",urls[j].path());
            setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+ "rev",
                QString::number( (*it).revision));
            setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+"author",
                (*it).author);
            setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+"logmessage",
                (*it).message);
            m_pData->m_Listener.incCounter();
            for (long z = 0; z<(*it).changedPaths.count();++z) {
                setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+ "rev",
                    QString::number( (*it).revision));
                setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+ "path",urls[j].path());
                setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+ "loggedpath",
                    (*it).changedPaths[z].path);
                setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+ "loggedaction",
                    QChar((*it).changedPaths[z].action));
                setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+ "loggedcopyfrompath",
                    (*it).changedPaths[z].copyFromPath);
                setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+ "loggedcopyfromrevision",
                    QString::number((*it).changedPaths[z].copyFromRevision));
                m_pData->m_Listener.incCounter();

            }
        }
    }
}

void kio_svnProtocol::revert(const KUrl::List&l)
{
    svn::Pathes list;
    for (long j=0; j<l.count();++j) {
        list.append(svn::Path(l[j].path()));
    }
    svn::Targets target(list);
    try {
        m_pData->m_Svnclient->revert(target,svn::DepthEmpty);
    } catch (const svn::ClientException&e) {
        error(KIO::ERR_SLAVE_DEFINED,e.msg());
    }
}

void kio_svnProtocol::wc_switch(const KUrl&wc,const KUrl&target,bool rec,int rev,const QString&revstring)
{
    svn::Revision where(rev,revstring);
    svn::Path wc_path(wc.path());
    try {
        m_pData->m_Svnclient->doSwitch(wc_path,makeSvnUrl(target.url()),where,rec?svn::DepthInfinity:svn::DepthFiles);
    } catch (const svn::ClientException&e) {
        error(KIO::ERR_SLAVE_DEFINED,e.msg());
    }
}

void kio_svnProtocol::diff(const KUrl&uri1,const KUrl&uri2,int rnum1,const QString&rstring1,int rnum2, const QString&rstring2,bool rec)
{
    QByteArray ex;
    /// @todo read settings for diff (ignore contentype)
    try {
        svn::Revision r1(rnum1,rstring1);
        svn::Revision r2(rnum2,rstring2);
        QString u1 = makeSvnUrl(uri1,true);
        QString u2 = makeSvnUrl(uri2,true);
        KTempDir tdir;
        kDebug(9510) << "kio_ksvn::diff : " << u1 << " at revision " << r1.toString() << " with "
            << u2 << " at revision " << r2.toString()
            << endl ;
        svn::DiffParameter _opts;
        // no peg revision required
        _opts.path1(u1).path2(u2).tmpPath(tdir.name()).
            rev1(r1).rev2(r2).
            ignoreContentType(false).extra(svn::StringArray()).depth(rec?svn::DepthInfinity:svn::DepthEmpty).ignoreAncestry(false).noDiffDeleted(false).
            relativeTo(svn::Path((u1==u2?u1:""))).changeList(svn::StringArray());

        tdir.setAutoRemove(true);
        ex = m_pData->m_Svnclient->diff(_opts);
    } catch (const svn::ClientException&e) {
        error(KIO::ERR_SLAVE_DEFINED,e.msg());
        return;
    }
    QString out = QString::FROMUTF8(ex);
    QTextStream stream(&out);
    while (!stream.atEnd()) {
        setMetaData(QString::number(m_pData->m_Listener.counter()).rightJustified( 10,'0' )+ "diffresult",stream.readLine());
        m_pData->m_Listener.incCounter();
    }
}

void kio_svnProtocol::import(const KUrl& repos, const KUrl& wc)
{
    try {
        QString target = makeSvnUrl(repos);
        QString path = wc.path();
        m_pData->m_Svnclient->import(svn::Path(path),target,QString(),svn::DepthInfinity,false,false);
    } catch (const svn::ClientException&e) {
        error(KIO::ERR_SLAVE_DEFINED,e.msg());
        return;
    }
    finished();
}

void kio_svnProtocol::add(const KUrl& wc)
{
    QString path = wc.path();
    try {
                                               /* rec */
        m_pData->m_Svnclient->add(svn::Path(path),svn::DepthInfinity);
    } catch (const svn::ClientException&e) {
        error(KIO::ERR_SLAVE_DEFINED,e.msg());
        return;
    }
    finished();
}

void kio_svnProtocol::wc_delete(const KUrl::List&l)
{
    svn::Pathes p;
    for (KUrl::List::const_iterator it = l.begin(); it != l.end() ; ++it ) {
        p.append((*it).path());
    }
    try {
        m_pData->m_Svnclient->remove(svn::Targets(p),false);
    } catch (const svn::ClientException&e) {
        error(KIO::ERR_SLAVE_DEFINED,e.msg());
        return;
    }
    finished();
}

void kio_svnProtocol::wc_resolve(const KUrl&url,bool recurse)
{
    try {
        svn::Depth depth=recurse?svn::DepthInfinity:svn::DepthEmpty;
        m_pData->m_Svnclient->resolve(url.path(),depth);
    } catch (const svn::ClientException&e) {
        error(KIO::ERR_SLAVE_DEFINED,e.msg());
        return;
    }
    finished();
}

void kio_svnProtocol::streamWritten(const KIO::filesize_t current)
{
    processedSize(current);
}

void kio_svnProtocol::streamSendMime(KMimeType::Ptr mt)
{
    if (mt) {
        mimeType(mt->name());
    }
}

void kio_svnProtocol::streamPushData(QByteArray array)
{
    data(array);
}

void kio_svnProtocol::contextProgress(long long int current, long long int)
{
    if (m_pData->dispProgress) {
        processedSize(current);
    }
}

void kio_svnProtocol::streamTotalSizeNull()
{
    totalSize(0);
}


/*!
    \fn kio_svnProtocol::getDefaultLog()
 */
QString kio_svnProtocol::getDefaultLog()
{
    QString res;
    Kdesvnsettings::self()->readConfig();
    if (Kdesvnsettings::kio_use_standard_logmsg()) {
        res = Kdesvnsettings::kio_standard_logmsg();
    }
    return res;
}

} // namespace KIO
