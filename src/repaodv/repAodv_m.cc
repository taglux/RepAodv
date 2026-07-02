//
// Generated file, do not edit! Created by opp_msgtool 6.3 from src/repaodv/repAodv.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wshadow"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wc++98-compat"
#  pragma clang diagnostic ignored "-Wunreachable-code-break"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

#include <iostream>
#include <sstream>
#include <memory>
#include <type_traits>
#include "repAodv_m.h"

namespace omnetpp {

// Template pack/unpack rules. They are declared *after* a1l type-specific pack functions for multiple reasons.
// They are in the omnetpp namespace, to allow them to be found by argument-dependent lookup via the cCommBuffer argument

// Packing/unpacking an std::vector
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::vector<T,A>& v)
{
    int n = v.size();
    doParsimPacking(buffer, n);
    for (int i = 0; i < n; i++)
        doParsimPacking(buffer, v[i]);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::vector<T,A>& v)
{
    int n;
    doParsimUnpacking(buffer, n);
    v.resize(n);
    for (int i = 0; i < n; i++)
        doParsimUnpacking(buffer, v[i]);
}

// Packing/unpacking an std::list
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::list<T,A>& l)
{
    doParsimPacking(buffer, (int)l.size());
    for (typename std::list<T,A>::const_iterator it = l.begin(); it != l.end(); ++it)
        doParsimPacking(buffer, (T&)*it);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::list<T,A>& l)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        l.push_back(T());
        doParsimUnpacking(buffer, l.back());
    }
}

// Packing/unpacking an std::set
template<typename T, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::set<T,Tr,A>& s)
{
    doParsimPacking(buffer, (int)s.size());
    for (typename std::set<T,Tr,A>::const_iterator it = s.begin(); it != s.end(); ++it)
        doParsimPacking(buffer, *it);
}

template<typename T, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::set<T,Tr,A>& s)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        T x;
        doParsimUnpacking(buffer, x);
        s.insert(x);
    }
}

// Packing/unpacking an std::map
template<typename K, typename V, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::map<K,V,Tr,A>& m)
{
    doParsimPacking(buffer, (int)m.size());
    for (typename std::map<K,V,Tr,A>::const_iterator it = m.begin(); it != m.end(); ++it) {
        doParsimPacking(buffer, it->first);
        doParsimPacking(buffer, it->second);
    }
}

template<typename K, typename V, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::map<K,V,Tr,A>& m)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        K k; V v;
        doParsimUnpacking(buffer, k);
        doParsimUnpacking(buffer, v);
        m[k] = v;
    }
}

// Default pack/unpack function for arrays
template<typename T>
void doParsimArrayPacking(omnetpp::cCommBuffer *b, const T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimPacking(b, t[i]);
}

template<typename T>
void doParsimArrayUnpacking(omnetpp::cCommBuffer *b, T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimUnpacking(b, t[i]);
}

// Default rule to prevent compiler from choosing base class' doParsimPacking() function
template<typename T>
void doParsimPacking(omnetpp::cCommBuffer *, const T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimPacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

template<typename T>
void doParsimUnpacking(omnetpp::cCommBuffer *, T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimUnpacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

}  // namespace omnetpp

namespace inet {
namespace aodv {

Register_Class(AodvForwardAck)

AodvForwardAck::AodvForwardAck() : ::inet::aodv::AodvControlPacket()
{
}

AodvForwardAck::AodvForwardAck(const AodvForwardAck& other) : ::inet::aodv::AodvControlPacket(other)
{
    copy(other);
}

AodvForwardAck::~AodvForwardAck()
{
}

AodvForwardAck& AodvForwardAck::operator=(const AodvForwardAck& other)
{
    if (this == &other) return *this;
    ::inet::aodv::AodvControlPacket::operator=(other);
    copy(other);
    return *this;
}

void AodvForwardAck::copy(const AodvForwardAck& other)
{
    this->packetId = other.packetId;
    this->originSender = other.originSender;
}

void AodvForwardAck::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::aodv::AodvControlPacket::parsimPack(b);
    doParsimPacking(b,this->packetId);
    doParsimPacking(b,this->originSender);
}

void AodvForwardAck::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::aodv::AodvControlPacket::parsimUnpack(b);
    doParsimUnpacking(b,this->packetId);
    doParsimUnpacking(b,this->originSender);
}

uint64_t AodvForwardAck::getPacketId() const
{
    return this->packetId;
}

void AodvForwardAck::setPacketId(uint64_t packetId)
{
    handleChange();
    this->packetId = packetId;
}

const ::inet::L3Address& AodvForwardAck::getOriginSender() const
{
    return this->originSender;
}

void AodvForwardAck::setOriginSender(const ::inet::L3Address& originSender)
{
    handleChange();
    this->originSender = originSender;
}

class AodvForwardAckDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_packetId,
        FIELD_originSender,
    };
  public:
    AodvForwardAckDescriptor();
    virtual ~AodvForwardAckDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyName) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyName) const override;
    virtual int getFieldArraySize(omnetpp::any_ptr object, int field) const override;
    virtual void setFieldArraySize(omnetpp::any_ptr object, int field, int size) const override;

    virtual const char *getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const override;
    virtual std::string getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const override;
    virtual omnetpp::cValue getFieldValue(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual omnetpp::any_ptr getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const override;
};

