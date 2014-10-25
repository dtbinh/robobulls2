#define _USE_MATH_DEFINES
// to not generate gamemodel: comment out contents of strategy/strategycontroller.cpp->gameModelUpdated()
// colision notification
// different field & robot scales based on SIMULATED
// make bearing dial gray if robot is NULL
// communication/nxtrobcomm.cpp - sets velocity to zero
#include <math.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include "model/gamemodel.h"
#include "model/robot.h"
//#include "communication/visioncomm.h"
#include <string>
#include "utilities/point.h"
#include "model/robot.h"
#include "sys/wait.h"
#include <unistd.h>
#include <signal.h>
#include <chrono>
#include <thread>
#include "guicomm.h"
#include <QLCDNumber>
#include "include/config/simulated.h"
#include <time.h>
#include <math.h>
#include "include/config/simulated.h"
#include <QGraphicsView>
#include <QShortcut>
#include <QMenu>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMap>
#include <QCursor>
#include "communication/refcomm.h"
#include "guidrawline.h"
#include "communication/nxtrobcomm.h"
#include "movement/move.h"
#include <QScrollBar>
#include "robotpanel.h"
#include "selrobotpanel.h"
#include "objectposition.h"

// Global static pointer used to ensure only a single instance of the class.
MainWindow* MainWindow::mw = NULL;

using namespace std;



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);

    gamemodel = GameModel::getModel();

    // Setting up GUI; not enabling thread until we're done
    ui->btn_connectGui->setEnabled(false);
    // Creating helper classes
    selrobotpanel = new SelRobotPanel(this);
    robotPanel = new RobotPanel(this);
    objectPos = new ObjectPosition(this);
    fieldpanel = new FieldPanel(this);
    // Generating GUI
    fieldpanel->setUpScene();
    defaultZoom();
    robotPanel->setupBotPanel();
    setupKeyShortcuts();
    ui->btn_connectGui->setEnabled(true);


    // Create Threads, the parameters are the timer value, and parent.
    // threads is declated as QList<GuiComm*> threads;
    // create threads, and append them to the threads list, so that
    // threads can be accessed for making connections, and to start
    // and stop threads
    threads.append(new GuiComm(50, this));
//    threads.append(new GuiComm(30, this));

    // Connect each Widget to correcponding thread
    connect(threads[0], SIGNAL(valueChanged(int))
            , this, SLOT(launch(int)));


    // Zoom slider
    connect(ui->zoom_slider, SIGNAL(valueChanged(int))
            , this, SLOT(zoomField(int)));

    // Default zoom button
    connect(ui->zoom_default, SIGNAL(clicked())
            , this, SLOT(defaultZoom()));
}

void MainWindow::launch(int value)
{
    if (SIMULATED) {
        ui->menuDashboard->setTitle("Simulation");
    } else {
        ui->menuDashboard->setTitle("Camera");
    }


//    ui->label->setText(QString("Current Thread Processing Status : %1").arg(value));

    // CTRL modifer for field scrolling
    if (QApplication::keyboardModifiers().testFlag(Qt::ControlModifier) == true) {
        ui->gView_field->setDragMode(QGraphicsView::ScrollHandDrag);
        fieldpanel->justScrolled = true;
    } else {
        ui->gView_field->setDragMode(QGraphicsView::NoDrag);
        if (fieldpanel->justScrolled) {
            fieldpanel->justScrolled = false;
            fieldpanel->refresh = true;
        }
    }
    // TEST
    for (int i=0; i<teamSize; i++) {
        botBehavior[i] = "";
//        botBehaviorNew[i] = false;
//        botBehaviorTemp[i] = " ";
    }

    // Updating GUI
    setMyVelocity();
    selrobotpanel->setGuiOverride();
    fieldpanel->updateScene();
    robotPanel->updateBotPanel();
    updateBallInfo();
    scanForSelection();


}





