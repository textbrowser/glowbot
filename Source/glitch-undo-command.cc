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

#include "Arduino/glitch-object-function-arduino.h"
#include "glitch-object.h"
#include "glitch-proxy-widget.h"
#include "glitch-scene.h"
#include "glitch-undo-command.h"
#include "glitch-user-functions.h"

glitch_undo_command::glitch_undo_command
(const QHash<glitch_canvas_settings::Settings,
             QVariant> &previousCanvasSettings,
 const Types type,
 glitch_canvas_settings *canvasSettings,
 QUndoCommand *parent):QUndoCommand(parent)
{
  if(canvasSettings)
    m_currentCanvasSettings = canvasSettings->settings();

  m_canvasSettings = canvasSettings;
  m_previousCanvasSettings = previousCanvasSettings;
  m_property = glitch_object::XYZ_PROPERTY;
  m_type = type;
}

glitch_undo_command::glitch_undo_command
(const QPointF &previousPosition,
 const Types type,
 glitch_proxy_widget *proxy,
 glitch_scene *scene,
 QUndoCommand *parent):QUndoCommand(parent)
{
  if(proxy)
    m_currentPosition = proxy->scenePos();

  m_previousPosition = previousPosition;
  m_property = glitch_object::XYZ_PROPERTY;
  m_proxy = proxy;
  m_scene = scene;
  m_type = type;
}

glitch_undo_command::glitch_undo_command
(const QString &previousFunctionValue,
 const Types type,
 glitch_object *object,
 glitch_user_functions *userFunctions,
 QUndoCommand *parent):QUndoCommand(parent)
{
  m_object = object;

  switch(type)
    {
    case FUNCTION_RENAMED:
      {
	if(m_object)
	  m_currentFunctionName = object->name();

	m_previousFunctionName = previousFunctionValue;
	break;
      }
    case FUNCTION_RETURN_TYPE_CHANGED:
      {
	if(qobject_cast<glitch_object_function_arduino *> (m_object))
	  m_currentFunctionReturnType =
	    qobject_cast<glitch_object_function_arduino *> (m_object)->
	    returnType();

	m_previousFunctionReturnType = previousFunctionValue;
	break;
      }
    default:
      {
	break;
      }
    }

  m_property = glitch_object::XYZ_PROPERTY;
  m_type = type;
  m_userFunctions = userFunctions;
}

glitch_undo_command::glitch_undo_command
(const QString &previousStyleSheet,
 const Types type,
 glitch_object *object,
 QUndoCommand *parent):QUndoCommand(parent)
{
  if(object)
    m_currentStyleSheet = object->styleSheet();

  m_object = object;
  m_previousStyleSheet = previousStyleSheet;
  m_property = glitch_object::XYZ_PROPERTY;
  m_type = type;
}

glitch_undo_command::glitch_undo_command
(const QVariant &currentProperty,
 const QVariant &previousProperty,
 const Types type,
 const glitch_object::Properties property,
 glitch_object *object,
 QUndoCommand *parent):QUndoCommand(parent)
{
  m_currentProperty = currentProperty;
  m_object = object;
  m_previousProperty = previousProperty;
  m_property = property;
  m_type = type;
}

glitch_undo_command::glitch_undo_command
(const Types type,
 glitch_proxy_widget *proxy,
 glitch_scene *scene,
 QUndoCommand *parent):QUndoCommand(parent)
{
  m_property = glitch_object::XYZ_PROPERTY;
  m_proxy = proxy;
  m_scene = scene;
  m_type = type;
}

glitch_undo_command::~glitch_undo_command()
{
}

void glitch_undo_command::redo(void)
{
  switch(m_type)
    {
    case CANVAS_SETTINGS_CHANGED:
      {
	if(m_canvasSettings)
	  m_canvasSettings->setSettings(m_currentCanvasSettings);

	break;
      }
    case FUNCTION_RENAMED:
      {
	if(m_object && m_userFunctions)
	  {
	    m_object->setName(m_currentFunctionName);
	    m_userFunctions->addFunction(m_currentFunctionName);
	    m_userFunctions->deleteFunction(m_previousFunctionName);
	  }

	break;
      }
    case FUNCTION_RETURN_TYPE_CHANGED:
      {
	if(qobject_cast<glitch_object_function_arduino *> (m_object))
	  qobject_cast<glitch_object_function_arduino *> (m_object)->
	    setReturnType(m_currentFunctionReturnType);

	break;
      }
    case ITEM_ADDED:
      {
	if(m_proxy && m_scene)
	  {
	    m_proxy->setParent(m_scene);
	    m_scene->addItem(m_proxy);
	    m_scene->update();
	  }

	break;
      }
    case ITEM_DELETED:
      {
	if(m_proxy && m_scene)
	  {
	    auto object = qobject_cast<glitch_object *> (m_proxy->widget());

	    if(object)
	      {
		object->closeEditWindow();
		object->simulateDelete();
	      }

	    m_scene->removeItem(m_proxy);
	    m_scene->update();
	  }

	break;
      }
    case ITEM_MOVED:
      {
	if(m_proxy && m_scene)
	  {
	    m_proxy->setPos(m_currentPosition);
	    m_scene->update();
	  }

	break;
      }
    case PROPERTY_CHANGED:
      {
	if(m_object)
	  m_object->setProperty(m_property, m_currentProperty);

	break;
      }
    case STYLESHEET_CHANGED:
      {
	if(m_object)
	  m_object->setStyleSheet(m_currentStyleSheet);

	break;
      }
    default:
      {
	break;
      }
    }
}

void glitch_undo_command::undo(void)
{
  switch(m_type)
    {
    case CANVAS_SETTINGS_CHANGED:
      {
	if(m_canvasSettings)
	  m_canvasSettings->setSettings(m_previousCanvasSettings);

	break;
      }
    case FUNCTION_RENAMED:
      {
	if(m_object && m_userFunctions)
	  {
	    m_object->setName(m_previousFunctionName);
	    m_userFunctions->addFunction(m_previousFunctionName);
	    m_userFunctions->deleteFunction(m_currentFunctionName);
	  }

	break;
      }
    case FUNCTION_RETURN_TYPE_CHANGED:
      {
	if(qobject_cast<glitch_object_function_arduino *> (m_object))
	  qobject_cast<glitch_object_function_arduino *> (m_object)->
	    setReturnType(m_previousFunctionReturnType);

	break;
      }
    case ITEM_ADDED:
      {
	if(m_proxy && m_scene)
	  {
	    m_scene->removeItem(m_proxy);
	    m_scene->update();
	  }

	break;
      }
    case ITEM_DELETED:
      {
	if(m_proxy && m_scene)
	  {
	    m_scene->addItem(m_proxy);
	    m_scene->update();
	  }

	break;
      }
    case ITEM_MOVED:
      {
	if(m_proxy && m_scene)
	  {
	    m_proxy->setPos(m_previousPosition);
	    m_scene->update();
	  }

	break;
      }
    case PROPERTY_CHANGED:
      {
	if(m_object)
	  m_object->setProperty(m_property, m_previousProperty);

	break;
      }
    case STYLESHEET_CHANGED:
      {
	if(m_object)
	  m_object->setStyleSheet(m_previousStyleSheet);

	break;
      }
    default:
      {
	break;
      }
    }
}
