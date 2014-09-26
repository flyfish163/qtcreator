/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "basehoverhandler.h"
#include "texteditor.h"

#include <coreplugin/icore.h>
#include <utils/tooltip/tooltip.h>
#include <utils/tooltip/tipcontents.h>

#include <QPoint>

using namespace Core;

namespace TextEditor {

BaseHoverHandler::BaseHoverHandler(QObject *parent) : QObject(parent), m_diagnosticTooltip(false)
{
    // Listen for editor opened events in order to connect to tooltip/helpid requests
    connect(EditorManager::instance(), &EditorManager::editorOpened,
            this, &BaseHoverHandler::editorOpened);
}

BaseHoverHandler::~BaseHoverHandler()
{}

void BaseHoverHandler::editorOpened(Core::IEditor *editor)
{
    if (acceptEditor(editor)) {
        BaseTextEditor *textEditor = qobject_cast<BaseTextEditor *>(editor);
        if (textEditor) {
            connect(textEditor, &BaseTextEditor::tooltipRequested,
                    this, &BaseHoverHandler::showToolTip);

            connect(textEditor, &BaseTextEditor::contextHelpIdRequested,
                    this, &BaseHoverHandler::updateContextHelpId);
        }
    }
}

void BaseHoverHandler::showToolTip(BaseTextEditor *editor, const QPoint &point, int pos)
{
    TextEditorWidget *editorWidget = editor->editorWidget();

    editor->setContextHelpId(QString());

    process(editor, pos);
    operateTooltip(editorWidget, point);
}

void BaseHoverHandler::updateContextHelpId(BaseTextEditor *editor, int pos)
{
    // If the tooltip is visible and there is a help match, this match is used to update
    // the help id. Otherwise, let the identification process happen.
    if (!Utils::ToolTip::isVisible() || !lastHelpItemIdentified().isValid())
        process(editor, pos);

    if (lastHelpItemIdentified().isValid())
        editor->setContextHelpId(lastHelpItemIdentified().helpId());
    else
        editor->setContextHelpId(QString()); // Make sure it's an empty string.
}

void BaseHoverHandler::setToolTip(const QString &tooltip)
{
    m_toolTip = tooltip;
}

const QString &BaseHoverHandler::toolTip() const
{
    return m_toolTip;
}

void BaseHoverHandler::appendToolTip(const QString &extension)
{
    m_toolTip.append(extension);
}

void BaseHoverHandler::addF1ToToolTip()
{
    m_toolTip = QString::fromLatin1("<table><tr><td valign=middle>%1</td><td>&nbsp;&nbsp;"
                                    "<img src=\":/texteditor/images/f1.png\"></td>"
                                    "</tr></table>").arg(m_toolTip);
}

void BaseHoverHandler::setIsDiagnosticTooltip(bool isDiagnosticTooltip)
{
    m_diagnosticTooltip = isDiagnosticTooltip;
}

bool BaseHoverHandler::isDiagnosticTooltip() const
{
    return m_diagnosticTooltip;
}

void BaseHoverHandler::setLastHelpItemIdentified(const HelpItem &help)
{
    m_lastHelpItemIdentified = help;
}

const HelpItem &BaseHoverHandler::lastHelpItemIdentified() const
{
    return m_lastHelpItemIdentified;
}

void BaseHoverHandler::clear()
{
    m_diagnosticTooltip = false;
    m_toolTip.clear();
    m_lastHelpItemIdentified = HelpItem();
}

void BaseHoverHandler::process(BaseTextEditor *editor, int pos)
{
    clear();
    identifyMatch(editor->editorWidget(), pos);
    decorateToolTip();
}

void BaseHoverHandler::decorateToolTip()
{
    if (Qt::mightBeRichText(toolTip()))
        setToolTip(toolTip().toHtmlEscaped());

    if (!isDiagnosticTooltip() && lastHelpItemIdentified().isValid()) {
        const QString &contents = lastHelpItemIdentified().extractContent(false);
        if (!contents.isEmpty()) {
            setToolTip(toolTip().toHtmlEscaped());
            appendToolTip(contents);
            addF1ToToolTip();
        }
    }
}

void BaseHoverHandler::operateTooltip(TextEditorWidget *editorWidget, const QPoint &point)
{
    if (m_toolTip.isEmpty())
        Utils::ToolTip::hide();
    else
        Utils::ToolTip::show(point, Utils::TextContent(m_toolTip), editorWidget);
}

} // namespace TextEditor
