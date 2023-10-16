// alarm.h
//	Data structures for a software alarm clock.
//
//	We make use of a hardware timer device, that generates
//	an interrupt every X time ticks (on real systems, X is
//	usually between 0.25 - 10 milliseconds).
//
//	From this, we provide the ability for a thread to be
//	woken up after a delay; we also provide time-slicing.
//
//	NOTE: this abstraction is not completely implemented.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef ALARM_H
#define ALARM_H

#include "copyright.h"
#include "utility.h"
#include "callback.h"
#include "timer.h"
#include <list>
#include "thread.h"

#define  THREAD_NOT_FINISH false
#define  THREAD_FINISH     true




class sleepThread
{
    public:
        sleepThread(Thread* t, int x) : sleeper(t), when(x) {};
        Thread* sleeper;
        int when;
};


class sleepList
{
    public :
        sleepList();
        void PutToSleep(Thread *t, int x); // Put thread to waiting queue for type : Sleep.
        bool PutToReady(); // using time sharing this interrupt to check waiting queue can put to ready queue.
        bool IsEmpty();
        void Get_Current_Interrupt_Val();
    private:
        int _current_interrupt;
        std::list<sleepThread> waittingQueue_sleepThread;
};



// The following class defines a software alarm clock.
class Alarm : public CallBackObj
{
    public:
        Alarm(bool doRandomYield);	// Initialize the timer, and callback
                                    // to "toCall" every time slice.
        //~Alarm() { cout << " delete timer class " << endl;delete timer; }
        ~Alarm();

        void WaitUntil(int timing);	// suspend execution until time > now + timing

    private:
        Timer *timer;		// the hardware timer device
        sleepList _sleepList;
        void CallBack();		// called when the hardware
                                // timer generates an interrupt
};

#endif // ALARM_H