void MainWindow::scanForSelection() {
    bool newSelection = true;

    // Scanning for double-click selection
    for (int i=0; i<teamSize; i++) {
        if (gamemodel->find(i,gamemodel->getMyTeam()) != NULL) {
            if (robotPanel->botIcons[i]->doubleClicked || fieldpanel->guiTeam[i]->doubleClicked) {
                robotPanel->botIcons[i]->doubleClicked = false;
                fieldpanel->guiTeam[i]->doubleClicked = false;
                fieldpanel->centeredBotID = i;
                centerViewOnBot();
                zoomField(20);
                guiPrint("Focused on Robot " + to_string(fieldpanel->centeredBotID));
                break;
            }
        }//nullcheck
    }//end for

    // Scrolling the camera removes centeredOn but not selection
    if (fieldpanel->justScrolled) {
        for (int i=0; i<teamSize; i++) {
            if (gamemodel->find(i,gamemodel->getMyTeam()) != NULL) {
                robotPanel->botIcons[i]->doubleClicked = false;
                fieldpanel->guiTeam[i]->doubleClicked = false;
            }//nullcheck
        }
        fieldpanel->centeredBotID = -1;
    }
    // Field/Sidelines clicked removes centeredOn and selection
    if (fieldpanel->field->Pressed == true) {
        fieldpanel->field->highlighted = true;
        fieldpanel->field->Pressed = false;
    }
    if (fieldpanel->sidelines->Pressed == true) {
        fieldpanel->sidelines->highlighted = true;
        fieldpanel->sidelines->Pressed = false;
    }
    if (fieldpanel->field->highlighted || fieldpanel->sidelines->highlighted) {
        for (int i=0; i<teamSize; i++) {
            if (gamemodel->find(i,gamemodel->getMyTeam()) != NULL) {
                fieldpanel->guiTeam[i]->highlighted = false;
                fieldpanel->guiTeam[i]->setSelected(false);
                robotPanel->botIcons[i]->highlighted = false;
                robotPanel->botIcons[i]->setSelected(false);
                robotPanel->botIcons[i]->doubleClicked = false;
                fieldpanel->guiTeam[i]->doubleClicked = false;
            }//nullcheck
        }
        fieldpanel->field->highlighted = false;
        fieldpanel->sidelines->highlighted = false;
        fieldpanel->selectedBot = -1;
        fieldpanel->centeredBotID = -1;
        newSelection = true;
    }

    for (int i=0; i<teamSize; i++) {
        if (gamemodel->find(i,gamemodel->getMyTeam()) != NULL) {
        // Bots on the panel clicked
            if (robotPanel->botIcons[i]->isSelected()) {
                fieldpanel->selectedBot = i;
                for (int j=0; j<teamSize; j++) {
                    fieldpanel->guiTeam[j]->highlighted = false;
                    fieldpanel->guiTeam[j]->setSelected(false);
                    robotPanel->botIcons[j]->highlighted = false;
                    robotPanel->botIcons[j]->setSelected(false);
//                    botIcons[i]->doubleClicked = false;
//                    guiTeam[i]->doubleClicked = false;

                }
//                field->highlighted = false;
                robotPanel->botIcons[i]->highlighted = true;
                fieldpanel->guiTeam[i]->highlighted = true;
                fieldpanel->refresh = true;
                // Refresh GUI
                for (int r=0; r<teamSize; r++) {
                    robotPanel->botIconFrames[i]->update();
                }
                ui->gView_robot_prime->hide();
                ui->gView_robot_prime->show();
                newSelection = true;
            }
            // Bots on the field clicked
            if (fieldpanel->guiTeam[i]->isSelected()) {
                fieldpanel->selectedBot = i;
                for (int j=0; j<teamSize; j++) {
                    fieldpanel->guiTeam[j]->highlighted = false;
                    fieldpanel->guiTeam[j]->setSelected(false);
                    robotPanel->botIcons[j]->highlighted = false;
                    robotPanel->botIcons[j]->setSelected(false);
//                    botIcons[i]->doubleClicked = false;
//                    guiTeam[i]->doubleClicked = false;
                }
//                field->highlighted = false;
                robotPanel->botIcons[i]->highlighted = true;
                fieldpanel->guiTeam[i]->highlighted = true;
                fieldpanel->refresh = true;
                // Refresh GUI
                for (int r=0; r<teamSize; r++) {
                    robotPanel->botIconFrames[i]->update();
                }
                ui->gView_robot_prime->hide();
                ui->gView_robot_prime->show();
                newSelection = true;
            }
        }//null check
    }//for loop
    if (newSelection) {
        updateSelectedBotPanel(fieldpanel->selectedBot);
    } else { return; }

}