Register_ClassDescriptor(AodvForwardAckDescriptor)

AodvForwardAckDescriptor::AodvForwardAckDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(inet::aodv::AodvForwardAck)), "inet::aodv::AodvControlPacket")
{
    propertyNames = nullptr;
}

AodvForwardAckDescriptor::~AodvForwardAckDescriptor()
{
    delete[] propertyNames;
}

bool AodvForwardAckDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<AodvForwardAck *>(obj)!=nullptr;
}

const char **AodvForwardAckDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *AodvForwardAckDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int AodvForwardAckDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 2+base->getFieldCount() : 2;
}

unsigned int AodvForwardAckDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_packetId
        0,    // FIELD_originSender
    };
    return (field >= 0 && field < 2) ? fieldTypeFlags[field] : 0;
}

const char *AodvForwardAckDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "packetId",
        "originSender",
    };
    return (field >= 0 && field < 2) ? fieldNames[field] : nullptr;
}

int AodvForwardAckDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "packetId") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "originSender") == 0) return baseIndex + 1;
    return base ? base->findField(fieldName) : -1;
}

const char *AodvForwardAckDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "uint64_t",    // FIELD_packetId
        "inet::L3Address",    // FIELD_originSender
    };
    return (field >= 0 && field < 2) ? fieldTypeStrings[field] : nullptr;
}

const char **AodvForwardAckDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *AodvForwardAckDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int AodvForwardAckDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    AodvForwardAck *pp = omnetpp::fromAnyPtr<AodvForwardAck>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void AodvForwardAckDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    AodvForwardAck *pp = omnetpp::fromAnyPtr<AodvForwardAck>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'AodvForwardAck'", field);
    }
}

const char *AodvForwardAckDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    AodvForwardAck *pp = omnetpp::fromAnyPtr<AodvForwardAck>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string AodvForwardAckDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    AodvForwardAck *pp = omnetpp::fromAnyPtr<AodvForwardAck>(object); (void)pp;
    switch (field) {
        case FIELD_packetId: return uint642string(pp->getPacketId());
        case FIELD_originSender: return pp->getOriginSender().str();
        default: return "";
    }
}

void AodvForwardAckDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    AodvForwardAck *pp = omnetpp::fromAnyPtr<AodvForwardAck>(object); (void)pp;
    switch (field) {
        case FIELD_packetId: pp->setPacketId(string2uint64(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'AodvForwardAck'", field);
    }
}

omnetpp::cValue AodvForwardAckDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    AodvForwardAck *pp = omnetpp::fromAnyPtr<AodvForwardAck>(object); (void)pp;
    switch (field) {
        case FIELD_packetId: return (omnetpp::intval_t)(pp->getPacketId());
        case FIELD_originSender: return omnetpp::toAnyPtr(&pp->getOriginSender()); break;
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'AodvForwardAck' as cValue -- field index out of range?", field);
    }
}

void AodvForwardAckDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    AodvForwardAck *pp = omnetpp::fromAnyPtr<AodvForwardAck>(object); (void)pp;
    switch (field) {
        case FIELD_packetId: pp->setPacketId(omnetpp::checked_int_cast<uint64_t>(value.intValue())); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'AodvForwardAck'", field);
    }
}

const char *AodvForwardAckDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

omnetpp::any_ptr AodvForwardAckDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    AodvForwardAck *pp = omnetpp::fromAnyPtr<AodvForwardAck>(object); (void)pp;
    switch (field) {
        case FIELD_originSender: return omnetpp::toAnyPtr(&pp->getOriginSender()); break;
        default: return omnetpp::any_ptr(nullptr);
    }
}

void AodvForwardAckDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    AodvForwardAck *pp = omnetpp::fromAnyPtr<AodvForwardAck>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'AodvForwardAck'", field);
    }
}

TrustShareEntry::TrustShareEntry()
{
}

void __doPacking(omnetpp::cCommBuffer *b, const TrustShareEntry& a)
{
    doParsimPacking(b,a.nodeId);
    doParsimPacking(b,a.reputationScore);
}

void __doUnpacking(omnetpp::cCommBuffer *b, TrustShareEntry& a)
{
    doParsimUnpacking(b,a.nodeId);
    doParsimUnpacking(b,a.reputationScore);
}

class TrustShareEntryDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_nodeId,
        FIELD_reputationScore,
    };
  public:
    TrustShareEntryDescriptor();
    virtual ~TrustShareEntryDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyName) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyName) const override;
    virtual int getFieldArraySize(omnetpp::any_ptr object, int field) const override;
    virtual void setFieldArraySize(omnetpp::any_ptr object, int field, int size) const override;

    virtual const char *getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const override;
    virtual std::string getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const override;
    virtual omnetpp::cValue getFieldValue(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual omnetpp::any_ptr getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const override;
};

Register_ClassDescriptor(TrustShareEntryDescriptor)

TrustShareEntryDescriptor::TrustShareEntryDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(inet::aodv::TrustShareEntry)), "")
{
    propertyNames = nullptr;
}

TrustShareEntryDescriptor::~TrustShareEntryDescriptor()
{
    delete[] propertyNames;
}

bool TrustShareEntryDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<TrustShareEntry *>(obj)!=nullptr;
}

const char **TrustShareEntryDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *TrustShareEntryDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int TrustShareEntryDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 2+base->getFieldCount() : 2;
}

unsigned int TrustShareEntryDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        0,    // FIELD_nodeId
        FD_ISEDITABLE,    // FIELD_reputationScore
    };
    return (field >= 0 && field < 2) ? fieldTypeFlags[field] : 0;
}

const char *TrustShareEntryDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "nodeId",
        "reputationScore",
    };
    return (field >= 0 && field < 2) ? fieldNames[field] : nullptr;
}

int TrustShareEntryDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "nodeId") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "reputationScore") == 0) return baseIndex + 1;
    return base ? base->findField(fieldName) : -1;
}

const char *TrustShareEntryDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "inet::L3Address",    // FIELD_nodeId
        "double",    // FIELD_reputationScore
    };
    return (field >= 0 && field < 2) ? fieldTypeStrings[field] : nullptr;
}

const char **TrustShareEntryDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *TrustShareEntryDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int TrustShareEntryDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    TrustShareEntry *pp = omnetpp::fromAnyPtr<TrustShareEntry>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void TrustShareEntryDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    TrustShareEntry *pp = omnetpp::fromAnyPtr<TrustShareEntry>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'TrustShareEntry'", field);
    }
}

const char *TrustShareEntryDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    TrustShareEntry *pp = omnetpp::fromAnyPtr<TrustShareEntry>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string TrustShareEntryDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    TrustShareEntry *pp = omnetpp::fromAnyPtr<TrustShareEntry>(object); (void)pp;
    switch (field) {
        case FIELD_nodeId: return pp->nodeId.str();
        case FIELD_reputationScore: return double2string(pp->reputationScore);
        default: return "";
    }
}

void TrustShareEntryDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    TrustShareEntry *pp = omnetpp::fromAnyPtr<TrustShareEntry>(object); (void)pp;
    switch (field) {
        case FIELD_reputationScore: pp->reputationScore = string2double(value); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'TrustShareEntry'", field);
    }
}

omnetpp::cValue TrustShareEntryDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    TrustShareEntry *pp = omnetpp::fromAnyPtr<TrustShareEntry>(object); (void)pp;
    switch (field) {
        case FIELD_nodeId: return omnetpp::toAnyPtr(&pp->nodeId); break;
        case FIELD_reputationScore: return pp->reputationScore;
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'TrustShareEntry' as cValue -- field index out of range?", field);
    }
}

void TrustShareEntryDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    TrustShareEntry *pp = omnetpp::fromAnyPtr<TrustShareEntry>(object); (void)pp;
    switch (field) {
        case FIELD_reputationScore: pp->reputationScore = value.doubleValue(); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'TrustShareEntry'", field);
    }
}

