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
        virtual void getSegmentLength(simtime_t when);
        virtual void estimate(simtime_t when);
        virtual void relink();
        virtual void join();
        virtual void leave();
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
        estimate(5.0);
    }
}

void DHTMember::handleMessage(cMessage* msg) {
    Packet* request = check_and_cast<Packet*>(msg);
    Packet* response;

    if (strcmp(request->getFullName(), "askForX") == 0) {
        response = new Packet("myX");
        response->setX(x);
        send(response, (strcmp(response->getNeighbour(), "prev") == 0) ? "prevShortLink$o" : "nextShortLink$o");
    } else if (strcmp(request->getFullName(), "myX") == 0) {
        double prevX = request->getX();
        if (prevX < x) {
            segmentLength = x - prevX;
        } else {
            segmentLength = x + (1 - prevX);
        }
    } else if (strcmp(request->getFullName(), "askForSegmentLength") == 0) {
        response = new Packet("segmentLengthCalculated");
        response->setNeighbour(request->getNeighbour());
        getSegmentLength(simTime());
        scheduleAt(simTime() + 1.0, response);
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
    }
}

double DHTMember::getEstimateRatio() {
    return n_estimate / n_link_estimate;
}

bool DHTMember::needToRelink() {
    double estimateRatio = getEstimateRatio();
    return estimateRatio < 0.5 || estimateRatio > 2;
}

void DHTMember::getSegmentLength(simtime_t when) {
    Packet* request = new Packet("askForX");
    sendDelayed(request, when, "prevShortLink$o");
}

void DHTMember::estimate(simtime_t when) {
    Packet* request1 = new Packet("askForSegmentLength");
    Packet* request2 = new Packet("askForSegmentLength");

    request1->setNeighbour("prev");
    request2->setNeighbour("next");

    sendDelayed(request1, when, "prevShortLink$o");
    sendDelayed(request2, when, "nextShortLink$o");
}

void DHTMember::relink() {
}

void DHTMember::join() {
}

void DHTMember::leave() {
}