int MainWindow::getVelocity(int id) {
    int velocity = 0;
    int wheels = 0;
    int LF = 0;
    int RF = 0;
    int LB = 0;
    int RB = 0;

    if ( gamemodel->find(id, gamemodel->getMyTeam())->type() == fourWheelOmni ) {
        guiPrintRobot(id,"fourWheelOmni");
//        if (SIMULATED) {
            LF = gamemodel->find(id, gamemodel->getMyTeam())->getLF();
            RF = gamemodel->find(id, gamemodel->getMyTeam())->getRF();
            LB = gamemodel->find(id, gamemodel->getMyTeam())->getLB();
            RB = gamemodel->find(id, gamemodel->getMyTeam())->getRB();
//        } else {
//            LF = nxtrobcomm->gui_left_front;
//            RF = nxtrobcomm->gui_right_front;
//            LB = nxtrobcomm->gui_left_back;
//            RB = nxtrobcomm->gui_right_back;
//        }
//        cout << "4wheel Robot " << id << ": " << LF << ", " << RF << "\n";
            velocity += LF;
            wheels++;
            velocity += RF;
            wheels++;
            velocity += LB;
            wheels++;
            velocity += RB;
            wheels++;
    } else if ( gamemodel->find(id, gamemodel->getMyTeam())->type() == differential ) {
        guiPrintRobot(id,"differential");
//        if (SIMULATED) {
            LF = gamemodel->find(id, gamemodel->getMyTeam())->getL();
            RF = gamemodel->find(id, gamemodel->getMyTeam())->getR();
//        } else {
//            LF = nxtrobcomm->gui_left;
//            RF = nxtrobcomm->gui_right;
//        }
//        cout << "diff Robot " << id << ": " << LF << ", " << RF << "\n";
            velocity += LF;
            wheels++;
            velocity += RF;
            wheels++;
    } else if ( gamemodel->find(id, gamemodel->getMyTeam())->type() == threeWheelOmni ) {
        guiPrintRobot(id,"threeWheelOmni");
//        if (SIMULATED) {
            LF = gamemodel->find(id, gamemodel->getMyTeam())->getLF();
            RF = gamemodel->find(id, gamemodel->getMyTeam())->getRF();
//        } else {
//            LF = nxtrobcomm->gui_left_front;
//            RF = nxtrobcomm->gui_right_front;
//        }
        int b = gamemodel->find(id, gamemodel->getMyTeam())->getB();
//        cout << "3wheel Robot " << id << ": " << LF << ", " << RF << "\n";

            velocity += LF;
            wheels++;
            velocity += RF;
            wheels++;
            velocity += b;
            wheels++;
    }

    if (velocity != 0 && wheels != 0)
        velocity /= wheels;

    guiPrintRobot(id,"Wheels: " + to_string(LF) + " & " + to_string(RF));
    return velocity;
}

void MainWindow::drawLine(int originX, int originY, int endX, int endY) {
//    guidrawline = new GuiDrawLine();
//    guidrawline->setZValue(3);
//    guidrawline->setX(100);
//    guidrawline->setY(100);
//    scene->addItem(guidrawline);

        guidrawline->x1 = originX;
        guidrawline->y1 = originY;
        guidrawline->x2 = endX;
        guidrawline->y2 = endY;
        ui->gView_field->update();
}

void MainWindow::guiPrint(string output) {
//    guiOutput.insert(0, QString::fromStdString(output));
    ui->text_guiPrint->setTextColor(Qt::white);
    if (guiOutput.toStdString() == output) {

    } else {
        // recording this string
        guiOutput = QString::fromStdString(output);
        // converting received string to QString for printing
        QString msg = QString::fromStdString(output);
        ui->text_guiPrint->append(msg);
        // Scrolling to bottom of text box
        QScrollBar *sb = ui->text_guiPrint->verticalScrollBar();
        sb->setValue(sb->maximum());
    }
}




void MainWindow::on_btn_connectGui_clicked()
{
    if(ui->btn_connectGui->text() == "Connect")
    {
        ui->btn_connectGui->setText("Disconnect");
        for(int i = 0; i < threads.count(); i++)
            threads[i]->start();
    }
    else
    {
        ui->btn_connectGui->setText("Connect");
        for(int i = 0; i < threads.count(); i++)
            threads[i]->exit(0);
    }
}



QString MainWindow::getRemTime() {
    QString t;  // return value
    std::string time = std::to_string(gamemodel->getRemainingTime());
    t = QString::fromStdString(time);

    return t;
}


