package symphonydhtsim;

@license(LGPL);

simple Internet {
    parameters:        
        @display("i=device/accesspoint");
    gates:
        inout gate; // gate to DHT
}

simple DHTMember {
    parameters:               
        @display("i=block/app2_vs");
    gates:
        inout prevShortLink; // gate to previous DHTMember
        inout nextShortLink; // gate to next DHTMember
        inout longLinks[];   // long distance links
}

network SimphonyDHTNetwork {   
    parameters:
        int K = default(1);
        int dhtSize = default(16);
        @display("i=misc/cloud");
    gates:
        inout gate; // gate to Internet
    types:
        channel InternetChannel extends ned.DelayChannel {
            delay = 100ms;
        }
        
        channel MembersChannel extends ned.DelayChannel {
            delay = 100ms;
        }
    submodules:
        internet: Internet;
    	members[dhtSize]: DHTMember;
    connections:
        // Submodules connections
        gate <--> InternetChannel <--> internet.gate;
        
        // Short links
    	for i=0..sizeof(members)-1 {
        	members[i].nextShortLink <--> MembersChannel <--> members[(i+1) % sizeof(members)].prevShortLink;
        }
        
        // Long links
        for i=0..sizeof(members)-1, for j=0..K-1 {
    		members[i].longLinks++ <--> MembersChannel <--> members[(i+8+j) % sizeof(members)].longLinks++;
    	}
}
