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
        int K = default(4);      
        @display("i=device/pc3");
    gates:
        inout prevShortLink; // gate to previous DHTMember
        inout nextShortLink; // gate to next DHTMember
        inout longLinks[];   // long distance links
}

network SimphonyDHTNetwork {   
    parameters:
        int dhtSize = default(32);
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
        gate <--> InternetChannel <--> internet.gate;
    	for i=0..sizeof(members)-1 {
        	members[i].nextShortLink <--> MembersChannel <--> members[(i+1) % sizeof(members)].prevShortLink;
        }
}
