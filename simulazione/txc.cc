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


class Txc10 : public cSimpleModule
{
  protected:
    virtual void forwardMessage(Packet* msg);
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
};

Define_Module(Txc10);

void Txc10::initialize()
{
    if (getIndex()==0)
    {
        //cModuleType* moduleType = cModuleType::get("Txc10");
        //cModule* module = moduleType->create("tic", this);

        // Boot the process scheduling the initial message as a self-message.
        Packet* p = new Packet("createNewPacket");
        scheduleAt(0.0, p);
    }
}

void Txc10::handleMessage(cMessage *msg)
{
    Packet* p = check_and_cast<Packet*>(msg);

    if (strcmp(this->getFullName(), "newborn") != 0) {

        cModuleType *modtype = cModuleType::get("simulazione.Txc10");

        cModule *newborn = modtype->createScheduleInit("newborn", this->getParentModule());

        cModule *x5 = this->getParentModule()->getSubmodule("tic", 5);
        cModule *x6 = this->getParentModule()->getSubmodule("tic", 6);

        x5->gate("gate$o", 1)->disconnect();
        x5->gate("gate$i", 1)->disconnect();

        x6->gate("gate$o", 0)->disconnect();
        x6->gate("gate$i", 0)->disconnect();

        newborn->setGateSize("gate", 2);

        x5->gate("gate$o", 1)->connectTo(newborn->gate("gate$i", 0));
        newborn->gate("gate$o", 0)->connectTo(x5->gate("gate$i", 1));

        newborn->gate("gate$o", 1)->connectTo(x6->gate("gate$i", 0));
        x6->gate("gate$o", 0)->connectTo(newborn->gate("gate$i", 1));
    }

    if (strcmp(p->getFullName(), "createNewPacket") == 0)
    {
        char msgname[20];
        sprintf(msgname, "tic-%d", getIndex());
        Packet* newP = new Packet(msgname);
        newP->setSender(0);
        newP->setDestination(18);
        forwardMessage(newP);

        scheduleAt(simTime()+exponential(1), p);
    }
    else if (getIndex() == p->getDestination())
    {
        // Message arrived.
        EV << "Message " << msg << " arrived.\n";
        delete msg;
    }
    else
    {
        // We need to forward the message.
        p->setSender(getIndex());
        forwardMessage(p);
    }
}

void Txc10::forwardMessage(Packet* p)
{
    // In this example, we just pick a random gate to send it on.
    // We draw a random number between 0 and the size of gate `out[]'.
    int n = gateSize("gate$o");
    int k = intuniform(0, n-1);

    cGate* gate = p->getArrivalGate();

    if (gate != NULL) {
        while (n > 1 && k == gate->getIndex()) {
            k = intuniform(0, n-1);
            EV << "Message sent by gate index " << gate->getIndex() << " choosed " << k << "\n";
        }
    }

    EV << "Forwarding message " << p << " on port out[" << k << "]\n";
    send(p, "gate$o", k);
}
