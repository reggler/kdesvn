/***************************************************************************
 *   Copyright (C) 2006-2009 by Rajko Albrecht                             *
 *   ral@alwins-world.de                                                   *
 *                                                                         *
 * This program is free software; you can redistribute it and/or           *
 * modify it under the terms of the GNU Lesser General Public              *
 * License as published by the Free Software Foundation; either            *
 * version 2.1 of the License, or (at your option) any later version.      *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 * Lesser General Public License for more details.                         *
 *                                                                         *
 * You should have received a copy of the GNU Lesser General Public        *
 * License along with this program (in the file LGPL.txt); if not,         *
 * write to the Free Software Foundation, Inc., 51 Franklin St,            *
 * Fifth Floor, Boston, MA  02110-1301  USA                                *
 *                                                                         *
 * This software consists of voluntary contributions made by many          *
 * individuals.  For exact contribution history, see the revision          *
 * history and logs, available at http://kdesvn.alwins-world.de.           *
 ***************************************************************************/
#include "svnfrontend/createrepo_impl.h"
#include "src/svnqt/version_check.hpp"
#include "src/svnqt/repoparameter.hpp"

#include <kdebug.h>
#include <kurl.h>
#include <kurlrequester.h>
#include <kcombobox.h>

#include <qcheckbox.h>

class RecurseCheck
{
    bool&value;
public:
    RecurseCheck(bool&aValue):value(aValue){ value=true;}
    ~RecurseCheck(){value = false;}
};

struct CreateRepoData
{
    bool inChangeCompat;
    mutable svn::repository::CreateRepoParameter params;
};

Createrepo_impl::Createrepo_impl(QWidget *parent, const char *name)
//     :CreateRepo_Dlg(parent, name)
    : QWidget(parent)
{
    setupUi(this);
    if (name) {
        setObjectName(name);
    } else {
        setObjectName("Createrepo_impl");
    }
    m_ReposPathinput->setMode(KFile::Directory|KFile::LocalOnly);
    _data = new CreateRepoData;

   _data->inChangeCompat=true;
    m_DisableFsync->setEnabled(false);
    m_LogKeep->setEnabled(false);

    if ( svn::Version::version_major()>1||svn::Version::version_minor()>3 ){
        m_svn13compat->setEnabled(true);
    } else {
        m_svn13compat->setEnabled(false);
        m_svn13compat->hide();
    }
    if (svn::Version::version_major()>1||svn::Version::version_minor()>4){
        m_svn14compat->setEnabled(true);
    } else {
        m_svn14compat->setEnabled(false);
        m_svn14compat->hide();
    }
    if (svn::Version::version_major()>1||svn::Version::version_minor()>5){
        m_svn15compat->setEnabled(true);
    } else {
        m_svn15compat->setEnabled(false);
        m_svn15compat->hide();
    }

   _data->inChangeCompat=false;
}

void Createrepo_impl::fsTypeChanged(int which)
{
    m_DisableFsync->setEnabled(which==1);
    m_LogKeep->setEnabled(which==1);
}

QString Createrepo_impl::targetDir()const
{
    KUrl u = m_ReposPathinput->url();
    QString res = u.path();
    while (res.endsWith('/')) {
        res.truncate(res.length()-1);
    }
    return res;
}

bool Createrepo_impl::createMain()const
{
    return m_CreateMainDirs->isChecked();
}

void Createrepo_impl::compatChanged15(bool)
{
    if ((_data->inChangeCompat)) {
        return;
    }
    RecurseCheck rc((_data->inChangeCompat));
    if (m_svn15compat->isChecked()) {
        m_svn13compat->setChecked(false);
        m_svn14compat->setChecked(false);
    }
}

void Createrepo_impl::compatChanged14(bool)
{
    if ((_data->inChangeCompat)) {
        return;
    }
    RecurseCheck rc((_data->inChangeCompat));
    if (m_svn14compat->isChecked()) {
        if (m_svn15compat->isEnabled()) {
            m_svn15compat->setChecked(false);
        }
        m_svn13compat->setChecked(false);
    }
}

void Createrepo_impl::compatChanged13(bool)
{
    if ((_data->inChangeCompat)) {
        return;
    }
    RecurseCheck rc((_data->inChangeCompat));
    if (m_svn13compat->isChecked()) {
        if (m_svn14compat->isEnabled()) {
            m_svn14compat->setChecked(false);
        }
        if (m_svn15compat->isEnabled()) {
            m_svn15compat->setChecked(false);
        }
    }
}

const svn::repository::CreateRepoParameter&Createrepo_impl::parameter()const
{
    _data->params.path(targetDir());
    _data->params.pre14_compat((m_svn13compat->isChecked()||!m_svn13compat->isEnabled()));
    _data->params.pre15_compat((m_svn14compat->isChecked()||!m_svn14compat->isEnabled()));
    _data->params.fstype(m_FilesystemSelector->currentText());
    _data->params.bdbnosync(m_DisableFsync->isChecked());
    _data->params.bdbautologremove(!m_LogKeep->isChecked());
    return _data->params;
}

#include "createrepo_impl.moc"
