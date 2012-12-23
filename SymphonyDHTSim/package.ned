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
        @display("i=device/pc3");
    gates:
        inout prevShortLink; // gate to previous DHTMember
        inout nextShortLink; // gate to next DHTMember
        inout longLinks[];   // long distance links
}

network SimphonyDHTNetwork {   
    parameters:
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
    	members[32]: DHTMember;
    connections:
        gate <--> InternetChannel <--> internet.gate;
    	for i=0..sizeof(members)-1 {
        	members[i].nextShortLink <--> MembersChannel <--> members[(i+1) % sizeof(members)].prevShortLink;
        }
}
