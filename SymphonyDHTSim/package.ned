package symphonydhtsim;

@license(LGPL);

simple Internet {
    parameters:        
        @display("i=block/app2");
    gates:
        inout gate; // gate to DHT
}

simple DHTMember {
    parameters:        
        @display("i=block/app2");
    gates:
        inout prevShortLink; // gate to previous DHTMember
        inout nextShortLink; // gate to next DHTMember
        inout longLinks[];   // long distance links
}

network SimphonyDHTNetwork {   
    parameters:
        @display("i=block/app2");
    gates:
        inout gate; // gate to Internet
    types:
        channel MembersChannel extends ned.DelayChannel {
            delay = 100ms;
        }
    submodules:
        internet: Internet;
    	members[32]: DHTMember;
    connections:
        gate <--> MembersChannel <--> internet.gate;
    	for i=0..sizeof(members)-1 {
        	members[i].prevShortLink <--> MembersChannel <--> members[(i == 0) ? (sizeof(members) - 1) : (i-1)].nextShortLink;
        }
}
