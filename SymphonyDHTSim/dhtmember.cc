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

class DHTMember : public cSimpleModule {
    private:
        int id;
        double x; // x in [0..1[
        // length of the segment between this node and its previous neighbour.
        double segmentLength;
        int n_estimate;
        int n_link_estimate;

        double neighboursSegmentsLengths;
        int segmentsReceived;

    protected:
        virtual void initialize();
        virtual void handleMessage(cMessage* msg);

        virtual double getEstimateRatio();
        virtual bool needToRelink();
        virtual double calculateSegmentLength(int prevX, int x);
        virtual void getSegmentLengthProcedure();
        virtual void estimateProcedure();
        virtual void relinkProcedure();
        virtual void joinProcedure();
        virtual void leaveProcedure();
};

Define_Module(DHTMember);

void DHTMember::initialize() {
    id = getIndex();
    x = getIndex() * (1.0 / (double)getAncestorPar("dhtSize"));
    segmentLength = 0;
    n_estimate = (int)getAncestorPar("dhtSize");
    n_link_estimate = (int)getAncestorPar("dhtSize");

    neighboursSegmentsLengths = 0;
    segmentsReceived = 0;

    WATCH(id);
    WATCH(x);
    WATCH(segmentLength);
    WATCH(n_estimate);
    WATCH(n_link_estimate);

    // DEBUG
    if (getIndex() == 5) {
        EV << "DHTMember (" << getIndex() << "): starts procedure to calculate its segment length" << endl;
        getSegmentLengthProcedure();
    }
}

void DHTMember::handleMessage(cMessage* msg) {
    Packet* request = check_and_cast<Packet*>(msg);
    Packet* response;

    if (strcmp(request->getFullName(), "askForX") == 0) {
        /* on "ask for x" message current value for x variable for this member
         * is sent over the network to the request sender
         */
        const char* neighbour = request->getNeighbour();
        const char* gate;

        /* create a packet containing x value and relative position according to request sender */
        response = new Packet("myX");
        response->setX(x);
        if (strcmp(neighbour , "next") == 0) {
            response->setNeighbour("prev");
            gate = "nextShortLink$o";
        } else {
            response->setNeighbour("next");
            gate = "prevShortLink$o";
        }

        /* send it back */
        send(response, gate);

        /* debug action */
        EV << "DHTMember (" << getIndex() << "): " << neighbour << " asked for my x, that is: " << x << ". Sending it back" << endl;
    } else if (strcmp(request->getFullName(), "myX") == 0) {
        /* on "my x" message current node received the value of x it asked for,
         * segment length for current node is updated accordingly.
         */
        double prevX = request->getX();
        segmentLength = calculateSegmentLength(x, prevX);

        /* debug action */
        EV << "DHTMember (" << getIndex() << "): " << request->getNeighbour() << " sent its x, that is: " << prevX << ". My x is " << x << ". Segment length is now " << segmentLength << endl;
    }/* else if (strcmp(request->getFullName(), "askForSegmentLength") == 0) {
        response = new Packet("segmentLengthCalculated");
        response->setNeighbour(request->getNeighbour());

        getSegmentLength();
        scheduleAt(simTime() + 3.0, response);
    } else if (strcmp(request->getFullName(), "segmentLengthCalculated") == 0) {
        response = new Packet("mySegmentLength");
        response->setSegmentLength(segmentLength);

        send(response, (strcmp(response->getNeighbour(), "prev") == 0) ? "prevShortLink$o" : "nextShortLink$o");
    } else if (strcmp(request->getFullName(), "mySegmentLength") == 0) {
        neighboursSegmentsLengths += request->getSegmentLength();
        segmentsReceived++;

        if (segmentsReceived == 2) {
            n_estimate = 3 / (neighboursSegmentsLengths + segmentLength);
            neighboursSegmentsLengths = 0;
            segmentsReceived = 0;
        }
    }*/
}

double DHTMember::getEstimateRatio() {
    return n_estimate / n_link_estimate;
}

bool DHTMember::needToRelink() {
    double estimateRatio = getEstimateRatio();
    return estimateRatio < 0.5 || estimateRatio > 2;
}

double DHTMember::calculateSegmentLength(int prevX, int x) {
    if (prevX < x) {
        return x - prevX;
    } else {
        return x + (1 - prevX);
    }
}

void DHTMember::getSegmentLengthProcedure() {
    Packet* request = new Packet("askForX");
    request->setNeighbour("next");

    send(request, "prevShortLink$o");

    /* debug action */
    EV << "DHTMember (" << getIndex() << "): asking for x to next neighbour" << endl;
}

void DHTMember::estimateProcedure() {
    /*
    Packet* request1 = new Packet("askForSegmentLength");
    Packet* request2 = new Packet("askForSegmentLength");

    request1->setNeighbour("prev");
    request2->setNeighbour("next");

    send(request1, "nextShortLink$o");
    send(request2, "prevShortLink$o");
    */
}

void DHTMember::relinkProcedure() {
}

void DHTMember::joinProcedure() {
}

void DHTMember::leaveProcedure() {
}
