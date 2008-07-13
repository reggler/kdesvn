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
#include "ccontextlistener.h"
#include "src/settings/kdesvnsettings.h"
#include "src/ksvnwidgets/authdialogimpl.h"
#include "src/ksvnwidgets/logmsg_impl.h"
#include "src/ksvnwidgets/ssltrustprompt_impl.h"
#include "src/ksvnwidgets/pwstorage.h"

#include <klocale.h>
#include <kapp.h>
#include <kinputdialog.h>
#include <kpassdlg.h>
#include <kdebug.h>
#include <kfiledialog.h>

#include <qtextstream.h>
#include <qthread.h>

class CContextListenerData
{
public:
    CContextListenerData();
    virtual ~CContextListenerData();

    // data
    bool m_cancelMe;
    QMutex m_CancelMutex;
    bool loginDialogRunning;
    bool noDialogs;
};

class simpleBoolCheck
{
private:
    bool*which;
public:
    simpleBoolCheck(bool*val):which(val)
    {
        while(*which == true) {
            sleep(1);
        }
        *which = true;
    }
    ~simpleBoolCheck()
    {
        *which = false;
    }
};

CContextListenerData::CContextListenerData()
    : m_cancelMe(false),m_CancelMutex(),loginDialogRunning(false),noDialogs(false)
{
}

CContextListenerData::~CContextListenerData()
{
}

const int CContextListener::smax_actionstring=svn_wc_notify_failed_unlock+1;

const QString CContextListener::action_strings[]={
    I18N_NOOP("Add to revision control"),
    I18N_NOOP("Copy"),
    I18N_NOOP("Delete"),
    I18N_NOOP("Restore missing"),
    I18N_NOOP("Revert"),
    I18N_NOOP("Revert failed"),
    I18N_NOOP("Resolved"),
    I18N_NOOP("Skip"),
    I18N_NOOP("Deleted"),
    I18N_NOOP("Added"),
    I18N_NOOP("Update"), //svn_wc_notify_update_update
    I18N_NOOP("Update complete"),
    I18N_NOOP("Update external module"),
    QString::null, // status completed - will not send is just noisy
    I18N_NOOP("Status on external"), //svn_wc_notify_status_external
    I18N_NOOP("Commit Modified"),
    I18N_NOOP("Commit Added"),
    I18N_NOOP("Commit Deleted"),
    I18N_NOOP("Commit Replaced"),
    QString::null, //tx delta -> making ticks instead
    QString::null, //svn_wc_notify_blame_revision - using ticks
    I18N_NOOP("Locking"),
    I18N_NOOP("Unlocked"),
    I18N_NOOP("Lock failed"),
    I18N_NOOP("Unlock failed")
};

const QString CContextListener::notify_state_strings[]={
    QString::null, // = 0
    QString::null,
    I18N_NOOP("unchanged"),
    I18N_NOOP("item wasn't present"),
    I18N_NOOP("unversioned item obstructed work"),
    // I18N_NOOP("Pristine state was modified."), // should send a signal with path instead of message?
    QString::null,
    I18N_NOOP("Modified state had mods merged in."),
    I18N_NOOP("Modified state got conflicting mods.")
};

QString CContextListener::NotifyAction(svn_wc_notify_action_t action)
{
    if (action>=smax_actionstring||action<0) {
        return QString::null;
    }
    return action_strings[action].isEmpty()?QString::null:i18n(action_strings[action]);
}

QString CContextListener::NotifyState(svn_wc_notify_state_t state)
{
    if (state > svn_wc_notify_state_conflicted || state<0) return QString::null;
    return notify_state_strings[state].isEmpty()?QString::null:i18n(notify_state_strings[state]);
}

CContextListener::CContextListener(QObject *parent, const char *name)
 : QObject(parent, name), svn::ContextListener(),ref_count()
{
    m_Data = new CContextListenerData();
}

CContextListener::~CContextListener()
{
    disconnect();
    delete m_Data;
}

bool CContextListener::contextGetSavedLogin (const QString & realm,QString & username,QString & password)
{
    simpleBoolCheck(&(m_Data->loginDialogRunning));
    return PwStorage::self()->getLogin(realm,username,password);
}

bool CContextListener::contextGetLogin (
                    const QString & realm,
                    QString & username,
                    QString & password,
                    bool & maySave)
{
    maySave = false;
    //simpleBoolCheck(&(m_Data->loginDialogRunning));
    kdDebug()<<"Context get login"<<endl;
    emit waitShow(true);
    emit sendNotify(realm);
    AuthDialogImpl auth(realm,username);
    if (auth.exec()==QDialog::Accepted) {
        username=auth.Username();
        password=auth.Password();
        maySave = (Kdesvnsettings::passwords_in_wallet()?false:auth.maySave());
        if (Kdesvnsettings::passwords_in_wallet() && auth.maySave()) {
            PwStorage::self()->setLogin(realm,username,password);
        }
        emit waitShow(false);
        return true;
    }
    emit waitShow(false);
    return false;
}

void CContextListener::contextNotify(const QString&aMsg)
{
    if (aMsg.isEmpty()) {
        emit tickProgress();
    } else {
        emit sendNotify(aMsg);
    }
}

