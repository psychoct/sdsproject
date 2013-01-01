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
        int longLinksCreated;

    protected:
        virtual void initialize();
        virtual void handleMessage(cMessage* msg);

        /* utility methods */
        virtual double getSegmentLengthByPreviousNodesIntervalPosition(double previousNodeX);
        virtual int getGateToModule(cModule* module);
        virtual int getNeighboursNumber();
        virtual int getReverseGateIndexByGateIndex(int index);
        virtual bool hasAvailableConnections();
        virtual bool alreadyConnected(DHTMember* member);
        virtual bool amIManagerForPoint(double p);
        virtual void broadcast(Packet* msg);
        virtual void createLongLinkToMember(int index);
        virtual void createLongLinkByFirstUnconnectedGate(DHTMember* member);
        virtual void createLongLinkDisconnectingLastConnectedGate(DHTMember* member);
        virtual void disconnectGate(cGate* toDisconnect);
        virtual void dropAllLongLinks();
        virtual cGate* getLastConnectedGate(const char* gateRef);
        virtual cGate* getFirstUnconnectedGate(const char* gateRef);

        /* symphony DHT protocol methods */
        virtual void calculateSegmentLength();
        virtual void calculateNEstimate();
        virtual void relink();
        virtual void join(simtime_t delay);
        virtual void leave(simtime_t delay);
};

Define_Module(DHTMember);

/* ===========================================
 * |             omnet++ methods             |
 * ===========================================
 */

void DHTMember::initialize() {
    double connected = (double)getAncestorPar("connected");
    simtime_t delay = exponential(10);

    x = getIndex() / connected;
    segmentLength = 0;
    nEstimate = (int)connected;
    nEstimateAtLinking = (int)connected;

    neighboursTotalSegmentsLengths = 0;
    receivedSegments = 0;

    bestDistanceFoundSoFar = 42; /* note: every number >= than 1 could be considered +infinity */
    repliesToFindShortestPath = 0;
    longLinksCreated = 0;

    WATCH(x);
    WATCH(segmentLength);
    WATCH(nEstimate);
    WATCH(nEstimateAtLinking);

    if (getIndex() >= connected) {
        join(delay);
    }

    /* DEBUG */
    if (getIndex() == 0) {
        /*
        EV << "DHTMember: " << this->getFullName() << " starts procedure to calculate its segment length." << endl;
        calculateSegmentLength();

        EV << "DHTMember: " << this->getFullName() << " starts procedure to estimate the number of nodes in the DHT." << endl;
        calculateNEstimate();

        EV << "DHTMember: " << this->getFullName() << " starts relinking procedure." << endl;
        relink();
        */
    }
}

