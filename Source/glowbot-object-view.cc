/*
** Copyright (c) 2004 - 10^10^10, Alexis Megas.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from GlowBot without specific prior written permission.
**
** GLOWBOT IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** GLOWBOT, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QResizeEvent>
#include <QScrollBar>
#include <QSqlError>

#include "glowbot-alignment.h"
#include "glowbot-object.h"
#include "glowbot-object-view.h"
#include "glowbot-proxy-widget.h"
#include "glowbot-scene.h"

static const int s_scene_rect_fuzzy = 4;

glowbot_object_view::glowbot_object_view
(const quint64 id, QWidget *parent):QGraphicsView(parent)
{
  m_alignment = new glowbot_alignment(this);
  m_id = id;
  m_scene = new glowbot_scene(this);
  setBackgroundBrush(QBrush(QColor(211, 211, 211), Qt::SolidPattern));
  setDragMode(QGraphicsView::RubberBandDrag);
  setInteractive(true);
  setRenderHints(QPainter::Antialiasing |
		 QPainter::HighQualityAntialiasing | // OpenGL?
		 QPainter::SmoothPixmapTransform |
		 QPainter::TextAntialiasing);
  setRubberBandSelectionMode(Qt::IntersectsItemShape);
  setScene(m_scene);
  setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  connect(m_scene,
	  SIGNAL(changed(void)),
	  this,
	  SIGNAL(changed(void)));
  connect(this,
	  SIGNAL(customContextMenuRequested(const QPoint &)),
	  this,
	  SLOT(slotCustomContextMenuRequested(const QPoint &)));
}

glowbot_object_view::~glowbot_object_view()
{
}

quint64 glowbot_object_view::id(void) const
{
  return m_id;
}

void glowbot_object_view::contextMenuEvent(QContextMenuEvent *event)
{
  if(event && items(event->pos()).isEmpty())
    {
      event->ignore();
      emit customContextMenuRequested(event->pos());
    }
  else
    QGraphicsView::contextMenuEvent(event);
}

void glowbot_object_view::resizeEvent(QResizeEvent *event)
{
  if(event)
    {
      QRectF b(m_scene->itemsBoundingRect());

      m_scene->setSceneRect
      (0,
       0,
       qMax(static_cast<int> (b.width()),
	    event->size().width() - s_scene_rect_fuzzy),
       qMax(static_cast<int> (b.height()),
	    event->size().height() - (horizontalScrollBar() ?
				      horizontalScrollBar()->height() : 0) -
	    s_scene_rect_fuzzy));
    }

  QGraphicsView::resizeEvent(event);
}

void glowbot_object_view::save(const QSqlDatabase &db, QString &error)
{
  /*
  ** Save the children!
  */

  QList<QGraphicsItem *> list(m_scene->items());

  for(int i = 0; i < list.size(); i++)
    {
      glowbot_proxy_widget *proxy =
	qgraphicsitem_cast<glowbot_proxy_widget *> (list.at(i));

      if(!proxy)
	continue;

      glowbot_object *widget = qobject_cast<glowbot_object *>
	(proxy->widget());

      if(!widget)
	continue;

      widget->save(db, error);

      if(!error.isEmpty())
	break;
    }
}

void glowbot_object_view::slotCustomContextMenuRequested(const QPoint &point)
{
  QMenu menu(this);

  menu.addAction(tr("&Alignment..."),
		 m_alignment,
		 SLOT(show(void)));
  menu.exec(mapToGlobal(point));
}