const char *TrustShareEntryDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

omnetpp::any_ptr TrustShareEntryDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    TrustShareEntry *pp = omnetpp::fromAnyPtr<TrustShareEntry>(object); (void)pp;
    switch (field) {
        case FIELD_nodeId: return omnetpp::toAnyPtr(&pp->nodeId); break;
        default: return omnetpp::any_ptr(nullptr);
    }
}

void TrustShareEntryDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    TrustShareEntry *pp = omnetpp::fromAnyPtr<TrustShareEntry>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'TrustShareEntry'", field);
    }
}

Register_Class(AodvTrustShare)

AodvTrustShare::AodvTrustShare() : ::inet::aodv::AodvControlPacket()
{
}

AodvTrustShare::AodvTrustShare(const AodvTrustShare& other) : ::inet::aodv::AodvControlPacket(other)
{
    copy(other);
}

AodvTrustShare::~AodvTrustShare()
{
    delete [] this->entries;
}

AodvTrustShare& AodvTrustShare::operator=(const AodvTrustShare& other)
{
    if (this == &other) return *this;
    ::inet::aodv::AodvControlPacket::operator=(other);
    copy(other);
    return *this;
}

void AodvTrustShare::copy(const AodvTrustShare& other)
{
    delete [] this->entries;
    this->entries = (other.entries_arraysize==0) ? nullptr : new TrustShareEntry[other.entries_arraysize];
    entries_arraysize = other.entries_arraysize;
    for (size_t i = 0; i < entries_arraysize; i++) {
        this->entries[i] = other.entries[i];
    }
}

void AodvTrustShare::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::aodv::AodvControlPacket::parsimPack(b);
    b->pack(entries_arraysize);
    doParsimArrayPacking(b,this->entries,entries_arraysize);
}

void AodvTrustShare::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::aodv::AodvControlPacket::parsimUnpack(b);
    delete [] this->entries;
    b->unpack(entries_arraysize);
    if (entries_arraysize == 0) {
        this->entries = nullptr;
    } else {
        this->entries = new TrustShareEntry[entries_arraysize];
        doParsimArrayUnpacking(b,this->entries,entries_arraysize);
    }
}

size_t AodvTrustShare::getEntriesArraySize() const
{
    return entries_arraysize;
}

const TrustShareEntry& AodvTrustShare::getEntries(size_t k) const
{
    if (k >= entries_arraysize) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)entries_arraysize, (unsigned long)k);
    return this->entries[k];
}

void AodvTrustShare::setEntriesArraySize(size_t newSize)
{
    handleChange();
    TrustShareEntry *entries2 = (newSize==0) ? nullptr : new TrustShareEntry[newSize];
    size_t minSize = entries_arraysize < newSize ? entries_arraysize : newSize;
    for (size_t i = 0; i < minSize; i++)
        entries2[i] = this->entries[i];
    delete [] this->entries;
    this->entries = entries2;
    entries_arraysize = newSize;
}

void AodvTrustShare::setEntries(size_t k, const TrustShareEntry& entries)
{
    if (k >= entries_arraysize) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)entries_arraysize, (unsigned long)k);
    handleChange();
    this->entries[k] = entries;
}

void AodvTrustShare::insertEntries(size_t k, const TrustShareEntry& entries)
{
    if (k > entries_arraysize) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)entries_arraysize, (unsigned long)k);
    handleChange();
    size_t newSize = entries_arraysize + 1;
    TrustShareEntry *entries2 = new TrustShareEntry[newSize];
    size_t i;
    for (i = 0; i < k; i++)
        entries2[i] = this->entries[i];
    entries2[k] = entries;
    for (i = k + 1; i < newSize; i++)
        entries2[i] = this->entries[i-1];
    delete [] this->entries;
    this->entries = entries2;
    entries_arraysize = newSize;
}

void AodvTrustShare::appendEntries(const TrustShareEntry& entries)
{
    insertEntries(entries_arraysize, entries);
}

void AodvTrustShare::eraseEntries(size_t k)
{
    if (k >= entries_arraysize) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)entries_arraysize, (unsigned long)k);
    handleChange();
    size_t newSize = entries_arraysize - 1;
    TrustShareEntry *entries2 = (newSize == 0) ? nullptr : new TrustShareEntry[newSize];
    size_t i;
    for (i = 0; i < k; i++)
        entries2[i] = this->entries[i];
    for (i = k; i < newSize; i++)
        entries2[i] = this->entries[i+1];
    delete [] this->entries;
    this->entries = entries2;
    entries_arraysize = newSize;
}

class AodvTrustShareDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_entries,
    };
  public:
    AodvTrustShareDescriptor();
    virtual ~AodvTrustShareDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyName) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyName) const override;
    virtual int getFieldArraySize(omnetpp::any_ptr object, int field) const override;
    virtual void setFieldArraySize(omnetpp::any_ptr object, int field, int size) const override;

    virtual const char *getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const override;
    virtual std::string getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const override;
    virtual omnetpp::cValue getFieldValue(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual omnetpp::any_ptr getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const override;
};

Register_ClassDescriptor(AodvTrustShareDescriptor)

AodvTrustShareDescriptor::AodvTrustShareDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(inet::aodv::AodvTrustShare)), "inet::aodv::AodvControlPacket")
{
    propertyNames = nullptr;
}

AodvTrustShareDescriptor::~AodvTrustShareDescriptor()
{
    delete[] propertyNames;
}

bool AodvTrustShareDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<AodvTrustShare *>(obj)!=nullptr;
}

const char **AodvTrustShareDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *AodvTrustShareDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int AodvTrustShareDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 1+base->getFieldCount() : 1;
}

unsigned int AodvTrustShareDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISARRAY | FD_ISCOMPOUND | FD_ISRESIZABLE,    // FIELD_entries
    };
    return (field >= 0 && field < 1) ? fieldTypeFlags[field] : 0;
}

const char *AodvTrustShareDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "entries",
    };
    return (field >= 0 && field < 1) ? fieldNames[field] : nullptr;
}

int AodvTrustShareDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "entries") == 0) return baseIndex + 0;
    return base ? base->findField(fieldName) : -1;
}

const char *AodvTrustShareDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "inet::aodv::TrustShareEntry",    // FIELD_entries
    };
    return (field >= 0 && field < 1) ? fieldTypeStrings[field] : nullptr;
}

const char **AodvTrustShareDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *AodvTrustShareDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int AodvTrustShareDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    AodvTrustShare *pp = omnetpp::fromAnyPtr<AodvTrustShare>(object); (void)pp;
    switch (field) {
        case FIELD_entries: return pp->getEntriesArraySize();
        default: return 0;
    }
}

void AodvTrustShareDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    AodvTrustShare *pp = omnetpp::fromAnyPtr<AodvTrustShare>(object); (void)pp;
    switch (field) {
        case FIELD_entries: pp->setEntriesArraySize(size); break;
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'AodvTrustShare'", field);
    }
}

const char *AodvTrustShareDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    AodvTrustShare *pp = omnetpp::fromAnyPtr<AodvTrustShare>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string AodvTrustShareDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    AodvTrustShare *pp = omnetpp::fromAnyPtr<AodvTrustShare>(object); (void)pp;
    switch (field) {
        case FIELD_entries: return "";
        default: return "";
    }
}

void AodvTrustShareDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    AodvTrustShare *pp = omnetpp::fromAnyPtr<AodvTrustShare>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'AodvTrustShare'", field);
    }
}

omnetpp::cValue AodvTrustShareDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    AodvTrustShare *pp = omnetpp::fromAnyPtr<AodvTrustShare>(object); (void)pp;
    switch (field) {
        case FIELD_entries: return omnetpp::toAnyPtr(&pp->getEntries(i)); break;
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'AodvTrustShare' as cValue -- field index out of range?", field);
    }
}

void AodvTrustShareDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    AodvTrustShare *pp = omnetpp::fromAnyPtr<AodvTrustShare>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'AodvTrustShare'", field);
    }
}

const char *AodvTrustShareDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        case FIELD_entries: return omnetpp::opp_typename(typeid(TrustShareEntry));
        default: return nullptr;
    };
}

omnetpp::any_ptr AodvTrustShareDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    AodvTrustShare *pp = omnetpp::fromAnyPtr<AodvTrustShare>(object); (void)pp;
    switch (field) {
        case FIELD_entries: return omnetpp::toAnyPtr(&pp->getEntries(i)); break;
        default: return omnetpp::any_ptr(nullptr);
    }
}

void AodvTrustShareDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    AodvTrustShare *pp = omnetpp::fromAnyPtr<AodvTrustShare>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'AodvTrustShare'", field);
    }
}

}  // namespace aodv
}  // namespace inet

namespace omnetpp {

}  // namespace omnetpp

