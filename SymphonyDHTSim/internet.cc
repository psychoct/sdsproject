//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 2003 Ahmet Sekercioglu
// Copyright (C) 2003-2008 Andras Varga
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//

#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "packet_m.h"

class Internet : public cSimpleModule {
    protected:
        virtual void initialize();
        virtual void handleMessage(cMessage* msg);

        virtual void addMember();
};

Define_Module(Internet);

void Internet::initialize() {
    cMessage* p = new cMessage("addMember");
    scheduleAt(0.0, p);
}

void Internet::handleMessage(cMessage* msg) {
    simtime_t waitNewMember;

    if (strcmp(msg->getFullName(), "addMember") == 0) {
        waitNewMember = exponential(10);
        scheduleAt(simTime() + waitNewMember, msg);

        EV << "Internet: adding member to DHT, next member in " << waitNewMember << " seconds" << endl;
    }
}

void Internet::addMember() {
}