void MainWindow::updateBallInfo() {
    ui->lcd_coordX_ball->display(objectPos->getBallCoordX());
    ui->lcd_coordY_ball->display(objectPos->getBallCoordY());

    robotPanel->ballIcon->color = ui->combo_ballColor->currentText();
    ui->gView_ball->update();
    // Displaying ball icon
    if (ui->gView_ball->isHidden()) {
        ui->gView_ball->show();
    }
}

void MainWindow::on_pushButton_2_clicked()
{
    int rAngle = -45;
    ui->gView_field->rotate(rAngle);
    fieldpanel->currentFieldAngle += rAngle;
}

void MainWindow::on_pushButton_3_clicked()
{
    int lAngle = 45;
    ui->gView_field->rotate(lAngle);
    fieldpanel->currentFieldAngle += lAngle;
}

int MainWindow::frequency_of_primes (int n) {
  int i,j;
  int freq=n-1;
  for (i=2; i<=n; ++i) for (j=sqrt(i);j>1;--j) if (i%j==0) {--freq; break;}
  return freq;
}

int MainWindow::getClock()
{
    time_t now;
     struct tm newyear;
     double seconds;

     time(&now);  /* get current time; same as: now = time(NULL)  */

     newyear = *localtime(&now);

     newyear.tm_hour = 0; newyear.tm_min = 0; newyear.tm_sec = 0;
     newyear.tm_mon = 0;  newyear.tm_mday = 1;

     seconds = difftime(now,mktime(&newyear));

     printf ("%.f seconds since new year in the current timezone.\n", seconds);

     return 0;
}



int MainWindow::getSpeed(QGraphicsItem *p, double o)
{
    // Worked, then mysteriously started crashing at the second line (p reference)
    int speed = 0;
    double currentPos = p->y() / p->x();
    speed = currentPos - o;
    o = currentPos;
    speed *= 10;
    if (speed < 0) {
        speed *= -1;
    }
//    cout << "ball speed: " << speed << "\n";
    return speed;

}

void MainWindow::moveBot() {

}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    // Robot binds
    if (fieldpanel->selectedBot > -1) {
        switch(event->key()) {
            case Qt::Key_W:
                on_btn_botForward_pressed();
                break;
            case Qt::Key_A:
                on_btn_botTurnLeft_pressed();
                break;
            case Qt::Key_S:
                on_btn_botReverse_pressed();
                break;
            case Qt::Key_D:
                on_btn_botTurnRight_pressed();
                break;
            // Alternate arrow bindings. Don't seem to work :(
            case Qt::UpArrow:
                on_btn_botForward_pressed();
                break;
            case Qt::LeftArrow:
                on_btn_botTurnLeft_pressed();
                break;
            case Qt::DownArrow:
                on_btn_botReverse_pressed();
                break;
            case Qt::RightArrow:
                on_btn_botTurnRight_pressed();
                break;

            case Qt::Key_Space:
                on_btn_botKick_pressed();
                break;
            case Qt::Key_J:
                on_btn_botDrible_pressed();
                break;
            // Hide/show bot icon
//            case Qt::Key_Delete:
//                cout << "delete \n";
//                break;
//            case Qt::Key_0:
//                selectedBot = 0;
//                break;
//            case Qt::Key_1:
//                robot1->setSelected(true);
//                break;
            // Joystick test

        }
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event) {
    // Robot binds
    if (fieldpanel->selectedBot > -1) {
        switch(event->key()) {
            case Qt::Key_W:
                on_btn_botForward_released();
                break;
            case Qt::Key_A:
                on_btn_botTurnLeft_released();
                break;
            case Qt::Key_S:
                on_btn_botReverse_released();
                break;
            case Qt::Key_D:
                on_btn_botTurnRight_released();
                break;

            case Qt::Key_Up:
                on_btn_botForward_released();
                break;
            case Qt::Key_Left:
                on_btn_botTurnLeft_released();
                break;
            case Qt::Key_Down:
                on_btn_botReverse_released();
                break;
            case Qt::Key_Right:
                on_btn_botTurnRight_released();
                break;

            case Qt::Key_Space:
                on_btn_botKick_released();
                break;
            case Qt::Key_J:
                on_btn_botDrible_released();
                break;
        }
    }
}


