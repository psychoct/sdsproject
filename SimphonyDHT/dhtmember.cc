#include <stdio.h>
#include <string.h>
#include <math.h>
#include <omnetpp.h>
#include "packet_m.h"

class DHTMember : public cSimpleModule {
    private:
        /* x: represents the position in the unit interval of this node
         * segmentLength: length of the segment between this node and its previous neighbour
         * nEstimate: estimated number of nodes in the DHT
         * nEstimateAtLinking: estimated number of nodes in the DHT during last relinking
         */
        double x;
        double segmentLength;
        int nEstimate;
        int nEstimateAtLinking;

        /* private variable for estimation protocol
         *
         * neighboursTotalSegmentsLengths: partial total of short linked neighbours segment lengths
         * receivedSegments: how many segment lenghts were sent to this node during estimation protocol
         */
        double neighboursTotalSegmentsLengths;
        int receivedSegments;

    protected:
        virtual void initialize();
        virtual void handleMessage(cMessage* msg);

        /* utility methods */
        virtual int getGateToModule(cModule* module);
        virtual double getSegmentLengthByPreviousNodesIntervalPosition(double previousNodeX);

        /* symphony DHT protocol methods */
        virtual void calculateSegmentLength();
        virtual void nEstimateProcedure();
};

Define_Module(DHTMember);

/* ===========================================
 * |             omnet++ methods             |
 * ===========================================
 */

void DHTMember::initialize() {
    double DHTSize = (double)getAncestorPar("DHTSize");

    x = getIndex() / DHTSize;
    segmentLength = 0;
    nEstimate = (int)DHTSize;
    nEstimateAtLinking = (int)DHTSize;

    neighboursTotalSegmentsLengths = 0;
    receivedSegments = 0;

    WATCH(x);
    WATCH(segmentLength);
    WATCH(nEstimate);
    WATCH(nEstimateAtLinking);

    /* DEBUG */
    if (getIndex() == 0) {
        /*
        EV << "DHTMember: " << this->getFullName() << " starts procedure to calculate its segment length." << endl;
        calculateSegmentLength();
        */

        EV << "DHTMember: " << this->getFullName() << " starts procedure to estimate the number of nodes in the DHT." << endl;
        nEstimateProcedure();
    }
}

