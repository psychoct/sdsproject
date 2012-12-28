#include <stdio.h>
#include <string.h>
#include <omnetpp.h>

class Internet : public cSimpleModule {
    protected:
        virtual void initialize();
        virtual void handleMessage(cMessage* msg);

        virtual void addMember();
};

Define_Module(Internet);

void Internet::initialize() {
}

void Internet::handleMessage(cMessage* msg) {
}

void Internet::addMember() {
}
