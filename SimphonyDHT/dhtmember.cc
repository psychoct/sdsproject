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

        /* private variables for estimation protocol
         *
         * neighboursTotalSegmentsLengths: partial total of short linked neighbours segment lengths
         * receivedSegments: how many segment lenghts were sent to this node during estimation protocol
         */
        double neighboursTotalSegmentsLengths;
        int receivedSegments;

        /* private variables for relinking protocol */
        double randomPoint;
        double bestDistanceFoundSoFar;
        int gateIndexToClosestNode;
        int repliesToFindShortestPath;

    protected:
        virtual void initialize();
        virtual void handleMessage(cMessage* msg);

        /* utility methods */
        virtual int getGateToModule(cModule* module);
        virtual double getSegmentLengthByPreviousNodesIntervalPosition(double previousNodeX);
        virtual int getNeighboursNumber();
        virtual bool amIManagerForPoint(double p);
        virtual void broadcast(Packet* msg);
        virtual int getReverseGateIndexByGateIndex(int index);

        /* symphony DHT protocol methods */
        virtual void calculateSegmentLength();
        virtual void calculateNEstimate();
        virtual void relink();
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

    bestDistanceFoundSoFar = 42; /* note: every number >= than 1 could be considered +infinity */
    repliesToFindShortestPath = 0;

    WATCH(x);
    WATCH(segmentLength);
    WATCH(nEstimate);
    WATCH(nEstimateAtLinking);

    /* DEBUG */
    if (getIndex() == 0) {
        /*
        EV << "DHTMember: " << this->getFullName() << " starts procedure to calculate its segment length." << endl;
        calculateSegmentLength();

        EV << "DHTMember: " << this->getFullName() << " starts procedure to estimate the number of nodes in the DHT." << endl;
        calculateNEstimate();
        */

        EV << "DHTMember: " << this->getFullName() << " starts relinking procedure." << endl;
        relink();
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
    } else if (request->isName("amITheManagerOfThisPoint?")) {
        /* current nodes asks itself if it is the manager for
         * randomly generated point. When this message is received
         * current node has updated its segment length
         */
        EV << "DHTMember: node " << this->getFullName() << " asks itself if it is the manager for point " << request->getX() << "." << endl;

        int routinglistSize;
        int previousRequestingNodeGateIndex;

        if (amIManagerForPoint(request->getX())) {
            /* if current node is the manager for randomly generated point */
            EV << "DHTMember: node " << this->getFullName() << " is the manager for that point." << endl;
            routinglistSize = request->getRoutingListArraySize();
            if (routinglistSize > 0) {
                /* if information about the manager have not been transmitted
                 * to the node who made the request for it
                 */
                EV << "DHTMember: node " << this->getFullName() << " is not the requesting node so it passes backwards the message deleting itself from routing list." << endl;

                /* current node passes backwards its message along requesting chain (routing list) */
                previousRequestingNodeGateIndex = request->getRoutingList(routinglistSize - 1);
                response = request->dup();
                response->setName("managerIndexIs");
                response->setManager(getIndex());
                response->setRoutingListArraySize(routinglistSize - 1);
                send(response, gate("gate$o", previousRequestingNodeGateIndex));
            }
        } else {
            /* if current node is NOT the manager for randomly generated point */
            EV << "DHTMember: node " << this->getFullName() << " is NOT the manager for that point. Then current node search for path that minimize distance from randomly generated point." << endl;

            /* it asks to its neighbours their positions on the unit interval in order to
             * choose the best path to the manager of that point
             */
            randomPoint = request->getX();
            response = request->dup();
            response->setName("needYourIntervalPositionToCalculateShortestPath");
            broadcast(response);
        }
    } else if (request->isName("needYourIntervalPositionToCalculateShortestPath")) {
        /* a node needs current node interval position in order to choose the best path
         * to the manager of a randomly generated point, a message with current node interval
         * position is created and sent back to it
         */
        response = request->dup();
        response->setName("thisIsMyIntervalPositionToFindShortestPath");
        response->setX(x);
        send(response, toSender);
    } else if (request->isName("thisIsMyIntervalPositionToFindShortestPath")) {
        EV << "DHTMember: node " << this->getFullName() << " received the interval position of a neighbour in order to calculate shortest path through manager of randomly generated node." << endl;

        int routinglistSize;
        double distanceDeltaToPoint;
        int reverseGateIndex;

        distanceDeltaToPoint = fabs(randomPoint - request->getX());

        EV << "DHTMember: distance of point " << randomPoint << " from that neighbour is " << distanceDeltaToPoint << "." << endl;

        if (distanceDeltaToPoint < bestDistanceFoundSoFar) {
            EV << "DHTMember: that is better than best distance found so far, that is " << bestDistanceFoundSoFar << "." << endl;
            bestDistanceFoundSoFar = distanceDeltaToPoint;
            gateIndexToClosestNode = toSenderGateIndex;
        }

        repliesToFindShortestPath++;

        EV << "DHTMember: node " << this->getFullName() << " received " << repliesToFindShortestPath << "/" << getNeighboursNumber() << " replies." << endl;

        if (repliesToFindShortestPath >= getNeighboursNumber()) {
            EV << "DHTMember: node " << this->getFullName() << " received all the messages from its neighbours, it is now able to find the shortest path to the manager of randomly generated point." << endl;
            routinglistSize = request->getRoutingListArraySize();
            reverseGateIndex = getReverseGateIndexByGateIndex(gateIndexToClosestNode);

            response = request->dup();
            response->setName("areYouTheManagerOfThisPoint?");
            response->setX(randomPoint);
            response->setRoutingListArraySize(routinglistSize + 1);
            response->setRoutingList(routinglistSize, reverseGateIndex);
            send(response, "gate$o", gateIndexToClosestNode);
        }
    } else if (request->isName("areYouTheManagerOfThisPoint?")) {
        response = request->dup();
        response->setName("amITheManagerOfThisPoint?");
        response->setX(request->getX());
        calculateSegmentLength();
        scheduleAt(simTime() + 0.3, response);
    } else if (request->isName("managerIndexIs")) {
        EV << "DHTMember: node " << this->getFullName() << " knows that the manager of randomly generated point is " << request->getManager() << ". Routing this information to the node that asked it." << endl;

        int previousRequestingNodeGateIndex;
        int routinglistSize;

        routinglistSize = request->getRoutingListArraySize();
        if (routinglistSize > 0) {
            EV << "DHTMember: node " << this->getFullName() << " is not the requesting node so it passes the message through a chain to requesting node." << endl;
            previousRequestingNodeGateIndex = request->getRoutingList(routinglistSize - 1);
            response = request->dup();
            response->setName("managerIndexIs");
            response->setManager(getIndex());
            response->setRoutingListArraySize(routinglistSize - 1);
            send(response, gate("gate$o", previousRequestingNodeGateIndex));
        } else {
            EV << "DHTMember: node " << this->getFullName() << " is the requesting node for the manager of randomly generated point so a long link with that manager is estabilished." << endl;
            /*@ TODO, create long link connection */
        }
    }

    /* after every request process delete received message */
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