void MainWindow::centerViewOnBot() {
    // Centering camera on double-clicked bot
    if (fieldpanel->centeredBotID > -1) {
        ui->gView_field->centerOn(fieldpanel->guiTeam[fieldpanel->centeredBotID]);

    }
}

void MainWindow::setMyVelocity() {
    if (QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier) == true) {
//        cout << "Shift \n";
        myVelocity = 100;
    } else if (QApplication::keyboardModifiers().testFlag(Qt::AltModifier) == true) {
        cout << "Alt \n";
        myVelocity = 25;
    } else {
        myVelocity = 50;
    }
}

void MainWindow::guiPrintRobot(int robotID, string output) {
////    guiOutput.insert(0, QString::fromStdString(output));
////    botBehavior[robotID] = " ";
////    botBehaviorTemp[robotID] = " ";
////    botBehaviorNew[robotID] = false;

////    if (guiOutputRobot.toStdString() == output) {
//    if (botBehaviorTemp[robotID] == QString::fromStdString(output)) {
//        botBehaviorNew[robotID] = false;
////        cout << "matches \n";
//    } else {
////        cout << "does NOT match \n";
//        botBehaviorNew[robotID] = true;
//        // recording this string
////        guiOutputRobot = QString::fromStdString(output);
//        botBehaviorTemp[robotID] = QString::fromStdString(output);
//        // converting received string to QString for printing
//        QString qOutput = QString::fromStdString(output);
//        QString qBehavior = botBehavior[robotID];
//        qBehavior.append(qOutput);
//        botBehavior[robotID] = qBehavior;
//    }
    if (botBehavior[robotID] != ("\n" + QString::fromStdString(output)) ) {
        // if the field is blank, no carriage return
        if (botBehavior[robotID] == "") {
            botBehavior[robotID] += (QString::fromStdString(output));
        } else {
            botBehavior[robotID] += ("\n" + QString::fromStdString(output));
        }
    }

}


void MainWindow::setupKeyShortcuts() {
    QShortcut *enter = new QShortcut(this);
    enter->setKey(Qt::Key_Enter);

    QShortcut *backspace = new QShortcut(this);
    backspace->setKey(Qt::Key_Backspace);

    QShortcut *o = new QShortcut(this);
    o->setKey(Qt::Key_O);

    QShortcut *delKey = new QShortcut(this);
    delKey->setKey(Qt::Key_Delete);

//    QShortcut *spaceBar = new QShortcut(this);
//    spaceBar->setKey(Qt::Key_Space);

    // Connecting key signals to their respective slots
    connect(enter, SIGNAL(activated()), this, SLOT(on_btn_connectGui_clicked()));
    connect(backspace, SIGNAL(activated()), this, SLOT(on_btn_connectGui_clicked()));
    connect(o, SIGNAL(activated()), ui->check_botOverride, SLOT(click()));
    connect(delKey, SIGNAL(activated()), this, SLOT(toggleIconVisible()));
    //    connect(delKey, SIGNAL(activated()), this, SLOT(on_btn_connectGui_clicked()));
}


MainWindow::~MainWindow()
{
    //delete ui;
    while(threads.count() > 0)
    {
        QThread* thread = threads.takeFirst();
        if(thread->isRunning())
            thread->exit(0);
    }
}


MainWindow *MainWindow::getMainWindow() {
    if (mw == NULL) {
        mw = new MainWindow();
    }

    return mw;
}





void MainWindow::updateSelectedBotPanel(int id)
{
    int v = 0;
    if (id == -1) {
        ui->gView_robot_prime->hide();
        ui->dial_botSpeed->setValue(0);
        ui->lcd_botSpeed->display(0);
        ui->lcd_orient_prime->display("0");
        ui->lcd_coordX_prime->display("0");
        ui->lcd_coordY_prime->display("0");
        ui->dial_botOrient_prime->setValue(0);
        ui->box_primeBot->setTitle(" ");
        ui->text_primeBot->setText(" ");
    } else {
        v = getVelocity(id);
        ui->gView_robot_prime->setScene(robotPanel->botIconSelScenes[id]);
        ui->gView_robot_prime->show();
        ui->dial_botSpeed->setValue(v);
        ui->lcd_botSpeed->display(v);
        ui->lcd_orient_prime->display(objectPos->getBotOrientString(id));
        ui->lcd_coordX_prime->display(objectPos->getBotCoordX(true, id));
        ui->lcd_coordY_prime->display(objectPos->getBotCoordY(true,id));
        ui->dial_botOrient_prime->setValue(objectPos->getBotOrientDouble(true, id));
        ui->box_primeBot->setTitle("Robot " + QString::number(id));

        // Text field
        ui->text_primeBot->setTextColor(Qt::white);
        ui->text_primeBot->setText(botBehavior[id]);
        QScrollBar *sb = ui->text_primeBot->verticalScrollBar();
        sb->setValue(sb->maximum());
    }

}

