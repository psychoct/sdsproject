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

class DHTMember : public cSimpleModule {
    private:
        int id;
        double x; // x in [0..1[
        int n_estimate;
        int n_link_estimate;
    protected:
        virtual void initialize();
        virtual void handleMessage(cMessage* msg);

        virtual void getSegmentLength();
        virtual void estimate();
        virtual void relink();
        virtual void join();
        virtual void leave();
};

Define_Module(DHTMember);

void DHTMember::initialize() {
}

void DHTMember::handleMessage(cMessage* msg) {
}

void DHTMember::getSegmentLength() {
}

void DHTMember::estimate() {
}

void DHTMember::relink() {
}

void DHTMember::join() {
}

void DHTMember::leave() {
}