void CContextListener::contextNotify (const char *path,
                    svn_wc_notify_action_t action,
                    svn_node_kind_t /* kind */,
                    const char *mime_type,
                    svn_wc_notify_state_t content_state,
                    svn_wc_notify_state_t prop_state,
                    svn_revnum_t revision)
{
    QString msg;
    QString aString = NotifyAction(action);

    if (!aString.isEmpty()) {
        QTextStream ts(&msg,IO_WriteOnly);
        ts << NotifyAction(action) << " " << QString::FROMUTF8(path);
        if (revision>-1) {
            ts << " (Rev "<<revision<<")";
        }
        aString = NotifyState(content_state);
        if (!aString.isEmpty()) {
            ts << "\n" << aString;
        }
    }
    contextNotify(msg);
}

void CContextListener::contextNotify (const svn_wc_notify_t *action)
{
    if (!action) return;
//    if (action->action<svn_wc_notify_locked) {
        contextNotify(action->path,action->action,action->kind,action->mime_type,
            action->content_state,action->prop_state,action->revision);
//        return;
//    }
//    QString aString = NotifyAction(action->action);
}

void CContextListener::sendTick()
{
    emit tickProgress();
}

bool CContextListener::contextCancel()
{
    {
        QMutexLocker lock(&(m_Data->m_CancelMutex));
        if (m_Data->m_cancelMe) {
            m_Data->m_cancelMe=false;
            return true;
        }
    }
    // otherwise deadlock!
    sendTick();
    return false;
}

bool CContextListener::contextGetLogMessage (QString & msg,const svn::CommitItemList&items)
{
    bool isOk = false;
    emit waitShow(true);
    QString logMessage = Logmsg_impl::getLogmessage(items,&isOk,0,0,0);
    if (isOk) {
        msg = logMessage;
    }
    emit waitShow(false);
    return isOk;
}

svn::ContextListener::SslServerTrustAnswer CContextListener::contextSslServerTrustPrompt (
    const svn::ContextListener::SslServerTrustData & data , apr_uint32_t & acceptedFailures )
{
    bool ok,saveit;
    emit waitShow(true);
    if (!SslTrustPrompt_impl::sslTrust(
        data.hostname,
        data.fingerprint,
        data.validFrom,
        data.validUntil,
        data.issuerDName,
        data.realm,
        failure2Strings(acceptedFailures),
        &ok,&saveit)) {
        return DONT_ACCEPT;
    }
    emit waitShow(false);
    if (!saveit) {
        return ACCEPT_TEMPORARILY;
    }
    return ACCEPT_PERMANENTLY;
}

bool CContextListener::contextSslClientCertPrompt (QString & certFile)
{
    kdDebug()<<"CContextListener::contextSslClientCertPrompt "
        << certFile << endl;
    emit waitShow(true);
    QString afile = KFileDialog::getOpenFileName(QString::null,
        QString::null,
        0,
        i18n("Open a file with a #PKCS12 certificate"));
    emit waitShow(false);
    if (afile.isEmpty()) {
        return false;
    }
    certFile = afile;
    return true;
}

bool CContextListener::contextLoadSslClientCertPw(QString&password,const QString&realm)
{
    return PwStorage::self()->getCertPw(realm,password);
}

bool CContextListener::contextSslClientCertPwPrompt (QString & password,
                                   const QString & realm, bool & maysave)
{
    maysave = false;
    emit waitShow(true);
    QCString npass;
    int keep = 1;
    int res = KPasswordDialog::getPassword(npass,
        i18n("Enter password for realm %1").arg(realm),
        &keep);
    emit waitShow(false);
    if (res!=KPasswordDialog::Accepted) {
        return false;
    }
    maysave = (Kdesvnsettings::passwords_in_wallet()?false:keep!=0);
    if (Kdesvnsettings::store_passwords() && keep) {
        PwStorage::self()->setCertPw(realm,password);
    }
    password = npass;
    return true;
}

void CContextListener::setCanceled(bool how)
{
    QMutexLocker lock(&(m_Data->m_CancelMutex));
    m_Data->m_cancelMe = how;
}

QStringList CContextListener::failure2Strings(apr_uint32_t acceptedFailures)
{
    QStringList res;
    if (acceptedFailures&SVN_AUTH_SSL_UNKNOWNCA) {
        res << i18n("The certificate is not issued by a trusted authority. Use the fingerprint to validate the certificate manually!");
    }
    if (acceptedFailures&SVN_AUTH_SSL_CNMISMATCH) {
        res<< i18n("The certificate hostname does not match.");
    }
    if (acceptedFailures&SVN_AUTH_SSL_NOTYETVALID) {
        res << i18n("The certificate is not yet valid.");
    }
    if (acceptedFailures& SVN_AUTH_SSL_EXPIRED) {
        res << i18n("The certificate has expired.");
    }
    if (acceptedFailures&SVN_AUTH_SSL_OTHER) {
        res << i18n("The certificate has an unknown error.");
    }
    return res;
}

QString CContextListener::translate(const QString&what)
{
    return i18n(what);
}

/*!
    \fn CContextListener::contextProgress(long long int current, long long int max)
 */
void CContextListener::contextProgress(long long int current, long long int max)
{
    emit netProgress(current,max);
}

#include "ccontextlistener.moc"