/* returns the number of neighbours of current node */
int DHTMember::getNeighboursNumber() {
    int i;
    int total;
    total = 0;
    for (i=0; i<gateSize("gate$o"); i++) {
        if (hasGate("gate$o", i) && gate("gate$o", i)->isConnected()) {
            total++;
        }
    }

    return total;
}

/* returns true if current node is the manager of point p.
 * there are two cases in which current node (A, with its previous node B) is the manager of point p:
 * case 1:
 *      [0       B---x--------(A)      1[
 *
 * case 2:
 *      [0-----x--(A)      B-----------1[
 * or
 *      [0--------(A)      B-------x---1[
 */
bool DHTMember::amIManagerForPoint(double p) {
    double delta;
    delta = x - segmentLength;

    if (delta >= 0 && p > delta && p <= x) {
       return true;
    }

    if (delta < 0 && (p > 1 + delta || p <= x)) {
       return true;
    }

    return false;
}

/* sends a copy of the message taken in input to every
 * neighbour of current node
 */
void DHTMember::broadcast(Packet* packet) {
    int i;
    cGate* neighbourGate;
    Packet* packetcp;
    for (i=0; i<gateSize("gate$o"); i++) {
        if (hasGate("gate$o", i)) {
            neighbourGate = gate("gate$o", i);
            if (neighbourGate->isConnected()) {
                packetcp = packet->dup();
                send(packetcp, neighbourGate);
            }
        }
    }
}

int DHTMember::getReverseGateIndexByGateIndex(int index) {
    cGate* fromGate;
    cModule* nextGateModule;
    DHTMember* nextGateDHTModule;

    fromGate = gate("gate$o", index);
    nextGateModule = fromGate->getNextGate()->getOwnerModule();
    nextGateDHTModule = (DHTMember*)nextGateModule;

    return nextGateDHTModule->getGateToModule(this);
}

/* ===========================================
 * |     symphony DHT protocol methods       |
 * ===========================================
 */

void DHTMember::calculateSegmentLength() {
    Packet* request = new Packet("needYourIntervalPositionToCalculateMySegmentLength");
    send(request, "gate$o", 0);
}

void DHTMember::calculateNEstimate() {
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

void DHTMember::relink() {
    double randx;
    Packet* response;

    /* generate a random point on unit interval using protocol
     * probability density function
     */
    randx = exp(log(nEstimate)*(drand48() - 1.0));
    /* DEBUG */
    randx = 0.75; /* 0.5625 */

    /* current node asks itself if it is the manager for that point */
    response = new Packet("amITheManagerOfThisPoint?");
    response->setX(randx);

    /* to do so it calculates its segment length, it will take 0.3
     * simulated time steps
     */
    calculateSegmentLength();
    scheduleAt(simTime() + 0.3, response);
}