void DHTMember::handleMessage(cMessage* msg) {
    Packet* request = check_and_cast<Packet*>(msg);
    int toSenderGateIndex;
    cGate* toSender;
    Packet* response;

    toSenderGateIndex = getGateToModule(msg->getSenderModule());

    if (toSenderGateIndex >= 0)
        toSender = gate("gate$o", toSenderGateIndex);

    if (request->isName("needYourIntervalPositionToCalculateMySegmentLength")) {
        /* a node asked position of current node on unit interval in order to
         * calculate its segment length. A message with that position in sent back
         */
        response = request->dup();
        response->setName("thisIsMyIntervalPosition");
        response->setX(x);
        send(response, toSender);
        EV << "DHTMember: " << request->getSenderModule()->getFullName() << " asked for interval position of " << this->getFullName() << " that is: " << x << ". Sending it back." << endl;
    } else if (request->isName("thisIsMyIntervalPosition")) {
        /* a message with the position of a node has been received,
         * that was requested in order to calculate current node segment length.
         * current node segment length is then updated
         */
        segmentLength = getSegmentLengthByPreviousNodesIntervalPosition(request->getX());
        EV << "DHTMember: " << request->getSenderModule()->getFullName() << " sent its interval position to " << this->getFullName() << " that is: " << request->getX() << ". Updating segment length to " << segmentLength << "." << endl;
    } else if (request->isName("needYourSegmentLength")) {
        /* a node asked for current node segment length in order to calculate an estimate of
         * the number of nodes in the DHT. Segment length for current node will be
         * available in 0.3 simulated time steps
         */
        calculateSegmentLength();

        response = request->dup();
        response->setName("mySegmentLengthIsReady");
        response->setToSenderGateIndex(toSenderGateIndex);
        scheduleAt(simTime() + 0.3, response);
        EV << "DHTMember: " << request->getSenderModule()->getFullName() << " asked for segment length of " << this->getFullName() << ". This calculus will take 0.3 simulated time steps." << endl;
    } else if (request->isName("mySegmentLengthIsReady")) {
        /* on this self message segment length for this node is known;
         * it is sent back to the node who made the request in order to calculate
         * an estimate of the number of nodes in the DHT
         */
        response = request->dup();
        response->setName("thisIsMySegmentLength");
        response->setSegmentLength(segmentLength);
        send(response, gate("gate$o", request->getToSenderGateIndex()));
        EV << "DHTMember: segment length of node " << this->getFullName() << " that is " << segmentLength << " is now ready. Sending it back." << endl;
    } else if (request->isName("thisIsMySegmentLength")) {
        /* the segment length of a neighbour has been received,
         * sum it up to local partial total
         */
        neighboursTotalSegmentsLengths += request->getSegmentLength();
        /* notify a segment has been received */
        receivedSegments++;

        EV << "DHTMember: node " << this->getFullName() << " received segment length of node " << request->getSenderModule()->getFullName() << " that is " << request->getSegmentLength() << " is now ready. Sending it back." << endl;
        EV << "DHTMember: received " << receivedSegments << "/2 segment lengths." << endl;

        /* if both segments have been received update current estimate of
         * the number of nodes in the DHT
         */
        if (receivedSegments == 2) {
            nEstimate = 3 / (neighboursTotalSegmentsLengths + segmentLength);
            neighboursTotalSegmentsLengths = 0;
            receivedSegments = 0;

            EV << "DHTMember: both segment were received. Estimate for n is " << nEstimate << "." << endl;

            /* send back calculated estimate to neighbours */
            int i;
            for (i=0; i<2; i++) {
                response = request->dup();
                response->setName("updateYourEstimate");
                response->setNEstimate(nEstimate);
                send(response, "gate$o", i);
            }

            EV << "DHTMember: asking neighbours to update their estimates for n to " << nEstimate << "." << endl;
       }
    } else if (request->isName("updateYourEstimate")) {
        /* a node completed the calculus of an estimate of the number of
         * nodes in the DHT. It notified to current node that value,
         * then estimate of the number of nodes in the DHT is updated
         * for current node too
         */
        nEstimate = request->getNEstimate();
        EV << "DHTMember: " << request->getSenderModule()->getFullName() << " asked to " << this->getFullName() << " to update its estimate for n that is " << request->getNEstimate() << "." << endl;
    }

    delete request;
}


/* ===========================================
 * |             utility methods             |
 * ===========================================
 */

/* returns the index of the first output gate for current node which is connected
 * to the module taken in input. Returns -1 if no such gate exists
 */
int DHTMember::getGateToModule(cModule* module) {
    int i;
    cGate* nextGate;
    for (i=0; i<gateSize("gate$o"); i++) {
        nextGate = gate("gate$o", i)->getNextGate();
        if (nextGate != NULL && nextGate->getOwnerModule() == module) {
            return i;
        }
    }

    return -1;
}

/* given the position on the unit interval of the previous node of the current node
 * the length of the segment managed by current node is returned
 */
double DHTMember::getSegmentLengthByPreviousNodesIntervalPosition(double previousNodeX) {
    if (previousNodeX < x)
        return x - previousNodeX;

    return x + 1.0 - previousNodeX;
}

/* ===========================================
 * |     symphony DHT protocol methods       |
 * ===========================================
 */

void DHTMember::calculateSegmentLength() {
    Packet* request = new Packet("needYourIntervalPositionToCalculateMySegmentLength");
    send(request, "gate$o", 0);
}

void DHTMember::nEstimateProcedure() {
    int i;
    Packet* request;

    /* current node sends a request to short linked neighbours
     * for their segment lengths
     */
    for (i=0; i<2; i++) {
        request = new Packet("needYourSegmentLength");
        send(request, "gate$o", i);
    }

    /* In the meantime segment length for current node is calculated */
    calculateSegmentLength();
}
