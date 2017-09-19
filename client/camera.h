#ifndef CAMERA_H
#define CAMERA_H
/*
    well,there are two policies,one is emit buffer when VideoSrc avilable, one is timer emit fetching from VideoSrc per xx msecond.

*/
#include <QTimer>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/ml/ml.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include <QObject>
#include <QMutex>
#include <QThread>
#include "config.h"
#include "videosrc.h"
#include "videohandler.h"
using namespace cv;
using namespace std;
//class CameraManager;
/*
    get mat from src  every XX msec , give mat to video handler
    if video src fail to create  or fail to fetch mat, signal will send to this camera ,
    frequency will turn down, and try restart video src per XXX sencond
*/

class Camera : public QObject
{
    Q_OBJECT
public:
    explicit Camera(camera_data_t dat,QObject *parent=0) : data(dat),QObject(parent)
    {
        tick=0;
        tick_work=0;

        //        p_video_src=new VideoSrc(data.ip);
        //       connect(p_video_src,SIGNAL(video_disconnected()),this,SLOT(source_disconnected()));
        //       connect(p_video_src,SIGNAL(video_disconnected()),this,SLOT(source_connected()));

        connected=false;
        p_video_src=create_video_src();
        //  connected=false;
        timer=new QTimer();
        connect(timer,SIGNAL(timeout()),this,SLOT(work()));
        //   fetch_thread.start();
        timer->start(1000);

    }
    ~Camera(){
        delete timer;
        delete p_video_src;
    }
    VideoSrc *create_video_src()
    {
        VideoSrc *p=new VideoSrc(data.ip);
        connect(p,SIGNAL(video_disconnected()),this,SLOT(source_disconnected()));
        connect(p,SIGNAL(video_connected()),this,SLOT(source_connected()));
        return p;
    }

    void restart_video_src()
    {
        if(p_video_src!=NULL)
        {
            disconnect(p_video_src,SIGNAL(video_disconnected()),this,SLOT(source_disconnected()));
            disconnect(p_video_src,SIGNAL(video_connected()),this,SLOT(source_connected()));
            delete p_video_src;


        }
        p_video_src=new VideoSrc(data.ip);
        connect(p_video_src,SIGNAL(video_disconnected()),this,SLOT(source_disconnected()));
        connect(p_video_src,SIGNAL(video_connected()),this,SLOT(source_connected()));
     //   source_connected();
        if(p_video_src->video_connected_flag==true)
            source_connected();
        else
            source_disconnected();
    }

    void restart(camera_data_t dat)
    {
        data=dat;
    }
    //    void fetch()
    //    {
    //        IplImage *f=p_video_src->fetch_frame();
    //    }
#ifdef CLIENT
    QWidget *get_render()
    {
        return video_handler.get_render();
    }
#endif
    void created_done()
    {
        prt(info,"video connected");
        connected=true;
        timer->start(30);
        //  timer->setInterval(30);
        //  connected=false;
    }
signals:

public slots:
    void source_connected()
    {
        prt(info,"video connected");
        connected=true;
        timer->start(30);
        //  timer->setInterval(30);
        //  connected=false;
    }
    void source_disconnected()
    {
        prt(info,"video disconnected");
        connected=false;
        timer->setInterval(1000);

        //   timer->stop();
        restart_video_src();

        // timer->setInterval(1000);
    }
    /*
    video src -> fetch mat
    video handler -> handle mat
    timer (always alive) work per xx mseconds
    */
    void work()
    {
        if(connected==true){
            tick++;
            if(tick==200){
                // prt(info,"restart video");
                //   std::this_thread::sleep_for(chrono::milliseconds(1000));
                //            delete p_video_src;
                //            //       std::this_thread::sleep_for(chrono::milliseconds(1000));
                //            //     p_video_src=new VideoSrc(data.ip);
                //            p_video_src=create_video_src();
                restart_video_src();
                tick=0;
            }
            //    IplImage *f=p_video_src->fetch_frame();
            Mat *f=p_video_src->fetch_frame_mat();
            //   if(f!=NULL&&tick_work++%1==0){
            if(1){
                //     if(tick+5%200>10){

                if(f==NULL){
                    prt(info,"get null frame");
                    mt.resize(0);
                    video_handler.set_frame(&mt);
                }else
                {video_handler.set_frame(f);

                    video_handler.work("test url");
                }
                //   }
            }else{
                //      prt(info,"sleep start");
                //     std::this_thread::sleep_for(chrono::milliseconds(2000));
                //    prt(info,"sleep end");

            }
        }else{
            prt(info,"%s , work ignored cuz unconnected",data.ip.toStdString().data());
            restart_video_src();
        }
    }
private:
    camera_data_t data;//data that camera need
    QTimer *timer;//do work per xx micro seconds
    VideoSrc*p_video_src;//camera frame source
    VideoHandler video_handler;//camera frame handler
    int tick;
    int tick_work;
    bool connected=false;
    QList <IplImage> frame_list;
    QMutex lock;
    Mat mt;
    //   QThread fetch_thread;
};


