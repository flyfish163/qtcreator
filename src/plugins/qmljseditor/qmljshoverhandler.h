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

#ifndef QMLJSHOVERHANDLER_H
#define QMLJSHOVERHANDLER_H

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <texteditor/basehoverhandler.h>

#include <QColor>

QT_BEGIN_NAMESPACE
template <class> class QList;
QT_END_NAMESPACE

namespace Core { class IEditor; }

namespace TextEditor { class BaseTextEditor; }

namespace QmlJS {
class ScopeChain;
class Context;
typedef QSharedPointer<const Context> ContextPtr;
class Value;
class ObjectValue;
}

namespace QmlJSEditor {
namespace Internal {

class QmlJSEditorWidget;

class QmlJSHoverHandler : public TextEditor::BaseHoverHandler
{
    Q_OBJECT
public:
    QmlJSHoverHandler(QObject *parent = 0);

private:
    void reset();

    bool acceptEditor(Core::IEditor *editor);
    void identifyMatch(TextEditor::TextEditorWidget *editorWidget, int pos);
    void operateTooltip(TextEditor::TextEditorWidget *editorWidget, const QPoint &point);

    bool matchDiagnosticMessage(QmlJSEditorWidget *qmlEditor, int pos);
    bool matchColorItem(const QmlJS::ScopeChain &lookupContext,
                        const QmlJS::Document::Ptr &qmlDocument,
                        const QList<QmlJS::AST::Node *> &astPath,
                        unsigned pos);
    void handleOrdinaryMatch(const QmlJS::ScopeChain &lookupContext,
                             QmlJS::AST::Node *node);
    void handleImport(const QmlJS::ScopeChain &lookupContext,
                      QmlJS::AST::UiImport *node);

    void prettyPrintTooltip(const QmlJS::Value *value,
                            const QmlJS::ContextPtr &context);

    bool setQmlTypeHelp(const QmlJS::ScopeChain &scopeChain, const QmlJS::Document::Ptr &qmlDocument,
                        const QmlJS::ObjectValue *value, const QStringList &qName);
    bool setQmlHelpItem(const QmlJS::ScopeChain &lookupContext,
                        const QmlJS::Document::Ptr &qmlDocument,
                        QmlJS::AST::Node *node);

    QmlJS::ModelManagerInterface *m_modelManager;
    QColor m_colorTip;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLJSHOVERHANDLER_H
