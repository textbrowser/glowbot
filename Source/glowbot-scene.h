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

#ifndef _glowbot_scene_h_
#define _glowbot_scene_h_

#include <QGraphicsScene>

#include "glowbot-common.h"

class QUndoStack;
class glowbot_object;
class glowbot_proxy_widget;

class glowbot_scene: public QGraphicsScene
{
  Q_OBJECT

 public:
  glowbot_scene(const glowbot_common::ProjectType projectType, QObject *parent);
  ~glowbot_scene();
  QList<glowbot_object *> objects(void) const;
  QList<glowbot_object *> selectedObjects(void) const;
  glowbot_proxy_widget *addObject(const QPointF &point, glowbot_object *object);
  void deleteItems(void);
  void setMainScene(const bool state);
  void setUndoStack(QUndoStack *undoStack);

 private:
  QList<QPair<QPointF, glowbot_proxy_widget *> > m_movedPoints;
  QPointF m_lastScenePos;
  QUndoStack *m_undoStack;
  bool m_mainScene;
  glowbot_common::ProjectType m_projectType;
  bool allowDrag(QGraphicsSceneDragDropEvent *event, const QString &text);
  void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
  void dragMoveEvent(QGraphicsSceneDragDropEvent *event);
  void drawBackground(QPainter *painter, const QRectF &rect);
  void dropEvent(QGraphicsSceneDragDropEvent *event);
  void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  void mousePressEvent(QGraphicsSceneMouseEvent *event);
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

 private slots:
  void slotObjectDeletedViaContextMenu(void);

 signals:
  void changed(void);
  void functionAdded(const QString &name);
  void functionNameChanged(const QString &before, const QString &after);
  void sceneResized(void);
};

#endif
