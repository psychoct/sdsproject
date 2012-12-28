#include <stdio.h>
#include <string.h>
#include <omnetpp.h>

class Internet : public cSimpleModule {
    protected:
        virtual void initialize();
        virtual void handleMessage(cMessage* msg);

        /* symphony DHT protocol methods */
        virtual void addMember();
};

Define_Module(Internet);

/* ===========================================
 * |             omnet++ methods             |
 * ===========================================
 */

void Internet::initialize() {
}

void Internet::handleMessage(cMessage* msg) {
}

/* ===========================================
 * |     symphony DHT protocol methods       |
 * ===========================================
 */

void Internet::addMember() {
}
