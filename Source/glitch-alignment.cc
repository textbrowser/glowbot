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
**    derived from Glitch without specific prior written permission.
**
** GLITCH IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** GLITCH, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QtDebug>

#include <limits>

#include "glitch-alignment.h"
#include "glitch-object.h"
#include "glitch-proxy-widget.h"
#include "glitch-scene.h"
#include "glitch-undo-command.h"
#include "glitch-view.h"

static bool x_coordinate_less_than(glitch_object *w1, glitch_object *w2)
{
  if(!w1 || !w2)
    return false;
  else
    return w1->pos().x() < w2->pos().x();
}

static bool y_coordinate_less_than(glitch_object *w1, glitch_object *w2)
{
  if(!w1 || !w2)
    return false;
  else
    return w1->pos().y() < w2->pos().y();
}

glitch_alignment::glitch_alignment(QWidget *parent):QDialog(parent)
{
  m_ui.setupUi(this);
  connect(m_ui.bottom_align,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotAlign(void)));
  connect(m_ui.horizontal_center_align,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotAlign(void)));
  connect(m_ui.horizontal_stack,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotStack(void)));
  connect(m_ui.left_align,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotAlign(void)));
  connect(m_ui.right_align,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotAlign(void)));
  connect(m_ui.top_align,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotAlign(void)));
  connect(m_ui.vertical_center_align,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotAlign(void)));
  connect(m_ui.vertical_stack,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotStack(void)));
  setWindowModality(Qt::NonModal);
}

glitch_alignment::~glitch_alignment()
{
}

void glitch_alignment::align(const AlignmentType alignmentType)
{
  glitch_view *view = qobject_cast<glitch_view *> (parentWidget());

  if(!view)
    return;

  QList<QGraphicsItem *> list(view->scene()->items(Qt::AscendingOrder));

  if(list.isEmpty())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QPair<int, int> maxP;
  QPair<int, int> minP;
  int x = 0;
  int y = 0;

  switch(alignmentType)
    {
    case ALIGN_BOTTOM:
      {
	y = 0;
	break;
      }
    case ALIGN_CENTER_HORIZONTAL:
    case ALIGN_CENTER_VERTICAL:
      {
	maxP.first = maxP.second = 0;
	minP.first = minP.second = std::numeric_limits<int>::max();
	break;
      }
    case ALIGN_LEFT:
      {
	x = std::numeric_limits<int>::max();
	break;
      }
    case ALIGN_RIGHT:
      {
	x = 0;
	break;
      }
    case ALIGN_TOP:
      {
	y = std::numeric_limits<int>::max();
	break;
      }
    default:
      {
	QApplication::restoreOverrideCursor();
	return;
      }
    }

  bool began = false;
  bool firstIteration = true;

 start_label:

  for(int i = 0; i < list.size(); i++)
    {
      glitch_proxy_widget *proxy =
	qgraphicsitem_cast <glitch_proxy_widget *> (list.at(i));

      if(!proxy || !proxy->isSelected())
	continue;

      bool movable = proxy->isMovable();
      glitch_object *widget = qobject_cast<glitch_object *> (proxy->widget());

      if(!widget)
	continue;

      switch(alignmentType)
	{
	case ALIGN_BOTTOM:
	  {
	    x = widget->pos().x();
	    y = qMax(y, widget->height() + widget->pos().y());
	    break;
	  }
	case ALIGN_CENTER_HORIZONTAL:
	case ALIGN_CENTER_VERTICAL:
	  {
	    maxP.first = qMax
	      (maxP.first, widget->pos().x() + widget->width());
	    maxP.second = qMax
	      (maxP.second, widget->height() + widget->pos().y());
	    minP.first = qMin(minP.first, widget->pos().x());
	    minP.second = qMin(minP.second, widget->pos().y());
	    break;
	  }
	case ALIGN_LEFT:
	  {
	    x = qMin(x, widget->pos().x());
	    y = widget->pos().y();
	    break;
	  }
	case ALIGN_RIGHT:
	  {
	    x = qMax(x, widget->pos().x() + widget->width());
	    y = widget->pos().y();
	    break;
	  }
	case ALIGN_TOP:
	  {
	    x = widget->pos().x();
	    y = qMin(y, widget->pos().y());
	    break;
	  }
	default:
	  break;
	}

      if(firstIteration || !movable)
	continue;

      QPoint point;

      switch(alignmentType)
	{
	case ALIGN_BOTTOM:
	  {
	    if(y != widget->height() + widget->pos().y())
	      {
		point = widget->pos();
		widget->move(x, y - widget->height());
	      }

	    break;
	  }
	case ALIGN_CENTER_HORIZONTAL:
	case ALIGN_CENTER_VERTICAL:
	  {
	    QRect rect(QPoint(minP.first, minP.second),
		       QPoint(maxP.first, maxP.second));

	    point = widget->pos();

	    if(alignmentType == ALIGN_CENTER_HORIZONTAL)
	      widget->move
		(widget->pos().x(), rect.center().y() - widget->height() / 2);
	    else
	      widget->move
		(rect.center().x() - widget->width() / 2, widget->pos().y());

	    break;
	  }
	case ALIGN_RIGHT:
	  {
	    if(x != widget->pos().x() + widget->width())
	      {
		point = widget->pos();
		widget->move(x - widget->width(), y);
	      }

	    break;
	  }
	default:
	  {
	    point = widget->pos();
	    widget->move(x, y);
	    break;
	  }
	}

      if(!point.isNull())
	if(point != widget->pos())
	  {
	    if(!began)
	      {
		began = true;
		view->beginMacro(tr("items aligned"));
	      }

	    glitch_undo_command *undoCommand = new glitch_undo_command
	      (QPointF(point),
	       glitch_undo_command::ITEM_MOVED,
	       proxy,
	       view->scene());

	    view->push(undoCommand);
	  }
    }

  if(firstIteration)
    {
      firstIteration = false;
      goto start_label;
    }

  if(began)
    {
      view->endMacro();
      emit changed();
    }

  QApplication::restoreOverrideCursor();
}

