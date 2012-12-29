package simphonydht;

@license(LGPL);

simple Internet {
    parameters:        
        @display("i=device/accesspoint");
    gates:
        // gate to DHT
        inout gate; 
}

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
        int DHTSize = default(16);
        @display("i=misc/cloud");
    gates:
        // gate to internet
        inout gate; 
    types:
        channel InternetChannel extends ned.DelayChannel {
            delay = 100ms;
        }
        
        channel MembersChannel extends ned.DelayChannel {
            delay = 100ms;
        }
    submodules:
        internet: Internet;
    	members[DHTSize]: DHTMember;
    connections:
        // submodules connections
        gate <--> InternetChannel <--> internet.gate;
        
        // short links
        // second (output) gate of each node point to first (input) gate of next node
    	for i=0..DHTSize-1 {
        	members[i].gate[1] <--> MembersChannel <--> members[(i+1) % DHTSize].gate[0];
        }
        
		// long links
		// K other links are created with nodes at a maximum distance of (node index + K)
        for i=0..DHTSize-1, for j=0..members[i].K-1, if i+j+8 < DHTSize {
    		members[i].gate++ <--> MembersChannel <--> members[(i+j+8) % DHTSize].gate++;
    	}
}
