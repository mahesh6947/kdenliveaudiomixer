#ifndef AUDIOMIXER_H
#define AUDIOMIXER_H

#include"/home/mahesh/devel/src/kdenlive_git/kdenlive/src/timeline2/model/timelineitemmodel.hpp"
#include "/home/mahesh/devel/src/kdenlive_git/kdenlive/src/timeline2/model/timelinemodel.hpp"
#include "/home/mahesh/devel/src/kdenlive_git/kdenlive/src/timeline2/model/trackmodel.hpp"


#include <QWidget>

class audioMixer
{

public:
    audioMixer(QWidget *parent = 0);
    ~audioMixer();
};


class AudioMixer : public QWidget
{
   public:
   void setModel(std::shared_ptr<TimelineItemModel> model)
   {
       int pos;
       int ix;
       std::shared_ptr<TrackModel> ob1;


        int tracksCount = model->getTracksCount();
        int position = model->getTrackIndexFromPosition(pos);

        ob1->getProperty(QStringLiteral("kdenlive:track_name"));
    }




public:
    AudioMixer(QWidget *parent = 0);
    ~AudioMixer();
};




#endif // AUDIOMIXER_H
