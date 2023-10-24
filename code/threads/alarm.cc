// alarm.cc
//	Routines to use a hardware timer device to provide a
//	software alarm clock.  For now, we just provide time-slicing.
//
//	Not completely implemented.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "alarm.h"
#include "main.h"



T_SLEEP_LIST :: T_SLEEP_LIST()
{
    pHead = NULL;
    pTail = NULL;
}


bool T_SLEEP_LIST :: IsListEmpty()
{
    bool ret = false;
    if (pHead == NULL) ret = true;
    return ret;
}

void T_SLEEP_LIST :: InsertSleepList(Thread* pThread, int sleepTiming)
{
    // Construct  Node to Insert
    sleepThread *pNode = new sleepThread(pThread, sleepTiming);

    //
     if (IsListEmpty())
     {
         pHead = pNode;
         pTail = pNode;
     }
     else
     {
        sleepThread *pCurrent = pTail;
        bool keepFindFlag = (pCurrent->when > pNode->when) ? true : false;

        while (keepFindFlag)
        {
            if (pCurrent->prev != NULL)
            {
                pCurrent = pCurrent->prev;
            }
            else
            {
                break;
            }

            keepFindFlag = (pCurrent->when > pNode->when) ? true : false;
        }

        // list has someone priority > you !
        if (!keepFindFlag)
        {
            if (pCurrent->next != NULL)
            {
                (pCurrent->next)->prev = pNode;
                pNode->next = (pCurrent->next);

            }
            pCurrent->next = pNode;
            pNode->prev = pCurrent;
        }
        else
        {
            pCurrent->prev = pNode;
            pNode->next = pCurrent;
            pHead = pNode;
        }

     }

}

Thread* T_SLEEP_LIST :: PopWaittingQueue(void)
{
    sleepThread *pPopNode = pHead;
    Thread*  pThread = NULL;
    if (!IsListEmpty())
    {
        pThread = pHead->sleeper;
        pHead = pHead->next;
        pPopNode->next = NULL;
        if (pHead == NULL)
        {
            pTail = NULL;
        }
        else
        {
            pHead->prev = NULL;
        }
        delete pPopNode;
    }

    return pThread;
}

void sleepList::Get_Current_Interrupt_Val()
{
    //
    //cout << "Get value : " << _current_interrupt << endl;
}

sleepList::sleepList()
{
    //cout << " sleepList Init" << endl;
    pWaitQueue = new T_SLEEP_LIST();
    _current_interrupt = 0;
}
//----------------------------------------------------------------------
// Alarm::Alarm
//      Initialize a software alarm clock.  Start up a timer device
//
//      "doRandom" -- if true, arrange for the hardware interrupts to
//		occur at random, instead of fixed, intervals.
//----------------------------------------------------------------------


Alarm::Alarm(bool doRandom)
{
    //cout << "creat timer " << endl;
    timer = new Timer(doRandom, this);
}


Alarm::~Alarm()
{
    //cout << " delete timer class " << endl;
    delete timer;
}



//----------------------------------------------------------------------
// Alarm::CallBack
//	Software interrupt handler for the timer device. The timer device is
//	set up to interrupt the CPU periodically (once every TimerTicks).
//	This routine is called each time there is a timer interrupt,
//	with interrupts disabled.
//
//	Note that instead of calling Yield() directly (which would
//	suspend the interrupt handler, not the interrupted thread
//	which is what we wanted to context switch), we set a flag
//	so that once the interrupt handler is done, it will appear as
//	if the interrupted thread called Yield at the point it is
//	was interrupted.
//
//	For now, just provide time-slicing.  Only need to time slice
//      if we're currently running something (in other words, not idle).
//	Also, to keep from looping forever, we check if there's
//	nothing on the ready list, and there are no other pending
//	interrupts.  In this case, we can safely halt.
//----------------------------------------------------------------------

void Alarm::CallBack()
{
    //cout << "time alarm" <<endl;
    Interrupt *interrupt = kernel->interrupt;
    MachineStatus status = interrupt->getStatus();
    //_sleepList.Get_Current_Interrupt_Val();
    bool woken = _sleepList.PutToReady();

    kernel->currentThread->setPriority(kernel->currentThread->getPriority() - 1);
    if (status == IdleMode && !woken && _sleepList.IsEmpty())
    {	// is it time to quit?
        cout << "Cond1" << endl;
        if (!interrupt->AnyFutureInterrupts())
        {
            timer->Disable();	// turn off the timer
        }
    }
    else
    {			// there's someone to preempt
        //cout << "Cond2" << endl;
        // There use RR
        if(kernel->scheduler->getSchedulerType() == RR || kernel->scheduler->getSchedulerType() == Priority )
        {
            interrupt->YieldOnReturn();
        }
    }
}




void Alarm::WaitUntil(int timing)
{
    IntStatus oldLevel = kernel->interrupt->SetLevel(IntOff);
    Thread* t = kernel->currentThread;
    //cout << "Alarm::WaitUntil go sleep" << endl;
    _sleepList.PutToSleep(t, timing);
    kernel->interrupt->SetLevel(oldLevel);
}


bool sleepList::IsEmpty()
{
    return waittingQueue_sleepThread.size() == 0;
}



void sleepList::PutToSleep(Thread*t, int x)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    waittingQueue_sleepThread.push_back(sleepThread(t, _current_interrupt + x));
    t->Sleep(THREAD_NOT_FINISH);
}


bool sleepList::PutToReady()
{
    bool woken = false;
    _current_interrupt++;
    for (std::list<sleepThread>::iterator it = waittingQueue_sleepThread.begin(); it != waittingQueue_sleepThread.end(); )
    {
        if (_current_interrupt >= it->when)
        {
            woken = true;
            //cout << "sleepList::PutToReady Thread woken" << endl;
            kernel->scheduler->ReadyToRun(it->sleeper);
            it = waittingQueue_sleepThread.erase(it);
        }
        else
        {
            it++;
        }
    }
    return woken;
}
