/***************************************************************************
 *   Copyright (C) 2005 by Rajko Albrecht                                  *
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
#ifndef REVISIONTREE_H
#define REVISIONTREE_H

#include "svnqt/log_entry.hpp"
#include "svnqt/revision.hpp"
#include "svnqt/client.hpp"

#include <qstring.h>
#include <qmap.h>
#include <qpixmap.h>

class RtreeData;
class QWidget;
class KListViewItem;
class KListView;
class CContextListener;

namespace svn
{
    class Client;
}

/**
	@author Rajko Albrecht <ral@alwins-world.de>
*/
class RevisionTree{
public:
    RevisionTree(svn::Client*,
        CContextListener*aListener,
        const QString& reposRoot,
        const svn::Revision&startr,const svn::Revision&endr,
        const QString&,const svn::Revision& baserevision,QWidget*treeParent,
        QWidget*parent=0);
    virtual ~RevisionTree();

    bool isValid()const;
    QWidget*getView();

protected:
    long m_Baserevision;
    long m_InitialRevsion;
    QString m_Path;
    bool m_Valid;

    RtreeData*m_Data;

    bool topDownScan();
    bool bottomUpScan(long startrev,unsigned recurse,const QString&path,long sRev = -1);
    bool isDeleted(long revision,const QString&);

    static bool isParent(const QString&_par,const QString&tar);

    void fillItem(long revIndex,int pathIndex,const QString&nodeName,const QString&path);
};

#endif
