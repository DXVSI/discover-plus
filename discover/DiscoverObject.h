/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QUrl>

#include "Category/Category.h"
#include <QQuickView>

class AbstractResource;
class Category;
class KStatusNotifierItem;
class QWindow;
class QQmlApplicationEngine;
class CachedNetworkAccessManagerFactory;
class TransactionsJob;
class InlineMessage;

#define DISCOVER_BASE_URL "qrc:/qt/qml/org/kde/discover/qml"

class DiscoverObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isRoot READ isRoot CONSTANT)
    Q_PROPERTY(QQuickWindow *mainWindow READ mainWindow CONSTANT)
    Q_PROPERTY(InlineMessage *homePageMessage READ homePageMessage NOTIFY homeMessageChanged)
    Q_PROPERTY(int sidebarWidth READ sidebarWidth WRITE setSidebarWidth NOTIFY sidebarWidthChanged)

public:
    explicit DiscoverObject(const QVariantMap &initialProperties);
    ~DiscoverObject() override;

    QStringList modes() const;

    bool eventFilter(QObject *object, QEvent *event) override;

    Q_SCRIPTABLE static QString iconName(const QIcon &icon);

    void loadTest(const QUrl &url);

    static bool isRoot();
    QQuickWindow *mainWindow() const;
    void showError(const QString &msg);
    Q_INVOKABLE void copyTextToClipboard(const QString &text);
    Q_INVOKABLE QUrl searchUrl(const QString &searchText);
    Q_INVOKABLE QString mimeTypeComment(const QString &mimeTypeName);

    QString describeSources() const;
    Q_SCRIPTABLE void restore();
    [[nodiscard]] InlineMessage *homePageMessage() const;

    int sidebarWidth() const;
    void setSidebarWidth(int width);

    Q_SCRIPTABLE bool quitWhenIdle();

public Q_SLOTS:
    void openApplication(const QUrl &app);
    void openMimeType(const QString &mime);
    void openCategory(const QString &category);
    void openMode(const QString &mode);
    void openLocalPackage(const QUrl &localfile);
    void startHeadlessUpdate();

    void promptReboot();
    void rebootNow();
    void shutdownNow();

private Q_SLOTS:
    void switchApplicationLanguage();

Q_SIGNALS:
    void openSearch(const QString &search);
    void openApplicationInternal(AbstractResource *app);
    void listMimeInternal(const QString &mime);
    void listCategoryInternal(const std::shared_ptr<Category> &cat);

    void unableToFind(const QString &resid);
    void
    openErrorPage(const QString &errorMessage, const QString &errorExplanation, const QString &buttonText, const QString &buttonIcon, const QString &buttonURL);
    void homeMessageChanged();
    void sidebarWidthChanged(int width);

private:
    bool isBusy() const;
    void showLoadingPage();
    void initMainWindow(QQuickWindow *mainWindow);
    void reconsiderQuit();
    QQmlApplicationEngine *engine() const
    {
        return m_engine.get();
    }

    std::unique_ptr<QQmlApplicationEngine, QScopedPointerDeleteLater> m_engine;
    std::unique_ptr<QQuickWindow> m_mainWindow;
    std::unique_ptr<CachedNetworkAccessManagerFactory> m_networkAccessManagerFactory;
    std::unique_ptr<KStatusNotifierItem, QScopedPointerDeleteLater> m_sni;
    std::unique_ptr<InlineMessage> m_homePageMessage;

    bool m_isDeleting = false;
};
