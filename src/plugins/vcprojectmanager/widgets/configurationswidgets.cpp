/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
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
#include "configurationswidgets.h"

#include <QVBoxLayout>

#include "configurationswidget.h"
#include "../vcprojectmodel/configurations.h"
#include "../vcprojectmodel/vcprojectdocument.h"
#include "../vcprojectmodel/configurationsfactory.h"
#include "../vcprojectmodel/configuration.h"
#include "../vcprojectmodel/tools/toolattributes/tooldescription.h"
#include "../vcprojectmodel/tools/toolattributes/tooldescriptiondatamanager.h"
#include "configurationwidgets.h"
#include "../vcprojectmodel/files.h"
#include "../vcprojectmodel/file.h"
#include "../interfaces/iattributecontainer.h"
#include "../interfaces/itools.h"

namespace VcProjectManager {
namespace Internal {

ConfigurationsBaseWidget::ConfigurationsBaseWidget(Configurations *configs, VcProjectDocument *vcProjDoc)
    : m_configs(configs),
      m_vcProjDoc(vcProjDoc)
{
    m_configsWidget = new ConfigurationsWidget;

    if (m_configs) {
        for (int i = 0; i < m_configs->configurationCount(); ++i) {
            IConfiguration *config = m_configs->configuration(i);
            if (config)
                addConfiguration(config);
        }
    }

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(m_configsWidget);
    setLayout(layout);

    connect(m_configsWidget, SIGNAL(addNewConfigSignal(QString, QString)), this, SLOT(onAddNewConfig(QString, QString)));
    connect(m_configsWidget, SIGNAL(renameConfigSignal(QString,QString)), this, SLOT(onRenameConfig(QString, QString)));
    connect(m_configsWidget, SIGNAL(removeConfigSignal(QString)), this, SLOT(onRemoveConfig(QString)));
}

ConfigurationsBaseWidget::~ConfigurationsBaseWidget()
{
}

void ConfigurationsBaseWidget::saveData()
{
    // remove deleted configurations
    foreach (const QString &removeConfigName, m_removedConfigurations) {
        IConfiguration *foundConfig = m_configs->configuration(removeConfigName);
        if (foundConfig)
            m_configs->removeConfiguration(foundConfig->fullName());
    }

    // rename configurations that were renamed
    QMapIterator<IConfiguration*, QString> it(m_renamedConfigurations);

    while (it.hasNext()) {
        it.next();
        IConfiguration *config = it.key();
        config->setFullName(it.value());
    }

    // add new configurations
    foreach (IConfiguration *newConfig, m_newConfigurations)
        m_configs->addConfiguration(newConfig);

    QHashIterator<QSharedPointer<File>, IConfiguration*> fileConfigIt(m_newFilesConfigurations);

    while (fileConfigIt.hasNext()) {
        fileConfigIt.next();
        fileConfigIt.key()->addFileConfiguration(fileConfigIt.value());
    }

    // save data for every configuration
    QList<ConfigurationBaseWidget *> configWidgets = m_configsWidget->configWidgets();
    foreach (ConfigurationBaseWidget *configWidget, configWidgets) {
        if (configWidget)
            configWidget->saveData();
    }
}

void ConfigurationsBaseWidget::onAddNewConfig(QString newConfigName, QString copyFrom)
{
    Platforms::Ptr platforms = m_vcProjDoc->platforms();

    if (platforms && !newConfigName.isEmpty()) {
        if (copyFrom.isEmpty()) {
            QList<Platform::Ptr> platformList = platforms->platforms();
            foreach (const Platform::Ptr &platform, platformList) {
                IConfiguration *newConfig = createConfiguration(newConfigName + QLatin1Char('|') + platform->name());

                if (newConfig) {
                    newConfig->attributeContainer()->setAttribute(QLatin1String("OutputDirectory"), QLatin1String("$(SolutionDir)$(ConfigurationName)"));
                    newConfig->attributeContainer()->setAttribute(QLatin1String("IntermediateDirectory"), QLatin1String("$(ConfigurationName)"));
                    newConfig->attributeContainer()->setAttribute(QLatin1String("ConfigurationType"), QLatin1String("1"));
                    m_newConfigurations.append(newConfig);
                    addConfiguration(newConfig);
                }
            }
        } else {
            IConfiguration *config = m_configs->configuration(copyFrom);

            if (config) {
                QList<Platform::Ptr> platformList = platforms->platforms();

                foreach (const Platform::Ptr &platform, platformList) {
                    IConfiguration* newConfig = config->clone();

                    if (newConfig) {
                        newConfig->setFullName(newConfigName + QLatin1Char('|') + platform->name());
                        m_newConfigurations.append(newConfig);
                        addConfiguration(newConfig);
                    }

                    addConfigurationToFiles(copyFrom, newConfigName + QLatin1Char('|') + platform->name());
                }
            }
        }
    }
}

void ConfigurationsBaseWidget::onRenameConfig(QString newConfigName, QString oldConfigNameWithPlatform)
{
    Platforms::Ptr platforms = m_vcProjDoc->platforms();

    if (!platforms || newConfigName.isEmpty() || oldConfigNameWithPlatform.isEmpty())
        return;

    QStringList splits = oldConfigNameWithPlatform.split(QLatin1Char('|'));

    if (splits.isEmpty())
        return;

    QList<Platform::Ptr> platformList = platforms->platforms();
    foreach (const Platform::Ptr &platform, platformList) {
        QString targetConfigName = splits[0] + QLatin1Char('|') + platform->name();
        QString newName = newConfigName + QLatin1Char('|') + platform->name();
        IConfiguration *configInNew = configInNewConfigurations(targetConfigName);

        // if we are renaming newly added config
        if (configInNew) {
            configInNew->setFullName(newName);
        } else {
            // we are renaming a config that is already in the model
            bool targetAlreadyExists = false;
            QMapIterator<IConfiguration*, QString> it(m_renamedConfigurations);

            while (it.hasNext()) {
                it.next();

                if (it.value() == targetConfigName) {
                    IConfiguration* key = m_renamedConfigurations.key(targetConfigName);

                    if (key) {
                        m_renamedConfigurations.insert(key, newName);
                        targetAlreadyExists = true;
                        break;
                    }
                }
            }

            if (!targetAlreadyExists) {
                IConfiguration *config = m_configs->configuration(targetConfigName);
                if (config)
                    m_renamedConfigurations.insert(config, newName);
            }
        }

        m_configsWidget->renameConfiguration(newName, targetConfigName);
    }
}

void ConfigurationsBaseWidget::onRemoveConfig(QString configNameWithPlatform)
{
    Platforms::Ptr platforms = m_vcProjDoc->platforms();

    if (!platforms || configNameWithPlatform.isEmpty())
        return;

    QStringList splits = configNameWithPlatform.split(QLatin1Char('|'));

    if (splits.isEmpty())
        return;

    QList<Platform::Ptr> platformList = platforms->platforms();
    foreach (const Platform::Ptr &platform, platformList) {
        QString targetConfigName = splits[0] + QLatin1Char('|') + platform->name();
        IConfiguration *config = m_configs->configuration(targetConfigName);

        // if config exists in the document model, add it to remove list
        if (config) {
            removeConfiguration(config);
            m_removedConfigurations.append(config->fullName());
        } else {
            // else remove it from the list of newly added configurations
            foreach (IConfiguration *configPtr, m_newConfigurations) {
                if (configPtr && configPtr->fullName() == targetConfigName) {
                    removeConfiguration(configPtr);
                    m_newConfigurations.removeAll(configPtr);
                    break;
                }
            }
        }
    }
}

void ConfigurationsBaseWidget::addConfiguration(IConfiguration *config)
{
    if (config)
        m_configsWidget->addConfiguration(config->fullName(), config->createSettingsWidget());
}

void ConfigurationsBaseWidget::removeConfiguration(IConfiguration *config)
{
    if (config)
        m_configsWidget->removeConfiguration(config->fullName());
}

IConfiguration *ConfigurationsBaseWidget::createConfiguration(const QString &configNameWithPlatform) const
{
    IConfiguration *config = 0;

    if (m_vcProjDoc->documentVersion() == VcDocConstants::DV_MSVC_2003)
        config = new Configuration2003(QLatin1String("Configuration"));
    else if (m_vcProjDoc->documentVersion() == VcDocConstants::DV_MSVC_2005)
        config = new Configuration2005(QLatin1String("Configuration"));
    else if (m_vcProjDoc->documentVersion() == VcDocConstants::DV_MSVC_2008)
        config = new Configuration2008(QLatin1String("Configuration"));

    if (config) {
        config->setFullName(configNameWithPlatform);

        ToolDescriptionDataManager *tDDM = ToolDescriptionDataManager::instance();

        if (tDDM) {
            for (int i = 0; i < tDDM->toolDescriptionCount(); ++i) {
                ToolDescription *toolDesc = tDDM->toolDescription(i);

                if (toolDesc) {
                    ITool *configTool = toolDesc->createTool();

                    if (configTool)
                        config->tools()->addTool(configTool);
                }
            }
        }
    }

    return config;
}

IConfiguration *ConfigurationsBaseWidget::configInNewConfigurations(const QString &configNameWithPlatform) const
{
    foreach (IConfiguration *config, m_newConfigurations) {
        if (config && config->fullName() == configNameWithPlatform)
            return config;
    }

    return 0;
}

void ConfigurationsBaseWidget::addConfigurationToFiles(const QString &copyFromConfig, const QString &targetConfigName)
{
    Files::Ptr docFiles = m_vcProjDoc->files();
    if (docFiles) {
        QList<Filter::Ptr> filters = docFiles->filters();

        foreach (Filter::Ptr filter, filters)
            addConfigurationToFilesInFilter(filter, copyFromConfig, targetConfigName);

        QList<File::Ptr> files = docFiles->files();

        foreach (File::Ptr file, files)
            addConfigurationToFile(file, copyFromConfig, targetConfigName);

        Files2005::Ptr docFiles2005 = docFiles.dynamicCast<Files2005>();

        if (docFiles2005) {
            QList<Folder::Ptr> folders = docFiles2005->folders();

            foreach (Folder::Ptr folder, folders)
                addConfigurationToFilesInFolder(folder, copyFromConfig, targetConfigName);
        }
    }
}

void ConfigurationsBaseWidget::addConfigurationToFilesInFilter(QSharedPointer<Filter> filterPtr, const QString &copyFromConfig, const QString &targetConfigName)
{
    QList<Filter::Ptr> filters = filterPtr->filters();

    foreach (Filter::Ptr filter, filters)
        addConfigurationToFilesInFilter(filter, copyFromConfig, targetConfigName);

    QList<File::Ptr> files = filterPtr->files();

    foreach (File::Ptr file, files)
        addConfigurationToFile(file, copyFromConfig, targetConfigName);
}

void ConfigurationsBaseWidget::addConfigurationToFilesInFolder(QSharedPointer<Folder> folderPtr, const QString &copyFromConfig, const QString &targetConfigName)
{
    QList<Filter::Ptr> filters = folderPtr->filters();

    foreach (Filter::Ptr filter, filters)
        addConfigurationToFilesInFilter(filter, copyFromConfig, targetConfigName);

    QList<Folder::Ptr> folders = folderPtr->folders();

    foreach (Folder::Ptr folder, folders)
        addConfigurationToFilesInFolder(folder, copyFromConfig, targetConfigName);

    QList<File::Ptr> files = folderPtr->files();

    foreach (File::Ptr file, files)
        addConfigurationToFile(file, copyFromConfig, targetConfigName);
}

void ConfigurationsBaseWidget::addConfigurationToFile(QSharedPointer<File> filePtr, const QString &copyFromConfig, const QString &targetConfigName)
{
    IConfiguration *configPtr = filePtr->fileConfiguration(copyFromConfig);

    if (configPtr) {
        IConfiguration *newConfig = configPtr->clone();
        newConfig->setFullName(targetConfigName);
        m_newFilesConfigurations[filePtr] = newConfig;
    }
}

Configurations2003Widget::Configurations2003Widget(Configurations *configs, VcProjectDocument *vcProjDoc)
    : ConfigurationsBaseWidget(configs, vcProjDoc)
{
}

Configurations2003Widget::~Configurations2003Widget()
{
}


Configurations2005Widget::Configurations2005Widget(Configurations *configs, VcProjectDocument *vcProjDoc)
    : ConfigurationsBaseWidget(configs, vcProjDoc)
{
}

Configurations2005Widget::~Configurations2005Widget()
{
}


Configurations2008Widget::Configurations2008Widget(Configurations *configs, VcProjectDocument *vcProjDoc)
    : ConfigurationsBaseWidget(configs, vcProjDoc)
{
}

Configurations2008Widget::~Configurations2008Widget()
{
}

} // namespace Internal
} // namespace VcProjectManager