void MainWindow::on_btn_botForward_pressed() {
    if (fieldpanel->selectedBot > -1 && ui->check_botOverride->isChecked()) {

        ui->btn_botForward->setDown(true);
        setMyVelocity();
        int currentFwd = getVelocity(fieldpanel->selectedBot);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setL(currentFwd);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setR(currentFwd);
        if (currentFwd <= 0) {
            gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setL(currentFwd+myVelocity);
            gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setR(currentFwd+myVelocity);
        }
    }
}

void MainWindow::on_btn_botForward_released() {
    if (fieldpanel->selectedBot > -1 && ui->check_botOverride->isChecked()) {
        ui->btn_botForward->setDown(false);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setL(0);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setR(0);
    }
}

void MainWindow::on_btn_botTurnRight_pressed() {
    if (fieldpanel->selectedBot > -1 && ui->check_botOverride->isChecked()) {
        ui->btn_botTurnRight->setDown(true);
        int currentVel = getVelocity(fieldpanel->selectedBot);
        float currentL = gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->getL();
        float currentR = gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->getR();
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setL(currentL+myVelocity/2);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setR(currentR-myVelocity/2);
    }
}

void MainWindow::on_btn_botTurnRight_released() {
    if (fieldpanel->selectedBot > -1 && ui->check_botOverride->isChecked()) {
        ui->btn_botTurnRight->setDown(false);
        float currentFwd = getVelocity(fieldpanel->selectedBot);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setL(currentFwd);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setR(currentFwd);
    }
}

void MainWindow::on_btn_botTurnLeft_pressed() {
    if (fieldpanel->selectedBot > -1 && ui->check_botOverride->isChecked()) {
        ui->btn_botTurnLeft->setDown(true);
        int currentVel = getVelocity(fieldpanel->selectedBot);
        int currentL = gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->getL();
        int currentR = gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->getR();
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setL(currentL-myVelocity/2);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setR(currentR+myVelocity/2);
    }
}

void MainWindow::on_btn_botTurnLeft_released() {
    if (fieldpanel->selectedBot > -1 && ui->check_botOverride->isChecked()) {
        ui->btn_botTurnLeft->setDown(false);
        float currentVel = getVelocity(fieldpanel->selectedBot);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setL(currentVel);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setR(currentVel);
    }
}

void MainWindow::on_btn_botReverse_pressed() {
    if (fieldpanel->selectedBot > -1 && ui->check_botOverride->isChecked()) {
        ui->btn_botReverse->setDown(true);
        int currentVel = getVelocity(fieldpanel->selectedBot);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setL(currentVel);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setR(currentVel);
        if (currentVel >= 0) {
            gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setL(currentVel-myVelocity);
            gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setR(currentVel-myVelocity);
        }
    }
}

void MainWindow::on_btn_botReverse_released() {
    if (fieldpanel->selectedBot > -1 && ui->check_botOverride->isChecked()) {
        ui->btn_botReverse->setDown(false);
        int currentVel = getVelocity(fieldpanel->selectedBot);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setL(0);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setR(0);
    }
}

void MainWindow::on_btn_botKick_pressed() {
    if (fieldpanel->selectedBot > -1 && ui->check_botOverride->isChecked()) {
        ui->btn_botKick->setDown(true);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setKick(true);
        fieldpanel->guiTeam[fieldpanel->selectedBot]->kicking = true;
    }
}

void MainWindow::on_btn_botKick_released() {
    if (fieldpanel->selectedBot > -1 && ui->check_botOverride->isChecked()) {
        ui->btn_botKick->setDown(false);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setKick(false);
        fieldpanel->guiTeam[fieldpanel->selectedBot]->kicking = false;
    }
}

