/***************************************************************************
 *   Copyright (C) 2017 by Nicolas Carion                                  *
 *   This file is part of Kdenlive. See www.kdenlive.org.                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3 or any later version accepted by the       *
 *   membership of KDE e.V. (or its successor approved  by the membership  *
 *   of KDE e.V.), which shall act as a proxy defined in Section 14 of     *
 *   version 3 of the license.                                             *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef TRACKMODEL_H
#define TRACKMODEL_H

#include "undohelper.hpp"
#include <QReadWriteLock>
#include <QSharedPointer>
#include <memory>
#include <mlt++/MltPlaylist.h>
#include <mlt++/MltTractor.h>
#include <unordered_map>
#include <unordered_set>

class TimelineModel;
class ClipModel;
class CompositionModel;
class EffectStackModel;

/* @brief This class represents a Track object, as viewed by the backend.
   To allow same track transitions, a Track object corresponds to two Mlt::Playlist, between which we can switch when required by the transitions.
   In general, the Gui associated with it will send modification queries (such as resize or move), and this class authorize them or not depending on the
   validity of the modifications
*/
class TrackModel
{

public:
    TrackModel() = delete;
    ~TrackModel();

    friend class ClipModel;
    friend class CompositionModel;
    friend class TimelineController;
    friend struct TimelineFunctions;
    friend class TimelineItemModel;
    friend class TimelineModel;

private:
    /* This constructor is private, call the static construct instead */
    TrackModel(const std::weak_ptr<TimelineModel> &parent, int id = -1, const QString &trackName = QString(), bool audioTrack = false);
    TrackModel(const std::weak_ptr<TimelineModel> &parent, Mlt::Tractor mltTrack, int id = -1);

public:
    /* @brief Creates a track, which references itself to the parent
       Returns the (unique) id of the created track
       @param id Requested id of the track. Automatic if id = -1
       @param pos is the optional position of the track. If left to -1, it will be added at the end
     */
    static int construct(const std::weak_ptr<TimelineModel> &parent, int id = -1, int pos = -1, const QString &trackName = QString(), bool audioTrack = false);

    /* @brief returns the number of clips */
    int getClipsCount();

    /* @brief returns the number of compositions */
    int getCompositionsCount() const;

    /* Perform a split at the requested position */
    bool splitClip(QSharedPointer<ClipModel> caller, int position);

    /* Implicit conversion operator to access the underlying producer
     */
    operator Mlt::Producer &() { return *m_track.get(); }

    /* Returns true if track is in locked state
     */
    bool isLocked() const;
    /* Returns true if track is an audio track
     */
    bool isAudioTrack() const;

    // TODO make protected
    QVariant getProperty(const QString &name) const;
    void setProperty(const QString &name, const QString &value);
    std::unordered_set<int> getClipsAfterPosition(int position, int end = -1);
    std::unordered_set<int> getCompositionsAfterPosition(int position, int end);

protected:
    /* @brief Returns a lambda that performs a resize of the given clip.
       The lamda returns true if the operation succeeded, and otherwise nothing is modified
       This method is protected because it shouldn't be called directly. Call the function in the timeline instead.
       @param clipId is the id of the clip
       @param in is the new starting on the clip
       @param out is the new ending on the clip
       @param right is true if we change the right side of the clip, false otherwise
    */
    Fun requestClipResize_lambda(int clipId, int in, int out, bool right);

    /* @brief Performs an insertion of the given clip.
       Returns true if the operation succeeded, and otherwise, the track is not modified.
       This method is protected because it shouldn't be called directly. Call the function in the timeline instead.
       @param clip is the id of the clip
       @param position is the position where to insert the clip
       @param updateView whether we send update to the view
       @param finalMove if the move is finished (not while dragging), so we invalidate timeline preview / check project duration
       @param undo Lambda function containing the current undo stack. Will be updated with current operation
       @param redo Lambda function containing the current redo queue. Will be updated with current operation
    */
    bool requestClipInsertion(int clipId, int position, bool updateView, bool finalMove, Fun &undo, Fun &redo);
    /* @brief This function returns a lambda that performs the requested operation */
    Fun requestClipInsertion_lambda(int clipId, int position, bool updateView, bool finalMove);

    /* @brief Performs an deletion of the given clip.
       Returns true if the operation succeeded, and otherwise, the track is not modified.
       This method is protected because it shouldn't be called directly. Call the function in the timeline instead.
       @param clipId is the id of the clip
       @param updateView whether we send update to the view
       @param finalMove if the move is finished (not while dragging), so we invalidate timeline preview / check project duration
       @param undo Lambda function containing the current undo stack. Will be updated with current operation
       @param redo Lambda function containing the current redo queue. Will be updated with current operation
    */
    bool requestClipDeletion(int clipId, bool updateView, bool finalMove, Fun &undo, Fun &redo);
    /* @brief This function returns a lambda that performs the requested operation */
    Fun requestClipDeletion_lambda(int clipId, bool updateView, bool finalMove);

