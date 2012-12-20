@license(LGPL);

simple Txc10
{
    parameters:
        @display("i=block/routing");
    gates:
        input in[];
        output out[];
}

network Tictoc10
{
    submodules:
        tic[32]: Txc10;
    connections:
        for i=0..sizeof(tic)-2
        {
        	tic[i].out++ --> {  delay = 100ms; } --> tic[i+1].in++;
        	tic[i].in++ <-- {  delay = 100ms; } <-- tic[i+1].out++;
        
        }
        tic[sizeof(tic)-1].out++ --> {  delay = 100ms; } --> tic[0].in++;
        tic[sizeof(tic)-1].in++ <-- {  delay = 100ms; } <-- tic[0].out++;
        
}
