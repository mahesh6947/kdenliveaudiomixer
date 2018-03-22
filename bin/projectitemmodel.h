/*
Copyright (C) 2012  Till Theato <root@ttill.de>
Copyright (C) 2014  Jean-Baptiste Mardelle <jb@kdenlive.org>
Copyright (C) 2017 by Nicolas Carion
This file is part of Kdenlive. See www.kdenlive.org.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of
the License or (at your option) version 3 or any later version
accepted by the membership of KDE e.V. (or its successor approved
by the membership of KDE e.V.), which shall act as a proxy
defined in Section 14 of version 3 of the license.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PROJECTITEMMODEL_H
#define PROJECTITEMMODEL_H

#include "abstractmodel/abstracttreemodel.hpp"
#include "definitions.h"
#include "undohelper.hpp"
#include <QDomElement>
#include <QReadWriteLock>
#include <QSize>
#include <QIcon>

class AbstractProjectItem;
class BinPlaylist;
class MarkerListModel;
class ProjectClip;
class ProjectFolder;

namespace Mlt {
class Producer;
class Properties;
class Tractor;
}

/**
 * @class ProjectItemModel
 * @brief Acts as an adaptor to be able to use BinModel with item views.
 */

class ProjectItemModel : public AbstractTreeModel
{
    Q_OBJECT

protected:
    explicit ProjectItemModel(QObject *parent);

public:
    static std::shared_ptr<ProjectItemModel> construct(QObject *parent = nullptr);
    ~ProjectItemModel();

    friend class ProjectClip;

    /** @brief Returns a clip from the hierarchy, given its id
     */
    std::shared_ptr<ProjectClip> getClipByBinID(const QString &binId);

    /** @brief Helper to check whether a clip with a given id exists
     */
    bool hasClip(const QString &binId);

    /** @brief Gets a folder by its id. If none is found, the root is returned
     */
    std::shared_ptr<ProjectFolder> getFolderByBinId(const QString &binId);

    /** @brief Gets any item by its id.
     */
    std::shared_ptr<AbstractProjectItem> getItemByBinId(const QString &binId);

    /** @brief This function change the global enabled state of the bin effects
     */
    void setBinEffectsEnabled(bool enabled);

    /** @brief Returns some info about the folder containing the given index */
    QStringList getEnclosingFolderInfo(const QModelIndex &index) const;

    /** @brief Deletes all element and start a fresh model */
    void clean();

    /** @brief Returns the id of all the clips (excluding folders) */
    std::vector<QString> getAllClipIds() const;

    /** @brief Convenience method to access root folder */
    std::shared_ptr<ProjectFolder> getRootFolder() const;

    /** @brief Create the subclips defined in the parent clip.
        @param id is the id of the parent clip
        @param data is a definition of the subclips (keys are subclips' names, value are "in:out")*/
    void loadSubClips(const QString &id, const QMap<QString, QString> &data);

    /* @brief Convenience method to retrieve a pointer to an element given its index */
    std::shared_ptr<AbstractProjectItem> getBinItemByIndex(const QModelIndex &index) const;

    /* @brief Load the folders given the property containing them */
    bool loadFolders(Mlt::Properties &folders);

    /* @brief Parse a bin playlist from the document tractor and reconstruct the tree */
    void loadBinPlaylist(Mlt::Tractor *documentTractor, Mlt::Tractor *modelTractor, std::unordered_map<QString, QString> &binIdCorresp);

    /** @brief Save document properties in MLT's bin playlist */
    void saveDocumentProperties(const QMap<QString, QString> &props, const QMap<QString, QString> &metadata, std::shared_ptr<MarkerListModel> guideModel);

    /** @brief Save a property to main bin */
    void saveProperty(const QString &name, const QString &value);

    /** @brief Returns item data depending on role requested */
    QVariant data(const QModelIndex &index, int role) const override;
    /** @brief Called when user edits an item */
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    /** @brief Allow selection and drag & drop */
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    /** @brief Returns column names in case we want to use columns in QTreeView */
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    /** @brief Mandatory reimplementation from QAbstractItemModel */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    /** @brief Returns the MIME type used for Drag actions */
    QStringList mimeTypes() const override;
    /** @brief Create data that will be used for Drag events */
    QMimeData *mimeData(const QModelIndexList &indices) const override;
    /** @brief Set size for thumbnails */
    void setIconSize(QSize s);
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    Qt::DropActions supportedDropActions() const override;