    /* @brief Performs an insertion of the given composition.
       Returns true if the operation succeeded, and otherwise, the track is not modified.
       This method is protected because it shouldn't be called directly. Call the function in the timeline instead.
       Note that in Mlt, the composition insertion logic is not really at the track level, but we use that level to do collision checking
       @param compoId is the id of the composition
       @param position is the position where to insert the composition
       @param updateView whether we send update to the view
       @param undo Lambda function containing the current undo stack. Will be updated with current operation
       @param redo Lambda function containing the current redo queue. Will be updated with current operation
    */
    bool requestCompositionInsertion(int compoId, int position, bool updateView, Fun &undo, Fun &redo);
    /* @brief This function returns a lambda that performs the requested operation */
    Fun requestCompositionInsertion_lambda(int compoId, int position, bool updateView);

    bool requestCompositionDeletion(int compoId, bool updateView, Fun &undo, Fun &redo);
    Fun requestCompositionDeletion_lambda(int compoId, bool updateView);
    Fun requestCompositionResize_lambda(int compoId, int in, int out = -1);

    /* @brief Returns the size of the blank before or after the given clip
       @param clipId is the id of the clip
       @param after is true if we query the blank after, false otherwise
    */
    int getBlankSizeNearClip(int clipId, bool after);
    int getBlankSizeNearComposition(int compoId, bool after);
    int getBlankStart(int position);
    int getBlankSizeAtPos(int frame);

    /*@brief Returns the (unique) construction id of the track*/
    int getId() const;

    /*@brief This function is used only by the QAbstractItemModel
      Given a row in the model, retrieves the corresponding clip id. If it does not exist, returns -1
    */
    int getClipByRow(int row) const;

    /*@brief This function is used only by the QAbstractItemModel
      Given a row in the model, retrieves the corresponding composition id. If it does not exist, returns -1
    */
    int getCompositionByRow(int row) const;
    /*@brief This function is used only by the QAbstractItemModel
      Given a clip ID, returns the row of the clip.
    */
    int getRowfromClip(int clipId) const;

    /*@brief This function is used only by the QAbstractItemModel
      Given a composition ID, returns the row of the composition.
    */
    int getRowfromComposition(int compoId) const;

    /*@brief This is an helper function that test frame level consistancy with the MLT structures */
    bool checkConsistency();

    /* @brief Returns true if we have a composition intersecting with the range [in,out]*/
    bool hasIntersectingComposition(int in, int out) const;

    /* @brief This is an helper function that returns the sub-playlist in which the clip is inserted, along with its index in the playlist
     @param position the position of the target clip*/
    std::pair<int, int> getClipIndexAt(int position);

    /* @brief This is an helper function that checks in all playlists if the given position is a blank */
    bool isBlankAt(int position);

    /* @brief This is an helper function that returns the end of the blank that covers given position */
    int getBlankEnd(int position);
    /* Same, but we restrict to a specific track*/
    int getBlankEnd(int position, int track);

    /* @brief Returns the clip id on this track at position requested, or -1 if no clip */
    int getClipByPosition(int position);

    /* @brief Returns the composition id on this track starting position requested, or -1 if not found */
    int getCompositionByPosition(int position);
    /* @brief Add a track effect */
    bool addEffect(const QString &effectId);
    void replugClip(int clipId);
    int trackDuration();

public slots:
    /*Delete the current track and all its associated clips */
    void slotDelete();

private:
    std::weak_ptr<TimelineModel> m_parent;
    int m_id; // this is the creation id of the track, used for book-keeping

    // We fake two playlists to allow same track transitions.
    std::shared_ptr<Mlt::Tractor> m_track;
    Mlt::Playlist m_playlists[2];

    int m_currentInsertionOrder;

    std::map<int, std::shared_ptr<ClipModel>> m_allClips; /*this is important to keep an
                                                                            ordered structure to store the clips, since we use their ids order as row order*/
    std::map<int, std::shared_ptr<CompositionModel>>
        m_allCompositions; /*this is important to keep an
                                   ordered structure to store the clips, since we use their ids order as row order*/

    std::map<int, int> m_compoPos; // We store the positions of the compositions. In Melt, the compositions are not inserted at the track level, but we keep
                                   // those positions here to check for moves and resize

    mutable QReadWriteLock m_lock; // This is a lock that ensures safety in case of concurrent access

protected:
    std::shared_ptr<EffectStackModel> m_effectStack;
};

#endif