void MainWindow::on_btn_botDrible_pressed() {
    if (fieldpanel->selectedBot > -1 && ui->check_botOverride->isChecked()) {
        ui->btn_botDrible->setDown(true);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setDrible(true);
        fieldpanel->guiTeam[fieldpanel->selectedBot]->dribling = true;
    }
}

void MainWindow::on_btn_botDrible_released() {
    if (fieldpanel->selectedBot > -1 && ui->check_botOverride->isChecked()) {
        ui->btn_botDrible->setDown(false);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setDrible(false);
        fieldpanel->guiTeam[fieldpanel->selectedBot]->dribling = false;
    }
}


void MainWindow::on_check_botOverride_clicked(bool checked) {
    if (checked) {
        overriddenBots[fieldpanel->selectedBot] = true;
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setL(0);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setR(0);
        gamemodel->find(fieldpanel->selectedBot, gamemodel->getMyTeam())->setB(0);
    } else {
        overriddenBots[fieldpanel->selectedBot] = false;
    }
}

void MainWindow::on_btn_override_all_released() {
    for (int i=0; i<overriddenBots.size(); i++) {
        overriddenBots[i] = true;
    }
    // stopping all bots, so they don't fly off at their current velocities
    for (int i=0; i<teamSize; i++) {
        if (gamemodel->find(i, gamemodel->getMyTeam()) != NULL) {
            gamemodel->find(i, gamemodel->getMyTeam())->setL(0);
            gamemodel->find(i, gamemodel->getMyTeam())->setR(0);
            gamemodel->find(i, gamemodel->getMyTeam())->setB(0);
        }//nullcheck
    }
}

void MainWindow::on_btn_override_none_released() {
    for (int i=0; i<overriddenBots.size(); i++) {
        overriddenBots[i] = false;
    }
}

void MainWindow::zoomField(int zoom) {
    ui->zoom_slider->setValue(zoom);
    double zoomScale = zoom *.01;
    ui->gView_field->setTransform(QTransform::fromScale(zoomScale, zoomScale));
    ui->gView_field->scale(1, -1);
    ui->gView_field->rotate(fieldpanel->currentFieldAngle);
}

void MainWindow::defaultZoom() {
    // Removing robot focus
    fieldpanel->centeredBotID = -1;

    fieldpanel->currentFieldAngle = 0;
    zoomField(11);
    ui->zoom_slider->setValue(11);
//    dash->ui->gView_field->hide();
    ui->gView_field->centerOn(fieldpanel->sidelines);
}

// Field graphical settings which need to be refreshed when changed
void MainWindow::on_check_fieldGrid_clicked(){fieldpanel->refresh = true;}
void MainWindow::on_combo_gridScale_currentIndexChanged(int index){fieldpanel->refresh = true;}
void MainWindow::on_check_coloredGoals_clicked(){fieldpanel->refresh = true;}
void MainWindow::on_combo_fieldColor_currentIndexChanged(int index){fieldpanel->refresh = true;}
void MainWindow::on_check_showIDs_stateChanged(int arg1){fieldpanel->refresh = true;}
void MainWindow::on_combo_botScale_currentIndexChanged(int index){fieldpanel->refresh = true;}

void MainWindow::field_setDragMode() {
//    if (dash->ui->gView_field->dragMode() == QGraphicsView::NoDrag) {
//        dash->ui->gView_field->setDragMode(QGraphicsView::ScrollHandDrag);
//    } else {
//        dash->ui->gView_field->setDragMode(QGraphicsView::NoDrag);
//    }
}

void MainWindow::toggleIconVisible() {
    if (fieldpanel->selectedBot > -1) {
        if (fieldpanel->guiTeam[fieldpanel->selectedBot]->enabled) {
            fieldpanel->guiTeam[fieldpanel->selectedBot]->enabled = false;
            robotPanel->botIcons[fieldpanel->selectedBot]->enabled = false;
            robotPanel->botIcons[fieldpanel->selectedBot]->setOpacity(.3);
            fieldpanel->guiTeam[fieldpanel->selectedBot]->setOpacity(.3);
        } else {
            fieldpanel->guiTeam[fieldpanel->selectedBot]->enabled = true;
            robotPanel->botIcons[fieldpanel->selectedBot]->enabled = true;
            robotPanel->botIcons[fieldpanel->selectedBot]->setOpacity(1);
            fieldpanel->guiTeam[fieldpanel->selectedBot]->setOpacity(1);

        }
    }
}


