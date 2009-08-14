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
#ifndef GRAPHTREELABEL_H
#define GRAPHTREELABEL_H

#include "graphtree/drawparams.h"

#include <QGraphicsRectItem>
#include <QPixmap>

/**
	@author Rajko Albrecht <ral@alwins-world.de>
*/
class GraphTreeLabel : public QGraphicsRectItem,StoredDrawParams
{
public:
    GraphTreeLabel(const QString&,const QString&,const QRectF&r,QGraphicsItem*p=0);
    virtual ~GraphTreeLabel();

    virtual int type()const;
    //virtual void drawShape(QPainter& p);
    virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);

    void setBgColor(const QColor&);

    const QString&nodename()const;
    const QString&source()const;
    void setSource(const QString&);
    virtual void setSelected(bool);

protected:
    QString m_Nodename;
    QString m_SourceNode;
};

class GraphEdge;

class GraphEdgeArrow:public QGraphicsPolygonItem
{
public:
    GraphEdgeArrow(GraphEdge*,QGraphicsItem*p=0);
    GraphEdge*edge();
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);
    virtual int type()const;

private:
    GraphEdge*_edge;
};

/* line */
class GraphEdge:public QGraphicsPathItem
{
public:
    GraphEdge(QGraphicsItem*p=0);
    virtual ~GraphEdge();

    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);
    const QPolygonF& controlPoints()const;
    void setControlPoints(const QPolygonF& a);
    virtual int type()const;

private:
    QPolygonF _points;
};

class GraphMark:public QGraphicsRectItem
{
public:
    GraphMark(GraphTreeLabel*,QGraphicsItem*p=0);
    virtual ~GraphMark();
    virtual int type()const;
    virtual bool hit(const QPoint&)const;
    virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);

private:
    static QPixmap*_p;
};

#endif
