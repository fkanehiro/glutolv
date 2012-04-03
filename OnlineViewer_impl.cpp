#include "OnlineViewer_impl.h"
#include "GLmodel.h"
#include <iostream>
#include <hrpModel/ModelLoaderUtil.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <cstdio>
#ifdef __APPLE__
typedef struct{
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int count;
} unnamed_sem_t;
#define sem_t unnamed_sem_t
#define sem_init(x,y,z) { pthread_cond_init(&((x)->cond), NULL); pthread_mutex_init(&((x)->mutex), NULL); (x)->count = z;}
#define sem_wait(x) { pthread_mutex_lock(&((x)->mutex)); if ((x)->count <= 0) pthread_cond_wait(&((x)->cond), &((x)->mutex)); (x)->count--; pthread_mutex_unlock(&((x)->mutex)); }
#define sem_post(x) { pthread_mutex_lock(&((x)->mutex)); (x)->count++; pthread_cond_signal(&((x)->cond)); pthread_mutex_unlock(&((x)->mutex)); }
#else
#include <semaphore.h>
#endif

using namespace OpenHRP;

#define ADD_BODY 0x01
// global variables
int event=0;
std::string name;
OpenHRP::BodyInfo_var binfo;
double aspect = 1.0;
double pan=M_PI/4, tilt=M_PI/16, radius=5;
int prevX, prevY, button=-1, modifiers;
double xCenter=0, yCenter=0, zCenter=0.8;
sem_t sem;

void key(unsigned char c, int x, int y)
{
    //std::cout << "key(" << x << ", " << y << ")" << std::endl;
    GLscene *scene = GLscene::getInstance();
    if (c == 'p'){
        scene->play(); 
    }else if (c == 'q'){
        exit(0);
    }else if (c == 'f'){
        scene->faster();
    }else if (c == 's'){
        scene->slower();
    }else if (c == 'r'){
        scene->record();
    }
}

void display()
{
    //std::cout << "display" << std::endl;
    GLscene *scene = GLscene::getInstance();

    if (event&ADD_BODY){
        GLbody *body = new GLbody(binfo);
        scene->addBody(name, body);
        event &= ~ADD_BODY;
        sem_post(&sem);
    }
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(30,aspect, 0.1, 100);
    
    double xEye = xCenter + radius*cos(tilt)*cos(pan);
    double yEye = yCenter + radius*cos(tilt)*sin(pan);
    double zEye = zCenter + radius*sin(tilt);
    
    gluLookAt(xEye, yEye, zEye,
              xCenter, yCenter, zCenter,
              0,0,1);
    
    glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
    scene->draw();
    glutSwapBuffers();
}

void idle()
{
    //std::cout << "idle" << std::endl;
    GLscene *scene = GLscene::getInstance();
    if (!scene->isPlaying() && !scene->isNewStateAdded() && button == -1){
        usleep(100000);
    }
    glutPostRedisplay();
}

void resize(int w, int h)
{
    //std::cout << "resize(" << w << "," << h << ")" << std::endl;
    glViewport(0, 0, w, h);
    GLscene::getInstance()->setScreenSize(w, h);
    aspect = ((double)w)/h;
}

void mouse(int b, int state, int x, int y)
{
    if (state == GLUT_DOWN){
        prevX = x; prevY = y;
        button = b;
        modifiers = glutGetModifiers();
    }else{
        button = -1;
    }
}

void motion(int x, int y)
{
    //std::cout << "motion(" << x << "," << y << ")" << std::endl;
    int dx = x - prevX;
    int dy = y - prevY;
    if (button == GLUT_LEFT_BUTTON){
        if (modifiers & GLUT_ACTIVE_SHIFT){
            radius *= (1+ 0.1*dy);
            if (radius < 0.1) radius = 0.1; 
        }else{
            pan  -= 0.05*dx;
            tilt += 0.05*dy;
            if (tilt >  M_PI/2) tilt =  M_PI/2;
            if (tilt < -M_PI/2) tilt = -M_PI/2;
        }
    }else if (button == GLUT_RIGHT_BUTTON){
        xCenter += sin(pan)*dx*0.01;
        yCenter -= cos(pan)*dx*0.01;
        zCenter += dy*0.01;
    }
    prevX = x;
    prevY = y;
}

void special(int key, int x, int y)
{
    modifiers = glutGetModifiers();
    if (modifiers & GLUT_ACTIVE_CTRL){
        if (key == GLUT_KEY_RIGHT){
            GLscene::getInstance()->tail();
        }else if (key == GLUT_KEY_LEFT){
            GLscene::getInstance()->head();
        }
        return;
    }

    int delta =  modifiers & GLUT_ACTIVE_SHIFT ? 10 : 1;
    if (key == GLUT_KEY_RIGHT){
        GLscene::getInstance()->next(delta);
    }else if (key == GLUT_KEY_LEFT){
        GLscene::getInstance()->prev(delta);
    }
}
 
void glmain(int argc, char *argv[])
{
    GLscene *scene = GLscene::getInstance();

    sem_init(&sem, 0, 0);
    scene->init(argc, argv);
        
    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutKeyboardFunc(key);
    glutReshapeFunc(resize);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutSpecialFunc(special);
    printf("left button drag: rotate view point\n");
    printf("shift + left button drag: zoom in/out\n");
    printf("right arrow: show next frame\n");
    printf("shift + right arrow: forward 10 frames\n");
    printf("left arrow: rewind 10 frames\n");
    printf("p: playback recorded motion\n");
    printf("f: playback faster\n");
    printf("r: record movie\n");
    printf("s: playback slower\n");
    printf("q: quit this program\n");
    glutMainLoop();
}

OnlineViewer_impl::OnlineViewer_impl(CORBA::ORB_ptr orb, PortableServer::POA_ptr poa)
    :
    orb(CORBA::ORB::_duplicate(orb)),
    poa(PortableServer::POA::_duplicate(poa)),
    scene(GLscene::getInstance())
{
}

OnlineViewer_impl::~OnlineViewer_impl()
{
}
		
PortableServer::POA_ptr OnlineViewer_impl::_default_POA()
{
    return PortableServer::POA::_duplicate(poa);
}
		
void OnlineViewer_impl::update(const WorldState& state)
{
    scene->addState(state);
}

void OnlineViewer_impl::load(const char* name_, const char* url)
{
    if (!scene->findBody(name_)){
        std::cout << "load(" << url << ")" << std::endl;
        binfo = hrp::loadBodyInfo(url, orb);
        name = name_;
        event |= ADD_BODY;
        sem_wait(&sem); 
    }
}

void OnlineViewer_impl::clearLog()
{
    scene->clearLog();
}

void OnlineViewer_impl::clearData()
{
}

void OnlineViewer_impl::drawScene(const WorldState& state)
{
}

void OnlineViewer_impl::setLineWidth(::CORBA::Float width)
{
}

void OnlineViewer_impl::setLineScale(::CORBA::Float scale)
{
}

::CORBA::Boolean OnlineViewer_impl::getPosture(const char* robotId, DblSequence_out posture)
{
    return true;
}

void OnlineViewer_impl::setLogName(const char* name)
{
}

