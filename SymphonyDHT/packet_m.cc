//
// Generated file, do not edit! Created by opp_msgc 4.2 from packet.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#include <iostream>
#include <sstream>
#include "packet_m.h"

// Template rule which fires if a struct or class doesn't have operator<<
template<typename T>
std::ostream& operator<<(std::ostream& out,const T&) {return out;}

// Another default rule (prevents compiler from choosing base class' doPacking())
template<typename T>
void doPacking(cCommBuffer *, T& t) {
    throw cRuntimeError("Parsim error: no doPacking() function for type %s or its base class (check .msg and _m.cc/h files!)",opp_typename(typeid(t)));
}

template<typename T>
void doUnpacking(cCommBuffer *, T& t) {
    throw cRuntimeError("Parsim error: no doUnpacking() function for type %s or its base class (check .msg and _m.cc/h files!)",opp_typename(typeid(t)));
}




Register_Class(Packet);

Packet::Packet(const char *name, int kind) : cPacket(name,kind)
{
    this->x_var = 0;
    this->segmentLength_var = 0;
    this->nEstimate_var = 0;
    this->toSenderGateIndex_var = 0;
    routingList_arraysize = 0;
    this->routingList_var = 0;
    this->manager_var = 0;
}

Packet::Packet(const Packet& other) : cPacket(other)
{
    routingList_arraysize = 0;
    this->routingList_var = 0;
    copy(other);
}

Packet::~Packet()
{
    delete [] routingList_var;
}

Packet& Packet::operator=(const Packet& other)
{
    if (this==&other) return *this;
    cPacket::operator=(other);
    copy(other);
    return *this;
}

void Packet::copy(const Packet& other)
{
    this->x_var = other.x_var;
    this->segmentLength_var = other.segmentLength_var;
    this->nEstimate_var = other.nEstimate_var;
    this->toSenderGateIndex_var = other.toSenderGateIndex_var;
    delete [] this->routingList_var;
    this->routingList_var = (other.routingList_arraysize==0) ? NULL : new int[other.routingList_arraysize];
    routingList_arraysize = other.routingList_arraysize;
    for (unsigned int i=0; i<routingList_arraysize; i++)
        this->routingList_var[i] = other.routingList_var[i];
    this->manager_var = other.manager_var;
}

void Packet::parsimPack(cCommBuffer *b)
{
    cPacket::parsimPack(b);
    doPacking(b,this->x_var);
    doPacking(b,this->segmentLength_var);
    doPacking(b,this->nEstimate_var);
    doPacking(b,this->toSenderGateIndex_var);
    b->pack(routingList_arraysize);
    doPacking(b,this->routingList_var,routingList_arraysize);
    doPacking(b,this->manager_var);
}

void Packet::parsimUnpack(cCommBuffer *b)
{
    cPacket::parsimUnpack(b);
    doUnpacking(b,this->x_var);
    doUnpacking(b,this->segmentLength_var);
    doUnpacking(b,this->nEstimate_var);
    doUnpacking(b,this->toSenderGateIndex_var);
    delete [] this->routingList_var;
    b->unpack(routingList_arraysize);
    if (routingList_arraysize==0) {
        this->routingList_var = 0;
    } else {
        this->routingList_var = new int[routingList_arraysize];
        doUnpacking(b,this->routingList_var,routingList_arraysize);
    }
    doUnpacking(b,this->manager_var);
}

double Packet::getX() const
{
    return x_var;
}

void Packet::setX(double x)
{
    this->x_var = x;
}

double Packet::getSegmentLength() const
{
    return segmentLength_var;
}

void Packet::setSegmentLength(double segmentLength)
{
    this->segmentLength_var = segmentLength;
}

double Packet::getNEstimate() const
{
    return nEstimate_var;
}

void Packet::setNEstimate(double nEstimate)
{
    this->nEstimate_var = nEstimate;
}

int Packet::getToSenderGateIndex() const
{
    return toSenderGateIndex_var;
}

void Packet::setToSenderGateIndex(int toSenderGateIndex)
{
    this->toSenderGateIndex_var = toSenderGateIndex;
}

void Packet::setRoutingListArraySize(unsigned int size)
{
    int *routingList_var2 = (size==0) ? NULL : new int[size];
    unsigned int sz = routingList_arraysize < size ? routingList_arraysize : size;
    for (unsigned int i=0; i<sz; i++)
        routingList_var2[i] = this->routingList_var[i];
    for (unsigned int i=sz; i<size; i++)
        routingList_var2[i] = 0;
    routingList_arraysize = size;
    delete [] this->routingList_var;
    this->routingList_var = routingList_var2;
}

unsigned int Packet::getRoutingListArraySize() const
{
    return routingList_arraysize;
}

int Packet::getRoutingList(unsigned int k) const
{
    if (k>=routingList_arraysize) throw cRuntimeError("Array of size %d indexed by %d", routingList_arraysize, k);
    return routingList_var[k];
}

void Packet::setRoutingList(unsigned int k, int routingList)
{
    if (k>=routingList_arraysize) throw cRuntimeError("Array of size %d indexed by %d", routingList_arraysize, k);
    this->routingList_var[k] = routingList;
}

int Packet::getManager() const
{
    return manager_var;
}

void Packet::setManager(int manager)
{
    this->manager_var = manager;
}

