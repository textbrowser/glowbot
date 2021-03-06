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

#include <QShortcut>
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
  m_ui.bottom_align->setFocus();
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
  new QShortcut(tr("Ctrl+W"),
		this,
		SLOT(close(void)));
  setWindowModality(Qt::NonModal);
}

glitch_alignment::~glitch_alignment()
{
}

void glitch_alignment::align(const AlignmentType alignmentType)
{
  auto view = qobject_cast<glitch_view *> (parentWidget());

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

  for(auto i : list)
    {
      auto proxy = qgraphicsitem_cast <glitch_proxy_widget *> (i);

      if(!proxy || !proxy->isSelected())
	continue;

      auto object = qobject_cast<glitch_object *> (proxy->widget());

      if(!object)
	continue;

      bool movable = proxy->isMovable();

      switch(alignmentType)
	{
	case ALIGN_BOTTOM:
	  {
	    x = object->pos().x();
	    y = qMax(y, object->height() + object->pos().y());
	    break;
	  }
	case ALIGN_CENTER_HORIZONTAL:
	case ALIGN_CENTER_VERTICAL:
	  {
	    maxP.first = qMax
	      (maxP.first, object->pos().x() + object->width());
	    maxP.second = qMax
	      (maxP.second, object->height() + object->pos().y());
	    minP.first = qMin(minP.first, object->pos().x());
	    minP.second = qMin(minP.second, object->pos().y());
	    break;
	  }
	case ALIGN_LEFT:
	  {
	    x = qMin(x, object->pos().x());
	    y = object->pos().y();
	    break;
	  }
	case ALIGN_RIGHT:
	  {
	    x = qMax(x, object->pos().x() + object->width());
	    y = object->pos().y();
	    break;
	  }
	case ALIGN_TOP:
	  {
	    x = object->pos().x();
	    y = qMin(y, object->pos().y());
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
	    if(y != object->height() + object->pos().y())
	      {
		point = object->pos();
		object->move(x, y - object->height());
	      }

	    break;
	  }
	case ALIGN_CENTER_HORIZONTAL:
	case ALIGN_CENTER_VERTICAL:
	  {
	    QRect rect(QPoint(minP.first, minP.second),
		       QPoint(maxP.first, maxP.second));

	    point = object->pos();

	    if(alignmentType == ALIGN_CENTER_HORIZONTAL)
	      object->move
		(object->pos().x(), rect.center().y() - object->height() / 2);
	    else
	      object->move
		(rect.center().x() - object->width() / 2, object->pos().y());

	    break;
	  }
	case ALIGN_RIGHT:
	  {
	    if(x != object->pos().x() + object->width())
	      {
		point = object->pos();
		object->move(x - object->width(), y);
	      }

	    break;
	  }
	default:
	  {
	    point = object->pos();
	    object->move(x, y);
	    break;
	  }
	}

      if(!point.isNull())
	if(object->pos() != point)
	  {
	    if(!began)
	      {
		began = true;
		view->beginMacro(tr("items aligned"));
	      }

	    auto undoCommand = new glitch_undo_command
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
  auto toolButton = qobject_cast<QToolButton *> (sender());

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
  auto toolButton = qobject_cast<QToolButton *> (sender());

  if(m_ui.horizontal_stack == toolButton)
    stack(HORIZONTAL_STACK);
  else if(m_ui.vertical_stack == toolButton)
    stack(VERTICAL_STACK);
}

void glitch_alignment::stack(const StackType stackType)
{
  auto view = qobject_cast<glitch_view *> (parentWidget());

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

  for(auto i : list1)
    {
      auto proxy = qgraphicsitem_cast<glitch_proxy_widget *> (i);

      if(!proxy)
	continue;

      auto object = qobject_cast<glitch_object *> (proxy->widget());

      if(!object)
	continue;

      list2 << object;
    }

  if(list2.isEmpty())
    {
      QApplication::restoreOverrideCursor();
      return;
    }

  if(stackType == HORIZONTAL_STACK)
    std::sort(list2.begin(), list2.end(), x_coordinate_less_than);
  else
    std::sort(list2.begin(), list2.end(), y_coordinate_less_than);

  bool began = false;
  int coordinate = 0;

  if(stackType == HORIZONTAL_STACK)
    coordinate = list2.at(0)->pos().x();
  else
    coordinate = list2.at(0)->pos().y();

  for(auto widget : list2)
    {
      if(!widget || !widget->proxy())
	continue;

      QPoint point;

      if(stackType == HORIZONTAL_STACK)
        {
	  if(widget->proxy()->isMovable())
	    {
	      point = widget->pos();
	      widget->move(coordinate, widget->pos().y());
	    }

	  coordinate += widget->width();
        }
      else
	{
	  if(widget->proxy()->isMovable())
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

	    auto undoCommand = new glitch_undo_command
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
