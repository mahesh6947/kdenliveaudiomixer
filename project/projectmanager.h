/*
Copyright (C) 2014  Till Theato <root@ttill.de>
This file is part of kdenlive. See www.kdenlive.org.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include "kdenlivecore_export.h"
#include <KRecentFilesAction>
#include <QDir>
#include <QObject>
#include <QTime>
#include <QTimer>
#include <QUrl>

#include "timeline2/model/timelineitemmodel.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>

class KAutoSaveFile;
class KJob;
class KdenliveDoc;
class MarkerListModel;
class NotesPlugin;
class Project;
class QAction;
class QProgressDialog;
class QUrl;
class DocUndoStack;

/**
 * @class ProjectManager
 * @brief Takes care of interaction with projects.
 */

class /*KDENLIVECORE_EXPORT*/ ProjectManager : public QObject
{
    Q_OBJECT

public:
    /** @brief Sets up actions to interact for project interaction (undo, redo, open, save, ...) and creates an empty project. */
    explicit ProjectManager(QObject *parent = nullptr);
    virtual ~ProjectManager();

    /** @brief Returns a pointer to the currently opened project. A project should always be open. */
    KdenliveDoc *current();

    /** @brief Store command line args for later opening. */
    void init(const QUrl &projectUrl, const QString &clipList);

    void doOpenFile(const QUrl &url, KAutoSaveFile *stale);
    KRecentFilesAction *recentFilesAction();
    void prepareSave();
    /** @brief Disable all bin effects in current project */
    void disableBinEffects(bool disable);
    /** @brief Returns true if there is a selected item in timeline */
    bool hasSelection() const;
    /** @brief Returns current project's xml scene */
    QString projectSceneList(const QString &outputFolder);
    /** @brief returns a default hd profile depending on timezone*/
    static QString getDefaultProjectFormat();
    void saveZone(const QStringList &info, const QDir &dir);
    /** @brief Move project data files to new url */
    void moveProjectData(const QString &src, const QString &dest);
    /** @brief Retrieve current project's notes */
    QString documentNotes() const;

    /** @brief Retrieve the current Guide Model
        The method is virtual to allow mocking
     */
    virtual std::shared_ptr<MarkerListModel> getGuideModel();

    /** @brief Return the current undo stack
        The method is virtual to allow mocking
    */
    virtual std::shared_ptr<DocUndoStack> undoStack();

public slots:
    void newFile(bool showProjectSettings = true, bool force = false);
    /** @brief Shows file open dialog. */
    void openFile();
    void openLastFile();
    /** @brief Load files / clips passed on the command line. */
    void slotLoadOnOpen();

    /** @brief Checks whether a URL is available to save to.
    * @return Whether the file was saved. */
    bool saveFile();

    /** @brief Shows a save file dialog for saving the project.
    * @return Whether the file was saved. */
    bool saveFileAs();
    /** @brief Saves current timeline selection to an MLT playlist. */
    void slotSaveSelection(const QString &path = QString());

    /** @brief Set properties to match outputFileName and save the document.
     * Creates an autosave version of the output file too, at
     * ~/.kde/data/stalefiles/kdenlive/ \n
     * that will be actually written in KdenliveDoc::slotAutoSave()
    * @param outputFileName The URL to save to / The document's URL.
    * @return Whether we had success. */
    bool saveFileAs(const QString &outputFileName);
    /** @brief Close currently opened document. Returns false if something went wrong (cannot save modifications, ...). */
    bool closeCurrentDocument(bool saveChanges = true, bool quit = false);

    /** @brief Prepares opening @param url.
    *
    * Checks if already open and whether backup exists */
    void openFile(const QUrl &url);

    /** @brief Start autosave timer */
    void slotStartAutoSave();

    /** @brief Update project and monitors profiles */
    void slotResetProfiles();

    /** @brief Expand current timeline clip (recover clips and tracks from an MLT playlist) */
    void slotExpandClip();

    /** @brief Dis/enable all timeline effects */
    void slotDisableTimelineEffects(bool disable);

    /** @brief Un/Lock current timeline track */
    void slotSwitchTrackLock();
    void slotSwitchAllTrackLock();
    /** @brief Un/Set current track as target */
    void slotSwitchTrackTarget();

    /** @brief Set the text for current project's notes */
    void setDocumentNotes(const QString &notes);

    /** @brief Project's duration changed, adjust monitor, etc. */
    void adjustProjectDuration();
    /** @brief Add an asset in timeline (effect, transition). */
    void activateAsset(const QVariantMap effectData);

private slots:
    void slotRevert();
    /** @brief Open the project's backupdialog. */
    void slotOpenBackup(const QUrl &url = QUrl());
    /** @brief Start autosaving the document. */
    void slotAutoSave();
    /** @brief Report progress of folder move operation. */
    void slotMoveProgress(KJob *, unsigned long progress);
    void slotMoveFinished(KJob *job);

signals:
    void docOpened(KdenliveDoc *document);
    //     void projectOpened(Project *project);

protected:
    void updateTimeline(int pos = -1);

private:
    /** @brief Checks that the Kdenlive MIME type is correctly installed.
    * @param open If set to true, this will return the MIME type allowed for file opening (adds .tar.gz format)
    * @return The MIME type */
    QString getMimeType(bool open = true);
    /** @brief checks if autoback files exists, recovers from it if user says yes, returns true if files were recovered. */
    bool checkForBackupFile(const QUrl &url);

    KdenliveDoc *m_project;
    std::shared_ptr<TimelineItemModel> m_mainTimelineModel;
    QTime m_lastSave;
    QTimer m_autoSaveTimer;
    QUrl m_startUrl;
    QString m_loadClipsOnOpen;
    QMap<QString, QString> m_replacementPattern;

    QAction *m_fileRevert;
    KRecentFilesAction *m_recentFilesAction;
    NotesPlugin *m_notesPlugin;
    QProgressDialog *m_progressDialog;
    void saveRecentFiles();
};

#endif
