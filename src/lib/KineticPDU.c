#include "KineticPDU.h"
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>

static void KineticPDU_PackInt64(uint8_t* const buffer, int64_t value);
static int64_t KineticPDU_UnpackInt64(const uint8_t* const buffer);

void KineticPDU_Create(
    KineticPDU* const pdu,
    uint8_t* const buffer,
    const KineticProto* const proto,
    const uint8_t* const value,
    int64_t valueLength)
{
    size_t protoPackedSize;

    // Initialize the instance struct
    assert(pdu != NULL);
    memset(pdu, 0, sizeof(KineticPDU));

    // Assign the data buffer pointer
    assert(buffer != NULL);
    pdu->data = buffer;

    // Setup the PDU header fields
    pdu->prefix = &buffer[0];
    *pdu->prefix = (uint8_t)'F';
    pdu->protoLength = &buffer[1];
    assert(proto != NULL);
    protoPackedSize = KineticProto_get_packed_size(proto);
    KineticPDU_PackInt64(pdu->protoLength, protoPackedSize);
    pdu->valueLength = &buffer[1 + sizeof(int64_t)];
    KineticPDU_PackInt64(pdu->valueLength, valueLength);

    // Populate with the encoded kinetic protocol buffer
    pdu->proto = &buffer[PDU_HEADER_LEN];
    assert(proto != NULL);
    KineticProto_pack(proto, pdu->proto);

    // Populate with the value data, if specified
    if (value != NULL)
    {
        pdu->value = &buffer[PDU_HEADER_LEN + protoPackedSize];
        memcpy(pdu->value, value, valueLength);
    }
}

void KineticPDU_PackInt64(uint8_t* const buffer, int64_t value)
{
    int i;
    for (i = sizeof(int64_t) - 1; i >= 0; i--)
    {
        buffer[i] = value & 0xFF;
        value = value >> 8;
    }
}

int64_t KineticPDU_UnpackInt64(const uint8_t* const buffer)
{
    int i;
    int64_t value = 0;
    for (i = 0; i < sizeof(int64_t); i++)
    {
        value <<= 8;
        value += buffer[i];
    }
    return value;
}
//