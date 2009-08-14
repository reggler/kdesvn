/***************************************************************************
 *   Copyright (C) 2005-2009 by Rajko Albrecht                             *
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
#ifndef THREADCONTEXTLISTENERDATA_H
#define THREADCONTEXTLISTENERDATA_H

#include "src/svnqt/context_listener.hpp"

#include <qthread.h>
#include <qstring.h>

/**
@author Rajko Albrecht
*/
class ThreadContextListenerData{
public:
    ThreadContextListenerData();

    virtual ~ThreadContextListenerData();

    /* sometimes suppress progress messages */
    bool noProgress;

    /* only one callback at time */
    QWaitCondition m_trustpromptWait;

    /* safed due condition above */
    /* this variables are for the event handling across threads */
    /* Trust ssl realm* */
    struct strust_answer {
        svn::ContextListener::SslServerTrustAnswer m_SslTrustAnswer;
        const svn::ContextListener::SslServerTrustData*m_Trustdata;
    };


    /* login into server */
    struct slogin_data
    {
        QString user,password,realm;
        bool maysave,ok;
    };

    struct slog_message
    {
        QString msg;
        bool ok;
        const svn::CommitItemList*_items;
        slog_message(){_items = 0;}
    };

    struct scert_pw
    {
        QString password,realm;
        bool ok,maysave;
    };

    struct scert_file
    {
        QString certfile;
        bool ok;
    };

    struct snotify
    {
        QString msg;
    };
};

#endif