void glitch_alignment::slotAlign(void)
{
  QToolButton *toolButton = qobject_cast<QToolButton *> (sender());

  if(m_ui.bottom_align == toolButton)
    align(ALIGN_BOTTOM);
  else if(m_ui.horizontal_center_align == toolButton)
    align(ALIGN_CENTER_HORIZONTAL);
  else if(m_ui.left_align == toolButton)
    align(ALIGN_LEFT);
  else if(m_ui.right_align == toolButton)
    align(ALIGN_RIGHT);
  else if(m_ui.top_align == toolButton)
    align(ALIGN_TOP);
  else if(m_ui.vertical_center_align == toolButton)
    align(ALIGN_CENTER_VERTICAL);
}

void glitch_alignment::slotStack(void)
{
  QToolButton *toolButton = qobject_cast<QToolButton *> (sender());

  if(m_ui.horizontal_stack == toolButton)
    stack(HORIZONTAL_STACK);
  else if(m_ui.vertical_stack == toolButton)
    stack(VERTICAL_STACK);
}

void glitch_alignment::stack(const StackType stackType)
{
  glitch_view *view = qobject_cast<glitch_view *> (parentWidget());

  if(!view)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QList<QGraphicsItem *> list1(view->scene()->selectedItems());

  if(list1.isEmpty())
    {
      QApplication::restoreOverrideCursor();
      return;
    }

  QList<glitch_object *> list2;

  for(int i = 0; i < list1.size(); i++)
    {
      glitch_proxy_widget *proxy = qgraphicsitem_cast
	<glitch_proxy_widget *> (list1.at(i));

      if(!proxy)
	continue;

      glitch_object *widget = qobject_cast<glitch_object *> (proxy->widget());

      if(!widget)
	continue;

      list2 << widget;
    }

  if(list2.isEmpty())
    {
      QApplication::restoreOverrideCursor();
      return;
    }

  if(stackType == HORIZONTAL_STACK)
    qSort(list2.begin(), list2.end(), x_coordinate_less_than);
  else
    qSort(list2.begin(), list2.end(), y_coordinate_less_than);

  bool began = false;
  int coordinate = 0;

  if(stackType == HORIZONTAL_STACK)
    coordinate = list2.at(0)->pos().x();
  else
    coordinate = list2.at(0)->pos().y();

  for(int i = 0; i < list2.size(); i++)
    {
      glitch_object *widget = list2.at(i);

      if(!widget)
	continue;

      QPoint point;

      if(stackType == HORIZONTAL_STACK)
        {
	  if(widget->property("movable").toBool())
	    {
	      point = widget->pos();
	      widget->move(coordinate, widget->pos().y());
	    }

	  coordinate += widget->width();
        }
      else
	{
	  if(widget->property("movable").toBool())
	    {
	      point = widget->pos();
	      widget->move(widget->pos().x(), coordinate);
	    }

	  coordinate += widget->height();
	}

      if(!point.isNull())
	if(point != widget->pos())
	  {
	    if(!began)
	      {
		began = true;
		view->beginMacro(tr("items stacked"));
	      }

	    glitch_undo_command *undoCommand = new glitch_undo_command
	      (QPointF(point),
	       glitch_undo_command::ITEM_MOVED,
	       widget->proxy(),
	       view->scene());

	    view->push(undoCommand);
	  }
    }

  if(began)
    {
      view->endMacro();
      emit changed();
    }

  QApplication::restoreOverrideCursor();
}