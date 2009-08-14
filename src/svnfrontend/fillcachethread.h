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
#ifndef FILLCACHE_THREAD_H
#define FILLCACHE_THREAD_H

#include "src/svnqt/client.hpp"
#include "src/svnqt/revision.hpp"
#include "src/svnqt/status.hpp"
#include "ccontextlistener.h"
#include "eventnumbers.h"
#include "frontendtypes.h"

#include <qthread.h>
#include <qevent.h>

class QObject;

class FillCacheThread:public QThread
{
public:
    FillCacheThread(QObject*,const QString&reposRoot);
    virtual ~FillCacheThread();
    virtual void run();
    virtual void cancelMe();

    const QString&reposRoot()const;

protected:
    QMutex mutex;
    svn::Client* m_Svnclient;
    svn::ContextP m_CurrentContext;
    svn::smart_pointer<ThreadContextListener> m_SvnContextListener;
    QObject*m_Parent;
    QString m_what;
};

#endif
