/*
 * Txc1.cpp
 *
 *  Created on: Dec 19, 2012
 *      Author: psycho
 */

#include <string>
#include <omnetpp.h>

class Txc1: public cSimpleModule {
private:
    int counter;
protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
};
Define_Module(Txc1);

void Txc1::initialize() {

    counter = par("limit");
    WATCH(counter);

    if (par("sendMsgOnInit").boolValue() == true)
    {
        EV << "Sending initial message\n";
        cMessage *msg = new cMessage("tictocMsg");
        send(msg, "out");
    }
}

void Txc1::handleMessage(cMessage *msg) {
    counter--;
    if (counter == 0) {
        EV << getName() << "'s counter reached zero, deleting message\n";
        delete msg;
    } else {
        EV << getName() << "'s counter is " << counter
                  << ", sending back message\n";
        send(msg, "out");
    }

}
