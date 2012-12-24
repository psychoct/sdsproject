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
        virtual double calculateSegmentLength(double prevX, double x);
        virtual const char* oppositeOf(const char* ref);
        virtual const char* getGateByRef(const char* ref);

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
    if (getIndex() == 26) {
        /*
        EV << "DHTMember (" << getIndex() << "): starts procedure to calculate its segment length" << endl;
        getSegmentLengthProcedure();
        */
        EV << "DHTMember (" << getIndex() << "): starts procedure to calculate how many nodes are in the net" << endl;
        estimateProcedure();
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
        response->setNeighbour(oppositeOf(neighbour));
        gate = getGateByRef(neighbour);

        /* send it back */
        send(response, gate);

        /* debug action */
        EV << "DHTMember (" << getIndex() << "): " << neighbour << " asked for my x, that is: " << x << ". Sending it back" << endl;
    } else if (strcmp(request->getFullName(), "myX") == 0) {
        /* on "my x" message current node received the value of x it asked for,
         * segment length for current node is updated accordingly.
         */
        double prevX = request->getX();
        segmentLength = calculateSegmentLength(prevX, x);

        /* debug action */
        EV << "DHTMember (" << getIndex() << "): " << request->getNeighbour() << " sent its x, that is: " << prevX << ". My x is " << x << ". Segment length is now " << segmentLength << endl;
    } else if (strcmp(request->getFullName(), "askForSegmentLength") == 0) {
        getSegmentLengthProcedure();

        /* segment length will be available in 0.3
         * simtime steps, wait for that
         */
        response = new Packet("segmentLengthCalculated");
        response->setNeighbour(request->getNeighbour());
        scheduleAt(simTime() + 0.3, response);

        /* debug action */
        EV << "DHTMember (" << getIndex() << "): " << request->getNeighbour() << " asked for my segment length. I started to calculate it, it will take 0.3 simtime steps" << endl;
    } else if (strcmp(request->getFullName(), "segmentLengthCalculated") == 0) {
        /* now segment length for this node is known, send it
         * back to the node who made the request
         */
        const char* neighbour = request->getNeighbour();

        response = new Packet("mySegmentLength");
        response->setSegmentLength(segmentLength);
        response->setNeighbour(oppositeOf(neighbour));

        send(response, getGateByRef(neighbour));

        /* debug action */
        EV << "DHTMember (" << getIndex() << "): " << neighbour << " asked for my segment length. I calculated it, it is: "<< segmentLength << ", sending it back" << endl;
    } else if (strcmp(request->getFullName(), "mySegmentLength") == 0) {
        /* If neighbours segments lengths are received sum them up
         * and (when both are available) use them to calculate an
         * estimate of the number of nodes in the net
         */
        neighboursSegmentsLengths += request->getSegmentLength();
        segmentsReceived++;

        EV << "DHTMember (" << getIndex() << "): " << request->getNeighbour() << " sent me its segment length that is " << request->getSegmentLength() << ". At the moment I recived " << segmentsReceived << "/2 segments" << endl;

        if (segmentsReceived == 2) {
            n_estimate = 3 / (neighboursSegmentsLengths + segmentLength);
            neighboursSegmentsLengths = 0;
            segmentsReceived = 0;

            EV << "DHTMember (" << getIndex() << "): I received both segments I was looking for, the estimate calculated is " << n_estimate << endl;
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

double DHTMember::calculateSegmentLength(double prevX, double x) {
    if (prevX < x)
        return (x - prevX);

    return (x + (1.0 - prevX));
}

const char* DHTMember::oppositeOf(const char* ref) {
    if (strcmp(ref, "prev") == 0)
        return "next";

    return "prev";
}

const char* DHTMember::getGateByRef(const char* ref) {
    if (strcmp(ref, "prev") == 0)
        return "prevShortLink$o";

    return "nextShortLink$o";
}

void DHTMember::getSegmentLengthProcedure() {
    Packet* request = new Packet("askForX");
    request->setNeighbour("next");

    send(request, "prevShortLink$o");
}

void DHTMember::estimateProcedure() {
    Packet* request1 = new Packet("askForSegmentLength");
    Packet* request2 = new Packet("askForSegmentLength");

    request1->setNeighbour("prev");
    request2->setNeighbour("next");

    /* I ask for my neighbours segments lengths */
    send(request1, "nextShortLink$o");
    send(request2, "prevShortLink$o");

    /* In the meantime I calculate my segment length */
    getSegmentLengthProcedure();
}

void DHTMember::relinkProcedure() {
}

void DHTMember::joinProcedure() {
}

void DHTMember::leaveProcedure() {
}
