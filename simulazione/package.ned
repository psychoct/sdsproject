package simulazione;

@license(LGPL);

//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 2003-2008 Andras Varga
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//


simple Txc10
{
    parameters:        
        @display("i=block/routing");
    gates:
        inout gate[];
}

network Tictoc10
{
    types:
        channel Channel extends ned.DelayChannel {
            delay = 100ms;
        }
    submodules:
        tic[32]: Txc10;
    connections:
        for i=0..sizeof(tic)-1 {
    		tic[i].gate++ <--> Channel <--> tic[(i+1)%sizeof(tic)].gate++;
        }
}