class CameraManager:public QObject{
    Q_OBJECT
public:
    //    CameraManager(){
    //        p_cfg=new Config("/root/repo-github/pedestrian-v1/server/config.json");
    //        //     p_cfg=new Config();
    //        for(int i=0;i<p_cfg->data.camera_amount;i++){
    //            Camera *c=new Camera(p_cfg->data.camera[i]);
    //            cams.append(c);
    //        }
    //    }
    CameraManager(char * url){
        p_cfg=new Config(url);
        reload_camera();
    }
    ~CameraManager(){
        for(int i=0;i<p_cfg->data.camera_amount;i++){
            delete cams[i];
        }
    }

    void reload_camera()
    {
        foreach (Camera *c, cams) {
            delete c;
        }
        int num;
        cams.clear();
        for(int i=0;i<p_cfg->data.camera_amount;i++){
            Camera *c=new Camera(p_cfg->data.camera[i]);
            cams.append(c);
            //  if(i==0)
            //    connect(c->p_src,SIGNAL(frame_update(Mat)),&c->render,SLOT(set_mat(Mat)));
            //   if(i==0)
            //   layout->addWidget(&c->render,i,i);
        }
        num=cams.size();
    }
    QList <Camera *>  &get_cam()
    {
        return cams;
    }
    void save_config(QByteArray buf)
    {
        p_cfg->set_ba((buf));
    }

public slots:
    void add_camera(QByteArray buf)
    {
        p_cfg->set_ba((buf));
        Camera *c=new Camera(p_cfg->data.camera[p_cfg->data.camera_amount-1]);
        cams.append(c);
    }
    void add_camera(QString ip)
    {
        //         Camera *c=new Camera(cfg.data.camera[i]);

        camera_data_t ca;
        ca.ip=ip;
        ca.port=554;
        p_cfg->data.camera.append(ca);
        p_cfg->data.camera_amount++;
        Camera *c=new Camera(p_cfg->data.camera[p_cfg->data.camera_amount-1]);
        cams.append(c);
        p_cfg->save();
        //  if(i==0)
        //    connect(c->p_src,SIGNAL(frame_update(Mat)),&c->render,SLOT(set_mat(Mat)));
        //   if(i==0)
        //    layout->addWidget(&c->render,1,cams.length()-1);
    }
    void del_camera(int index)
    {
        if(index<=p_cfg->data.camera_amount&&index>0){
            p_cfg->data.camera.removeAt(index-1);
            p_cfg->data.camera_amount--;
            p_cfg->save_config_to_file();
            delete cams[index-1];
            cams.removeAt(index-1);
        }
    }


    void modify_camera(int index)
    {
        cams[index]->restart(p_cfg->data.camera[index-1]);
    }
    int get_config(char *c)
    {
        QByteArray b(p_cfg->get_ba());
        int len=b.length();
        memcpy(c,b.data(),len);
        return len;
    }
    QString get_config()
    {
        QByteArray b(p_cfg->get_ba());
        QString str(b);
        return str;
    }
    QByteArray get_config(int i)
    {
        QByteArray b(p_cfg->get_ba());

        return b;
    }

    int get_size()
    {
        return cams.size();
    }

private:
    QList <Camera *> cams;//cameras that opened, all cameras is working,or trying to work
    Config *p_cfg;//all the setting on this server
};


#endif // CAMERA_H
