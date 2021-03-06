/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QVector>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace TextEditor {
    class TextEditorWidget;
}

namespace Android {
namespace Internal {

class AndroidManifestEditorIconWidget;

class SplashIconContainerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SplashIconContainerWidget(QWidget *parent,
                                                      TextEditor::TextEditorWidget *textEditorWidget);
    void loadImages();
    bool hasImages();
    bool hasPortraitImages();
    bool hasLandscapeImages();
    bool isSticky();
    void setSticky(bool sticky);
signals:
    void splashScreensModified();
private:
    QVector<AndroidManifestEditorIconWidget *> m_imageButtons;
    QVector<AndroidManifestEditorIconWidget *> m_portraitImageButtons;
    QVector<AndroidManifestEditorIconWidget *> m_landscapeImageButtons;
    bool m_splashScreenSticky = false;
    QCheckBox *m_stickyCheck = nullptr;
};

} // namespace Internal
} // namespace Android