void DHTMember::handleMessage(cMessage* msg) {
    Packet* request = check_and_cast<Packet*>(msg);
    Packet* response;
    cGate* toSender;
    int toSenderGateIndex;

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
            } else {
                /* if current node is the manager for the randomly generated
                 * point it ignores this connection
                 */
                int K = (int)par("K");
                longLinksCreated++;
                EV << "DHTMember: node " << this->getFullName() << " created " << longLinksCreated << "/" << K << " long links." << endl;
                if (longLinksCreated < K) {
                    relink();
                } else {
                    longLinksCreated = 0;
                }
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
        EV << "DHTMember: node " << request->getSenderModule()->getFullName() << " asked to node " << this->getFullName() << " its interval position that is " << x << ", in order to find the shortest path through the manager of a randomly generated point. Sending it back." << endl;

        response = request->dup();
        response->setName("thisIsMyIntervalPositionToFindShortestPath");
        response->setX(x);
        send(response, toSender);
    } else if (request->isName("thisIsMyIntervalPositionToFindShortestPath")) {
        /* a neighbour sent its interval position in order to find shortest path to the manager
         * of a randomly generated point
         */
        EV << "DHTMember: node " << this->getFullName() << " received the interval position of a neighbour in order to calculate shortest path through manager of randomly generated node." << endl;

        int routinglistSize;
        double distanceDeltaToPoint;
        int reverseGateIndex;

        /* if that neighbour is the closest one (found so far) to the manager of that point
         * current node remembers the gate that connects itself to it
         */
        distanceDeltaToPoint = fabs(randomPoint - request->getX());

        EV << "DHTMember: distance of point " << randomPoint << " from that neighbour is " << distanceDeltaToPoint << "." << endl;

        if (distanceDeltaToPoint < bestDistanceFoundSoFar) {
            EV << "DHTMember: that is better than best distance found so far, that is " << bestDistanceFoundSoFar << "." << endl;
            bestDistanceFoundSoFar = distanceDeltaToPoint;
            gateIndexToClosestNode = toSenderGateIndex;
        }

        repliesToFindShortestPath++;

        EV << "DHTMember: node " << this->getFullName() << " received " << repliesToFindShortestPath << "/" << getNeighboursNumber() << " replies." << endl;

        /* when all neighbours have been considered */
        if (repliesToFindShortestPath >= getNeighboursNumber()) {
            /* current node asks to the node on the shortest path if it is the manager
             * for the randomly generated point it was looking for
             */
            EV << "DHTMember: node " << this->getFullName() << " received all the messages from its neighbours, it is now able to find the shortest path to the manager of randomly generated point." << endl;

            routinglistSize = request->getRoutingListArraySize();
            reverseGateIndex = getReverseGateIndexByGateIndex(gateIndexToClosestNode);

            response = request->dup();
            response->setName("areYouTheManagerOfThisPoint?");
            response->setX(randomPoint);
            response->setRoutingListArraySize(routinglistSize + 1);
            response->setRoutingList(routinglistSize, reverseGateIndex);
            send(response, "gate$o", gateIndexToClosestNode);

            /* reset protocol variables for next call of relink */
            bestDistanceFoundSoFar = 42;
            gateIndexToClosestNode = 0;
            repliesToFindShortestPath = 0;
        }
    } else if (request->isName("areYouTheManagerOfThisPoint?")) {
        /* current node calculate its segment length in order to decide, in 0.3 simulated
         * time steps, if it is the manager for randomly generated point
         */
        EV << "DHTMember: node " << this->getFullName() << " asks itself if it is the manager of a randomly generated point, to do so it calculates its segment length. This will take 0.3 simulated time steps." << endl;
        response = request->dup();
        response->setName("amITheManagerOfThisPoint?");
        response->setX(request->getX());
        calculateSegmentLength();
        scheduleAt(simTime() + 0.3, response);
    } else if (request->isName("managerIndexIs")) {
        /* manager for randomly generated point has been found, then it is routed back to
         * the node who made the request. If there are no remaining nodes to do the routing
         * it means current node is the one who made the request
         */
        EV << "DHTMember: node " << this->getFullName() << " knows that the manager of randomly generated point is " << request->getManager() << ". Routing this information to the node that asked it." << endl;

        int K;
        int previousRequestingNodeGateIndex;
        int routinglistSize;

        K = (int)par("K");
        routinglistSize = request->getRoutingListArraySize();
        if (routinglistSize > 0) {
            EV << "DHTMember: node " << this->getFullName() << " is not the requesting node so it passes the message through a chain to requesting node." << endl;
            previousRequestingNodeGateIndex = request->getRoutingList(routinglistSize - 1);
            response = request->dup();
            response->setName("managerIndexIs");
            response->setRoutingListArraySize(routinglistSize - 1);
            send(response, gate("gate$o", previousRequestingNodeGateIndex));
        } else {
            EV << "DHTMember: node " << this->getFullName() << " is the requesting node for the manager of randomly generated point so a long link with that manager, that is " << request->getManager() << ", is estabilished." << endl;
            createLongLinkToMember(request->getManager());

            longLinksCreated++;
            EV << "DHTMember: node " << this->getFullName() << " created " << longLinksCreated << "/" << K << " long links." << endl;
            if (longLinksCreated < K) {
                relink();
            } else {
                longLinksCreated = 0;
            }
        }
    } else if (request->isName("joinNetwork")) {
        /* TODO */
        leave(exponential(10));
    } else if (request->isName("leaveNetwork")) {
        /* TODO */
        join(exponential(10));
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
 *      [0      B--x--(A)    1[
 *
 * case 2:
 *      [0--x--(A)     B-----1[
 * or
 *      [0-----(A)     B--x--1[
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

/* returns the index of the output gate of the module connected to current
 * module through the output gate which index is taken in input
 */
int DHTMember::getReverseGateIndexByGateIndex(int index) {
    cGate* fromGate;
    cModule* nextGateModule;
    DHTMember* nextGateDHTModule;

    fromGate = gate("gate$o", index);
    nextGateModule = fromGate->getNextGate()->getOwnerModule();
    nextGateDHTModule = (DHTMember*)nextGateModule;

    return nextGateDHTModule->getGateToModule(this);
}

/* returns true if current node has got any unconnected gate */
bool DHTMember::hasAvailableConnections() {
    int i;
    for (i=0; i<gateSize("gate$o"); i++) {
        if (hasGate("gate$o", i) && !gate("gate$o", i)->isConnected()) {
            return true;
        }
    }
    return false;
}

/* returns true if current node is already connected to DHTMember member */
bool DHTMember::alreadyConnected(DHTMember* member) {
    int i;
    cGate* neighbourGate;
    for (i=0; i<gateSize("gate$o"); i++) {
        if (hasGate("gate$o", i)) {
            neighbourGate = gate("gate$o", i);
            if (neighbourGate->isConnected() && neighbourGate->getNextGate()->getOwnerModule() == member) {
                return true;
            }
        }
    }
    return false;
}

/* returns last connected gate for current node over gate vector gateRef,
 * this method returns NULL if all gates of that gate vector are unconnected
 */
cGate* DHTMember::getLastConnectedGate(const char* gateRef) {
    int i;
    for (i=gateSize(gateRef)-1; i>2; i--) {
        if (hasGate(gateRef, i) && gate(gateRef, i)->isConnected()) {
            return gate(gateRef, i);
        }
    }
    return NULL;
}

/* returns first unconnected gate for current node over gate vector gateRef,
 * this method returns NULL if all gates of that gate vector are connected
 */
cGate* DHTMember::getFirstUnconnectedGate(const char* gateRef) {
    int i;
    cGate* neighbourGate;
    for (i=0; i<gateSize(gateRef); i++) {
        if (hasGate(gateRef, i)) {
            neighbourGate = gate(gateRef, i);
            if (!neighbourGate->isConnected()) {
                return neighbourGate;
            }
        }
    }
    return NULL;
}

/* this method creates a link from current node to the node which member is
 * taken in input using the first unconnected gate for current node */
void DHTMember::createLongLinkByFirstUnconnectedGate(DHTMember* member) {
    cGate* currentFirstUnconnectedGateIn;
    cGate* currentFirstUnconnectedGateOut;
    cGate* memberFirstUnconnectedGateIn;
    cGate* memberFirstUnconnectedGateOut;

    currentFirstUnconnectedGateIn  = getFirstUnconnectedGate("gate$i");
    currentFirstUnconnectedGateOut = getFirstUnconnectedGate("gate$o");

    memberFirstUnconnectedGateIn  = member->getFirstUnconnectedGate("gate$i");
    memberFirstUnconnectedGateOut = member->getFirstUnconnectedGate("gate$o");

    /* connect current node to member */
    currentFirstUnconnectedGateOut->connectTo(memberFirstUnconnectedGateIn);
    memberFirstUnconnectedGateOut->connectTo(currentFirstUnconnectedGateIn);
}

void DHTMember::disconnectGate(cGate* toDisconnect) {
    int neighbourGateIndexToCurrentNode;
    cGate* neighbourGateToCurrentNode;

    neighbourGateIndexToCurrentNode = getReverseGateIndexByGateIndex(toDisconnect->getIndex());
    neighbourGateToCurrentNode = toDisconnect->getNextGate()->getOwnerModule()->gate("gate$o", neighbourGateIndexToCurrentNode);

    /* disconnect last long link for current node */
    neighbourGateToCurrentNode->disconnect();
    toDisconnect->disconnect();
}

void DHTMember::dropAllLongLinks() {
    int i;
    for (i=2; i<gateSize("gate$o"); i++) {
        if (hasGate("gate$o", i) && gate("gate$o", i)->isConnected()) {
            disconnectGate(gate("gate$o", i));
        }
    }
}

/* this method disconnects the last connected gate for current node and
 * connects it to the node which member is taken in input
 */
void DHTMember::createLongLinkDisconnectingLastConnectedGate(DHTMember* member) {
    cGate* lastConnectedGate;
    cGate* memberFirstUnconnectedGateIn;
    cGate* memberFirstUnconnectedGateOut;

    /* disconnect last long link for current node */
    lastConnectedGate = getLastConnectedGate("gate$o");
    disconnectGate(lastConnectedGate);

    /* connect current node to member */
    memberFirstUnconnectedGateIn  = member->getFirstUnconnectedGate("gate$i");
    memberFirstUnconnectedGateOut = member->getFirstUnconnectedGate("gate$o");

    lastConnectedGate->connectTo(memberFirstUnconnectedGateIn);
    memberFirstUnconnectedGateOut->connectTo(gate("gate$i", lastConnectedGate->getIndex()));
}

/* if necessary creates a long link to node which index is taken in input,
 * a link is not created if another link to that node yet exists, that node
 * is current node itself or that node has not any connection left
 */
void DHTMember::createLongLinkToMember(int index) {
    int K;
    DHTMember* manager;

    K = (int)par("K");
    manager = (DHTMember*)(getParentModule()->getSubmodule("members", index));

    EV << "DHTMember: node " << this->getFullName() << " checks for manager identity." << endl;

    /* if manager has got at least one unconnected gate */
    if (manager->hasAvailableConnections() && !alreadyConnected(manager) && manager != this) {
        EV << "DHTMember: manager has got available connections, is not already connected to current node and is not the current node." << endl;

        if (getNeighboursNumber() < 2 + K) {
            EV << "DHTMember: node " << this->getFullName() << " has got less than " << (2 + K) << " connections, then it connects through the first unconnected gate it has got." << endl;
            /* if current node has got at least one unconnected gate
             * then connect that gate to manager's first unconnected gate
             */
            createLongLinkByFirstUnconnectedGate(manager);
        } else {
            EV << "DHTMember: node " << this->getFullName() << " has got more than " << (2 + K) << " connections, then it drops one long distance connection and relinks to manager." << endl;
            /* otherwise, current node has not got any unconnected gate,
             * disconnect its last connected gate and use that gate to connect
             * itself to manager
             */
            createLongLinkDisconnectingLastConnectedGate(manager);
        }
    } else {
        EV << "DHTMember: manager has not got available connections, is already connected to current node or it is the current node itself. Thus the long link connection is NOT estabilished." << endl;
    }
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

    /* drop all long links to execute relink protocol
     * for the first time
     */
    if (longLinksCreated == 0)
        dropAllLongLinks();

    /* generate a random point on unit interval using protocol
     * probability density function
     */
    randx = exp(log(nEstimate)*(drand48() - 1.0));

    /* current node asks itself if it is the manager for that point */
    response = new Packet("amITheManagerOfThisPoint?");
    response->setX(randx);

    /* to do so it calculates its segment length, it will take 0.3
     * simulated time steps
     */
    calculateSegmentLength();
    scheduleAt(simTime() + 0.3, response);
}

void DHTMember::join(simtime_t delay) {
    Packet* joinPacket = new Packet("joinNetwork");
    scheduleAt(simTime() + delay, joinPacket);
}

void DHTMember::leave(simtime_t delay) {
    Packet* leavePacket = new Packet("leaveNetwork");
    scheduleAt(simTime() + delay, leavePacket);
}
