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

#include <QColorDialog>
#include <QDir>
#include <QSettings>
#include <QShortcut>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include "glitch-canvas-settings.h"
#include "glitch-common.h"
#include "glitch-misc.h"
#include "glitch-view.h"

glitch_canvas_settings::glitch_canvas_settings(QWidget *parent):
  QDialog(parent)
{
  m_ui.setupUi(this);
  m_ui.background_color->setStyleSheet
    (QString("QPushButton {background-color: %1}").
     arg(QColor(211, 211, 211).name()));
  m_ui.background_color->setText(QColor(211, 211, 211).name());
  new QShortcut(tr("Ctrl+W"),
		this,
		SLOT(close(void)));
  connect(m_ui.background_color,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotSelectBackgroundColor(void)));
  connect(m_ui.buttonBox->button(QDialogButtonBox::Apply),
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(accept(void)));
  setWindowModality(Qt::NonModal);
}

glitch_canvas_settings::~glitch_canvas_settings()
{
}

QColor glitch_canvas_settings::canvasBackgroundColor(void) const
{
  return QColor(m_ui.background_color->text());
}

QGraphicsView::ViewportUpdateMode glitch_canvas_settings::
viewportUpdateMode(void) const
{
  switch(m_ui.update_mode->currentIndex())
    {
    case 0:
      return QGraphicsView::BoundingRectViewportUpdate;
    case 1:
      return QGraphicsView::FullViewportUpdate;
    case 2:
      return QGraphicsView::MinimalViewportUpdate;
    case 3:
      return QGraphicsView::SmartViewportUpdate;
    default:
      return QGraphicsView::FullViewportUpdate;
    }
}

QHash<glitch_canvas_settings::Settings, QVariant> glitch_canvas_settings::
settings(void) const
{
  QHash<Settings, QVariant> hash;

  hash[CANVAS_BACKGROUND_COLOR] = canvasBackgroundColor().name();
  hash[CANVAS_NAME] = name();
  hash[REDO_UNDO_STACK_SIZE] = redoUndoStackSize();
  hash[VIEW_UPDATE_MODE] = viewportUpdateMode();
  return hash;
}

QString glitch_canvas_settings::defaultName(void) const
{
  if(m_ui.project_type->currentText() == tr("Arduino"))
    return "Arduino-Diagram";
  else
    return "Diagram";
}

QString glitch_canvas_settings::name(void) const
{
  return m_ui.name->text().trimmed();
}

bool glitch_canvas_settings::save(QString &error) const
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString connectionName("");
  bool ok = false;

  {
    QSqlDatabase db(glitch_common::sqliteDatabase());

    connectionName = db.connectionName();
    db.setDatabaseName(m_fileName);

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec
	  ("CREATE TABLE IF NOT EXISTS canvas_settings ("
	   "background_color TEXT NOT NULL, "
	   "name TEXT NOT NULL PRIMARY KEY, "
	   "project_type TEXT NOT NULL CHECK "
	   "(project_type IN ('Arduino')), "
	   "redo_undo_stack_size INTEGER NOT NULL DEFAULT 500, "
	   "update_mode TEXT NOT NULL CHECK "
	   "(update_mode IN ('bounding_rectangle', 'full', 'minimal', "
	   "'smart'))"
	   ")");

	if(!(ok = query.exec("DELETE FROM canvas_settings")))
	  {
	    error = query.lastError().text();
	    goto done_label;
	  }

	query.prepare
	  ("INSERT OR REPLACE INTO canvas_settings "
	   "(background_color, "
	   "name, "
	   "project_type, "
	   "redo_undo_stack_size, "
	   "update_mode) "
	   "VALUES (?, ?, ?, ?, ?)");
	query.addBindValue(m_ui.background_color->text());

	QString name(m_ui.name->text().trimmed());

	if(name.isEmpty())
	  name = defaultName();

	query.addBindValue(name);
	query.addBindValue(m_ui.project_type->currentText());
	query.addBindValue(m_ui.redo_undo_stack_size->value());
	query.addBindValue
	  (m_ui.update_mode->currentText().toLower().replace(' ', '_'));

	if(!(ok = query.exec()))
	  error = query.lastError().text();
      }
    else
      error = db.lastError().text();

  done_label:
    db.close();
  }

  error = error.trimmed();
  glitch_common::discardDatabase(connectionName);
  QApplication::restoreOverrideCursor();
  return ok;
}

int glitch_canvas_settings::redoUndoStackSize(void) const
{
  return m_ui.redo_undo_stack_size->value();
}

void glitch_canvas_settings::accept(void)
{
  QString name(m_ui.name->text().trimmed());

  if(name.isEmpty())
    m_ui.name->setText(defaultName());

  setResult(QDialog::Accepted);
  emit accepted(true);
}