    /* @brief Request deletion of a bin clip from the project bin
       @param clip : pointer to the clip to delete
       @param undo,redo: lambdas that are updated to accumulate operation.
     */
    bool requestBinClipDeletion(std::shared_ptr<AbstractProjectItem> clip, Fun &undo, Fun &redo);

    /* @brief Request creation of a bin folder
       @param id Id of the requested bin. If this is empty or invalid (already used, for example), it will be used as a return parameter to give the automatic
       bin id used.
       @param name Name of the folder
       @param parentId Bin id of the parent folder
       @param undo,redo: lambdas that are updated to accumulate operation.
    */
    bool requestAddFolder(QString &id, const QString &name, const QString &parentId, Fun &undo, Fun &redo);

    /* @brief Request creation of a bin clip
       @param id Id of the requested bin. If this is empty, it will be used as a return parameter to give the automatic bin id used.
       @param description Xml description of the clip
       @param parentId Bin id of the parent folder
       @param undo,redo: lambdas that are updated to accumulate operation.
    */
    bool requestAddBinClip(QString &id, const QDomElement &description, const QString &parentId, Fun &undo, Fun &redo);
    bool requestAddBinClip(QString &id, const QDomElement &description, const QString &parentId, const QString &undoText = QString());

    /* @brief This is the addition function when we already have a producer for the clip*/
    bool requestAddBinClip(QString &id, std::shared_ptr<Mlt::Producer> producer, const QString &parentId, Fun &undo, Fun &redo);

    /* @brief Create a subClip
       @param id Id of the requested bin. If this is empty, it will be used as a return parameter to give the automatic bin id used.
       @param parentId Bin id of the parent clip
       @param in,out : zone that corresponds to the subclip
       @param undo,redo: lambdas that are updated to accumulate operation.
    */
    bool requestAddBinSubClip(QString &id, int in, int out, const QString &parentId, Fun &undo, Fun &redo);
    bool requestAddBinSubClip(QString &id, int in, int out, const QString &parentId);

    /* @brief Request that a folder's name is changed
       @param clip : pointer to the folder to rename
       @param name: new name of the folder
       @param undo,redo: lambdas that are updated to accumulate operation.
    */
    bool requestRenameFolder(std::shared_ptr<AbstractProjectItem> folder, const QString &name, Fun &undo, Fun &redo);
    /* Same functions but pushes the undo object directly */
    bool requestRenameFolder(std::shared_ptr<AbstractProjectItem> folder, const QString &name);

    /* @brief Request that the unused clips are deleted */
    bool requestCleanup();

    /* @brief Retrieves the next id available for attribution to a folder */
    int getFreeFolderId();

    /* @brief Retrieves the next id available for attribution to a clip */
    int getFreeClipId();

protected:
    /* @brief Register the existence of a new element
     */
    void registerItem(const std::shared_ptr<TreeItem> &item) override;
    /* @brief Deregister the existence of a new element*/
    void deregisterItem(int id, TreeItem *item) override;

    /* @brief Helper function to generate a lambda that rename a folder */
    Fun requestRenameFolder_lambda(std::shared_ptr<AbstractProjectItem> folder, const QString &newName);

    /* @brief Helper function to add a given item to the tree */
    bool addItem(std::shared_ptr<AbstractProjectItem> item, const QString &parentId, Fun &undo, Fun &redo);

public slots:
    /** @brief An item in the list was modified, notify */
    void onItemUpdated(std::shared_ptr<AbstractProjectItem> item, int role);
    void onItemUpdated(const QString &binId, int role);

    /** @brief Check whether a given id is currently used or not*/
    bool isIdFree(const QString &id) const;

private:
    /** @brief Return reference to column specific data */
    int mapToColumn(int column) const;

    mutable QReadWriteLock m_lock; // This is a lock that ensures safety in case of concurrent access

    std::unique_ptr<BinPlaylist> m_binPlaylist;

    int m_nextId;

    QIcon m_blankThumb;
signals:
    // thumbs of the given clip were modified, request update of the monitor if need be
    void refreshAudioThumbs(const QString &id);
    void refreshClip(const QString &id);
    void emitMessage(const QString &, int, MessageType);
    void updateTimelineProducers(const QString &id, const QMap<QString, QString> &passProperties);
    void refreshPanel(const QString &id);
    void requestAudioThumbs(const QString &id, long duration);
    // TODO
    void markersNeedUpdate(const QString &id, const QList<int> &);
    void itemDropped(const QStringList &, const QModelIndex &);
    void itemDropped(const QList<QUrl> &, const QModelIndex &);
    void effectDropped(const QStringList &, const QModelIndex &);
    void addClipCut(const QString &, int, int);
};

#endif
