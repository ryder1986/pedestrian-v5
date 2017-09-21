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
#include <QMutex>
using namespace cv;
using namespace std;
//class CameraManager;
/*
    get mat from src  every XX msec , give mat to video handler
    if video src fail to create  or fail to fetch mat, signal will send to this camera ,
    frequency will turn down, and try restart video src per XXX sencond
*/

class Camera : public QThread
{
    Q_OBJECT
public:
    bool quit_work;
    explicit Camera(camera_data_t dat) : data(dat),p_video_src(NULL)
    {
        quit_work=false;
        tick=0;
        tick_work=0;
        connected=false;
        create_video_src();
    }
    ~Camera(){
        if(quit_work==false){
            prt(info,"waitin for thread quit");
            quit_work=true;
            //     QThread::msleep(1000);
        }
        this->exit();
        this->wait();

        delete p_video_src;
        p_video_src=NULL;
    }

    void create_video_src()
    {
        if(p_video_src!=NULL)
        {
//            disconnect(p_video_src,SIGNAL(video_disconnected()),this,SLOT(source_disconnected()));
//            disconnect(p_video_src,SIGNAL(video_connected()),this,SLOT(source_connected()));


            delete p_video_src;
            p_video_src=NULL;
        }
        p_video_src=new VideoSrc(data.ip);
//        connect(p_video_src,SIGNAL(video_disconnected()),this,SLOT(source_disconnected()));
//        connect(p_video_src,SIGNAL(video_connected()),this,SLOT(source_connected()));

        connect(this,SIGNAL(restart_source()),this,SLOT(source_disconnected()));
        connect(this,SIGNAL(restart_source()),this,SLOT(source_connected()));

        if(p_video_src->video_connected_flag==true)
            source_connected();
        else
            source_disconnected();
    }

    void restart(camera_data_t dat)
    {
        data=dat;
    }
#ifdef CLIENT
    QWidget *get_render()
    {
        return video_handler.get_render();
    }
#endif
protected:
    virtual void run()
    {
        while(quit_work==false)
        {
            //         prt(info,"runing %s",data.ip.toStdString().data());
            if(work()!=true){
            //    source_disconnected();
                emit restart_source();
          //      break;
            }
            QThread::msleep(45);
        }
    }

signals:
    void restart_source();
public slots:
    void source_connected()
    {
        prt(info,"video connected");
        connected=true;
    }
    void source_disconnected()
    {//   lock.lock();
        //  if(connected==true){//avoid multiple call
        prt(info,"video disconnected");
        connected=false;
        create_video_src();
        // }
        // lock.unlock();
    }
    /*
    video src -> fetch mat
    video handler -> handle mat
    timer (always alive) work per xx mseconds
    */
    bool work()
    {
        bool ret=true;
  //      if(connected==true){
            if(p_video_src!=NULL){
            tick++;
            //            if(tick==200){
            //                // prt(info,"restart video");
            //                //   std::this_thread::sleep_for(chrono::milliseconds(1000));
            //                //            delete p_video_src;
            //                //            //       std::this_thread::sleep_for(chrono::milliseconds(1000));
            //                //            //     p_video_src=new VideoSrc(data.ip);
            //                //            p_video_src=create_video_src();
            //                create_video_src();
            //                tick=0;
            //            }
            //    IplImage *f=p_video_src->fetch_frame();
            Mat *f=p_video_src->get_frame();
            if(1){
                if(f==NULL){
                    prt(info,"get null frame");
                //    source_disconnected();
                    ret=false;
                }else
                {
                    video_handler.set_frame(f);
                    video_handler.work("test url");
                }
            }else{
                //      prt(info,"sleep start");
                //     std::this_thread::sleep_for(chrono::milliseconds(2000));
                //    prt(info,"sleep end");
            }
        }else{
            prt(info,"%s , work ignored cuz unconnected",data.ip.toStdString().data());
        //    source_disconnected();
            ret=false;
        }
        return ret;
    }
private:
    camera_data_t data;//data that camera need
    //    QTimer *timer;//do work per xx micro seconds
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
        use_camera_config();
    }
    ~CameraManager(){

        for(int i=0;i<p_cfg->data.camera_amount;i++){
            // delete cams[i];
            del_camera_internal(i);
        }
    }

    void use_camera_config()
    {
        foreach (Camera *c, cams) {
            delete c;
        }
        int num;
        cams.clear();
        for(int i=0;i<p_cfg->data.camera_amount;i++){
            //            Camera *c=new Camera(p_cfg->data.camera[i]);
            //            cams.append(c);
            add_camera_internal(i);
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
        //        Camera *c=new Camera(p_cfg->data.camera[p_cfg->data.camera_amount-1]);
        //        cams.append(c);
        add_camera_internal();
    }
    void add_camera_internal()
    {
        //        Camera *c=new Camera(p_cfg->data.camera[p_cfg->data.camera_amount-1]);
        //        cams.append(c);
        add_camera_internal(p_cfg->data.camera_amount-1);

    }
    void add_camera_internal(int index)
    {
        Camera *c=new Camera(p_cfg->data.camera[index]);
        cams.append(c);
        c->start();
        prt(info,"cam %d append",index);
    }
    void del_camera_internal(int index)
    {
        delete cams[index];
        cams.removeAt(index);
        prt(info,"cam %d deleted",index);
    }
    void add_camera(QString ip)
    {
        //         Camera *c=new Camera(cfg.data.camera[i]);

        //        camera_data_t ca;
        //        ca.ip=ip;
        //        ca.port=554;
        //        p_cfg->data.camera.append(ca);
        //        p_cfg->data.camera_amount++;

        p_cfg->append_camera(ip,554);
        add_camera_internal();
        //        Camera *c=new Camera(p_cfg->data.camera[p_cfg->data.camera_amount-1]);
        //        cams.append(c);
        //  p_cfg->save();
        //  if(i==0)
        //    connect(c->p_src,SIGNAL(frame_update(Mat)),&c->render,SLOT(set_mat(Mat)));
        //   if(i==0)
        //    layout->addWidget(&c->render,1,cams.length()-1);
    }
    void del_camera(int cam_no)
    {
        if(cam_no<=p_cfg->data.camera_amount&&cam_no>0){
            //            p_cfg->data.camera.removeAt(index-1);
            //            p_cfg->data.camera_amount--;
            //            p_cfg->save_config_to_file();
            p_cfg->del_camera(cam_no);
            del_camera_internal(cam_no-1);
            //            delete cams[index-1];
            //            cams.removeAt(index-1);
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