class PacketDescriptor : public cClassDescriptor
{
  public:
    PacketDescriptor();
    virtual ~PacketDescriptor();

    virtual bool doesSupport(cObject *obj) const;
    virtual const char *getProperty(const char *propertyname) const;
    virtual int getFieldCount(void *object) const;
    virtual const char *getFieldName(void *object, int field) const;
    virtual int findField(void *object, const char *fieldName) const;
    virtual unsigned int getFieldTypeFlags(void *object, int field) const;
    virtual const char *getFieldTypeString(void *object, int field) const;
    virtual const char *getFieldProperty(void *object, int field, const char *propertyname) const;
    virtual int getArraySize(void *object, int field) const;

    virtual std::string getFieldAsString(void *object, int field, int i) const;
    virtual bool setFieldAsString(void *object, int field, int i, const char *value) const;

    virtual const char *getFieldStructName(void *object, int field) const;
    virtual void *getFieldStructPointer(void *object, int field, int i) const;
};

Register_ClassDescriptor(PacketDescriptor);

PacketDescriptor::PacketDescriptor() : cClassDescriptor("Packet", "cPacket")
{
}

PacketDescriptor::~PacketDescriptor()
{
}

bool PacketDescriptor::doesSupport(cObject *obj) const
{
    return dynamic_cast<Packet *>(obj)!=NULL;
}

const char *PacketDescriptor::getProperty(const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : NULL;
}

int PacketDescriptor::getFieldCount(void *object) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 6+basedesc->getFieldCount(object) : 6;
}

unsigned int PacketDescriptor::getFieldTypeFlags(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeFlags(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISARRAY | FD_ISEDITABLE,
        FD_ISEDITABLE,
    };
    return (field>=0 && field<6) ? fieldTypeFlags[field] : 0;
}

const char *PacketDescriptor::getFieldName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static const char *fieldNames[] = {
        "x",
        "segmentLength",
        "nEstimate",
        "toSenderGateIndex",
        "routingList",
        "manager",
    };
    return (field>=0 && field<6) ? fieldNames[field] : NULL;
}

int PacketDescriptor::findField(void *object, const char *fieldName) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    int base = basedesc ? basedesc->getFieldCount(object) : 0;
    if (fieldName[0]=='x' && strcmp(fieldName, "x")==0) return base+0;
    if (fieldName[0]=='s' && strcmp(fieldName, "segmentLength")==0) return base+1;
    if (fieldName[0]=='n' && strcmp(fieldName, "nEstimate")==0) return base+2;
    if (fieldName[0]=='t' && strcmp(fieldName, "toSenderGateIndex")==0) return base+3;
    if (fieldName[0]=='r' && strcmp(fieldName, "routingList")==0) return base+4;
    if (fieldName[0]=='m' && strcmp(fieldName, "manager")==0) return base+5;
    return basedesc ? basedesc->findField(object, fieldName) : -1;
}

const char *PacketDescriptor::getFieldTypeString(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeString(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static const char *fieldTypeStrings[] = {
        "double",
        "double",
        "double",
        "int",
        "int",
        "int",
    };
    return (field>=0 && field<6) ? fieldTypeStrings[field] : NULL;
}

const char *PacketDescriptor::getFieldProperty(void *object, int field, const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldProperty(object, field, propertyname);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        default: return NULL;
    }
}

int PacketDescriptor::getArraySize(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getArraySize(object, field);
        field -= basedesc->getFieldCount(object);
    }
    Packet *pp = (Packet *)object; (void)pp;
    switch (field) {
        case 4: return pp->getRoutingListArraySize();
        default: return 0;
    }
}

std::string PacketDescriptor::getFieldAsString(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldAsString(object,field,i);
        field -= basedesc->getFieldCount(object);
    }
    Packet *pp = (Packet *)object; (void)pp;
    switch (field) {
        case 0: return double2string(pp->getX());
        case 1: return double2string(pp->getSegmentLength());
        case 2: return double2string(pp->getNEstimate());
        case 3: return long2string(pp->getToSenderGateIndex());
        case 4: return long2string(pp->getRoutingList(i));
        case 5: return long2string(pp->getManager());
        default: return "";
    }
}

bool PacketDescriptor::setFieldAsString(void *object, int field, int i, const char *value) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->setFieldAsString(object,field,i,value);
        field -= basedesc->getFieldCount(object);
    }
    Packet *pp = (Packet *)object; (void)pp;
    switch (field) {
        case 0: pp->setX(string2double(value)); return true;
        case 1: pp->setSegmentLength(string2double(value)); return true;
        case 2: pp->setNEstimate(string2double(value)); return true;
        case 3: pp->setToSenderGateIndex(string2long(value)); return true;
        case 4: pp->setRoutingList(i,string2long(value)); return true;
        case 5: pp->setManager(string2long(value)); return true;
        default: return false;
    }
}

const char *PacketDescriptor::getFieldStructName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static const char *fieldStructNames[] = {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
    };
    return (field>=0 && field<6) ? fieldStructNames[field] : NULL;
}

void *PacketDescriptor::getFieldStructPointer(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructPointer(object, field, i);
        field -= basedesc->getFieldCount(object);
    }
    Packet *pp = (Packet *)object; (void)pp;
    switch (field) {
        default: return NULL;
    }
}


