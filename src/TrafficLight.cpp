#include "TrafficLight.h"
#include <iostream>
#include <random>
#include <stdlib.h>
/* Implementation of class "MessageQueue" */

static const int MINCYCLE_S = 4000;
static const int MAXCYCLE_S = 6000;
static const int SLEEP_MS = 1;

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
    std::unique_lock<std::mutex> ulk(_m);
    _c.wait(ulk, [this] {return !_q.empty();});
    T ret = std::move(_q.front());
    _q.pop_front();
    return ret;
}

template <typename T>
void MessageQueue<T>::send(T&& msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::lock_guard<std::mutex> lk(_m);
    _q.emplace_back(std::move(msg));
    _c.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while(true)
    {
        auto p = _q.receive();
        if (TrafficLightPhase::green == p)
        {
            break;
        }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    std::lock_guard<std::mutex> lk(_mutex);
    return _currentPhase;
}

void TrafficLight::setCurrentPhase()
{
    std::lock_guard<std::mutex> lk(_mutex);
    TrafficLightPhase p = (TrafficLightPhase::green == _currentPhase) ? TrafficLightPhase::red : TrafficLightPhase::green;
    _currentPhase = std::move(p);
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 
    srand(time(NULL));
    int cycles = rand() % (MAXCYCLE_S - MINCYCLE_S) + MINCYCLE_S;
    std::chrono::time_point<std::chrono::system_clock> last = std::chrono::system_clock::now();
    while (true)
    {
        long delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - last).count();
        if (delta > cycles)
        {
            setCurrentPhase();
            _q.send(std::move(getCurrentPhase()));
            last = std::chrono::system_clock::now();
            cycles = rand() % (MAXCYCLE_S - MINCYCLE_S) + MINCYCLE_S;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
    }
}