void glitch_canvas_settings::prepare(void)
{
  QFileInfo fileInfo(m_fileName);

  if(!fileInfo.isReadable())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString connectionName("");

  {
    QSqlDatabase db(glitch_common::sqliteDatabase());

    connectionName = db.connectionName();
    db.setDatabaseName(m_fileName);

    if(db.open())
      {
	QSqlQuery query(db);

	if(query.exec("SELECT background_color, " // 0
		      "name, "                    // 1
		      "project_type, "            // 2
		      "redo_undo_stack_size, "    // 3
		      "update_mode "              // 4
		      "FROM canvas_settings") && query.next())
	  {
	    QColor color(query.value(0).toString().trimmed());
	    QString name(query.value(1).toString().trimmed());
	    QString projectType(query.value(2).toString().trimmed());
	    QString updateMode(query.value(4).toString().trimmed());
	    int redoUndoStackSize = query.value(3).toInt();

	    if(!color.isValid())
	      color = QColor(211, 211, 211);

	    m_ui.background_color->setStyleSheet
	      (QString("QPushButton {background-color: %1}").arg(color.name()));
	    m_ui.background_color->setText(color.name());
	    m_ui.project_type->setCurrentIndex
	      (m_ui.project_type->findText(projectType));

	    if(m_ui.project_type->currentIndex() < 0)
	      m_ui.project_type->setCurrentIndex(0);

	    if(name.isEmpty())
	      name = defaultName();

	    m_ui.name->setText(name);
	    m_ui.redo_undo_stack_size->setValue(redoUndoStackSize);
	    m_ui.update_mode->setCurrentIndex
	      (m_ui.update_mode->findText(updateMode, Qt::MatchFixedString));

	    if(m_ui.update_mode->currentIndex() < 0)
	      m_ui.update_mode->setCurrentIndex(1); // Full

	    setResult(QDialog::Accepted);
	    emit accepted(false);
	  }
      }

    db.close();
  }

  glitch_common::discardDatabase(connectionName);
  QApplication::restoreOverrideCursor();
}

void glitch_canvas_settings::setFileName(const QString &fileName)
{
  m_fileName = fileName;
}

void glitch_canvas_settings::setName(const QString &name)
{
  if(name.trimmed().isEmpty())
    m_ui.name->setText(defaultName());
  else
    m_ui.name->setText(QString(name).remove("(*)").replace(" ", "-").trimmed());
}

void glitch_canvas_settings::setRedoUndoStackSize(const int value)
{
  m_ui.redo_undo_stack_size->setValue(value);
}

void glitch_canvas_settings::setSettings
(const QHash<glitch_canvas_settings::Settings, QVariant> &hash)
{
  QColor color(hash.value(CANVAS_BACKGROUND_COLOR).toString());

  m_ui.background_color->setStyleSheet
    (QString("QPushButton {background-color: %1}").arg(color.name()));
  m_ui.background_color->setText(color.name());
  setName(hash.value(CANVAS_NAME).toString());
  setResult(QDialog::Accepted);
  setViewportUpdateMode
    (QGraphicsView::ViewportUpdateMode(hash.value(VIEW_UPDATE_MODE).toInt()));
  emit accepted(false);
}

void glitch_canvas_settings::setViewportUpdateMode
(const QGraphicsView::ViewportUpdateMode mode)
{
  switch(mode)
    {
    case QGraphicsView::BoundingRectViewportUpdate:
      {
	m_ui.update_mode->setCurrentIndex
	  (m_ui.update_mode->findText(tr("Bounding Rectangle")));
	break;
      }
    case QGraphicsView::FullViewportUpdate:
      {
	m_ui.update_mode->setCurrentIndex
	  (m_ui.update_mode->findText(tr("Full")));
	break;
      }
    case QGraphicsView::MinimalViewportUpdate:
      {
	m_ui.update_mode->setCurrentIndex
	  (m_ui.update_mode->findText(tr("Minimal")));
	break;
      }
    case QGraphicsView::SmartViewportUpdate:
      {
	m_ui.update_mode->setCurrentIndex
	  (m_ui.update_mode->findText(tr("Smart")));
	break;
      }
    default:
      m_ui.update_mode->setCurrentIndex
	(m_ui.update_mode->findText(tr("Full")));
    }

  if(m_ui.update_mode->currentIndex() < 0)
    m_ui.update_mode->setCurrentIndex(1); // Full.
}

void glitch_canvas_settings::slotSelectBackgroundColor(void)
{
  QColorDialog dialog(this);

  dialog.setCurrentColor(QColor(m_ui.background_color->text()));
  dialog.setWindowIcon(windowIcon());
  QApplication::processEvents();

  if(dialog.exec() == QDialog::Accepted)
    {
      QColor color(dialog.selectedColor());

      m_ui.background_color->setStyleSheet
	(QString("QPushButton {background-color: %1;}").
	 arg(color.name()));
      m_ui.background_color->setText(color.name());
    }
}
