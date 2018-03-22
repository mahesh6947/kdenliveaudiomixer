/***************************************************************************
 *   Copyright (C) 2011 by Till Theato (root@ttill.de)                     *
 *   This file is part of Kdenlive (www.kdenlive.org).                     *
 *                                                                         *
 *   Kdenlive is free software: you can redistribute it and/or modify      *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Kdenlive is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with Kdenlive.  If not, see <http://www.gnu.org/licenses/>.     *
 ***************************************************************************/

#ifndef DRAGVALUE_H
#define DRAGVALUE_H

#include <QDoubleSpinBox>
#include <QProgressBar>
#include <QSpinBox>
#include <QWidget>
#include <kselectaction.h>

class QAction;
class QMenu;
class KSelectAction;

class CustomLabel : public QProgressBar
{
    Q_OBJECT
public:
    explicit CustomLabel(const QString &label, bool showSlider = true, int range = 1000, QWidget *parent = nullptr);
    void setProgressValue(double value);
    void setStep(double step);

protected:
    // virtual void mouseDoubleClickEvent(QMouseEvent * event);
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    // virtual void paintEvent(QPaintEvent *event);
    void wheelEvent(QWheelEvent *event) override;
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;

private:
    QPoint m_dragStartPosition;
    QPoint m_dragLastPosition;
    bool m_dragMode;
    bool m_showSlider;
    double m_step;
    void slotValueInc(double factor = 1);
    void slotValueDec(double factor = 1);
    void setNewValue(double, bool);

signals:
    void valueChanged(double, bool);
    void setInTimeline();
    void resetValue();
};

/**
 * @brief A widget for modifying numbers by dragging, using the mouse wheel or entering them with the keyboard.
 */

class DragValue : public QWidget
{
    Q_OBJECT

public:
    /**
    * @brief Default constructor.
    * @param label The label that will be displayed in the progress bar
    * @param defaultValue The default value
    * @param decimals The number of decimals for the parameter. 0 means it is an integer
    * @param min The minimum value
    * @param max The maximum value
    * @param id Used to identify this widget. If this parameter is set, "Show in Timeline" will be available in context menu.
    * @param suffix The suffix that will be displayed in the spinbox (for example '%')
    * @param showSlider If disabled, user can still drag on the label but no progress bar is shown
    */
    explicit DragValue(const QString &label, double defaultValue, int decimals, double min = 0, double max = 100, int id = -1,
                       const QString &suffix = QString(), bool showSlider = true, QWidget *parent = nullptr);
    virtual ~DragValue();

    /** @brief Returns the precision = number of decimals */
    int precision() const;
    /** @brief Returns the maximum value */
    qreal minimum() const;
    /** @brief Returns the minimum value */
    qreal maximum() const;

    /** @brief Sets the minimum value. */
    void setMinimum(qreal min);
    /** @brief Sets the maximum value. */
    void setMaximum(qreal max);
    /** @brief Sets minimum and maximum value. */
    void setRange(qreal min, qreal max);
    /** @brief Sets the size of a step (when dragging or using the mouse wheel). */
    void setStep(qreal step);

    /** @brief Returns the current value */
    qreal value() const;
    /** @brief Change the "inTimeline" property to paint the intimeline widget differently. */
    void setInTimelineProperty(bool intimeline);
    /** @brief Returns minimum size for QSpinBox, used to set all spinboxes to the same width. */
    int spinSize();
    /** @brief Sets the minimum size for QSpinBox, used to set all spinboxes to the same width. */
    void setSpinSize(int width);
    /** @brief Returns true if widget is currently being edited */
    bool hasEditFocus() const;

public slots:
    /** @brief Sets the value (forced to be in the valid range) and emits valueChanged. */
    void setValue(double value, bool final = true);
    void setValueFromProgress(double value, bool final);
    /** @brief Resets to default value */
    void slotReset();

signals:
    void valueChanged(double value, bool final = true);
    void inTimeline(int);

    /*
     * Private
     */

protected:
    /*virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);*/
    /** @brief Forwards tab focus to lineedit since it is disabled. */
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    // virtual void keyPressEvent(QKeyEvent *e);
    // virtual void wheelEvent(QWheelEvent *e);
    // virtual void paintEvent( QPaintEvent * event );

private slots:

    void slotEditingFinished();

    void slotSetScaleMode(int mode);
    void slotSetDirectUpdate(bool directUpdate);
    void slotShowContextMenu(const QPoint &pos);
    void slotSetValue(int value);
    void slotSetValue(double value);
    void slotSetInTimeline();

private:
    double m_maximum;
    double m_minimum;
    int m_decimals;
    double m_default;
    int m_id;
    QSpinBox *m_intEdit;
    QDoubleSpinBox *m_doubleEdit;

    QMenu *m_menu;
    KSelectAction *m_scale;
    QAction *m_directUpdate;
    CustomLabel *m_label;
};

#endif
