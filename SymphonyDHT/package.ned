package simphonydht;

@license(LGPL);

simple DHTMember {
    parameters:
        int K = default(3);
        @display("i=block/app2_vs");
    gates:        
        // first and second gate are short links others are long links, 
        // for each node at most 2K + 2 links are allowed
        inout gate[2 + (2*K)] @loose;
}

network SimphonyDHTNetwork {   
    parameters:        
        int DHTSize = default(32); // DHTSize >= 16
        int connected = default(8); // 8 <= connected <= DHTSize
        @display("i=misc/cloud");
    types:
        channel MembersChannel extends ned.DelayChannel {
            delay = 100ms;
        }
    submodules:
    	members[DHTSize]: DHTMember;
    connections:        
        // short links
        // second (output) gate of each node point to first (input) gate of next node
    	for i=0..connected-1 {
        	members[i].gate[1] <--> MembersChannel <--> members[(i+1) % connected].gate[0];
        }
        
		// long links
		// K other links are created with nodes at a maximum distance of (node index + K)
        for i=0..connected-1, for j=0..members[i].K-1, if i+j+8 < connected {
    		members[i].gate++ <--> MembersChannel <--> members[(i+j+8) % connected].gate++;
    	}
}
