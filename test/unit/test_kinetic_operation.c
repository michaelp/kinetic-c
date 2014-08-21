/*
* kinetic-c
* Copyright (C) 2014 Seagate Technology.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*
*/

#include "unity_helper.h"
#include "protobuf-c.h"
#include "kinetic_types.h"
#include "kinetic_operation.h"
#include "kinetic_proto.h"
#include "mock_kinetic_connection.h"
#include "mock_kinetic_message.h"
#include "mock_kinetic_pdu.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_KineticOperation_should_have_all_supported_fields(void)
{
    KineticConnection connection;
    KINETIC_CONNECTION_INIT(&connection, 12, "some_hmac_key");
    KineticPDU request, response;
    const ByteArray key = BYTE_ARRAY_INIT_FROM_CSTRING("SomeKey0123");
    const ByteArray tag = BYTE_ARRAY_INIT_FROM_CSTRING("some_tag_01");
    const ByteArray version = BYTE_ARRAY_INIT_FROM_CSTRING("v1.0");
    const ByteArray newVersion = BYTE_ARRAY_INIT_FROM_CSTRING("v2.0");
    const uint8_t dataVals[] = {1,2,3,4,5,6,7,8};
    const ByteArray value = BYTE_ARRAY_INIT(dataVals);

    KineticOperation op = {
        .connection = &connection,
        .request = &request,
        .response = &response,
        .value = value,
    };

    TEST_ASSERT_EQUAL_PTR(&connection, op.connection);
    TEST_ASSERT_EQUAL_PTR(&request, op.request);
    TEST_ASSERT_EQUAL_PTR(&response, op.response);
    TEST_ASSERT_EQUAL_BYTE_ARRAY(value, op.value);
}

void test_KineticOperation_BuildNoop_should_build_and_execute_a_NOOP_operation(void)
{
    KineticConnection connection;
    KINETIC_CONNECTION_INIT(&connection, 12, "some_hmac_key");
    KineticMessage requestMsg, responseMsg;
    KineticPDU request, response;
    KINETIC_MESSAGE_INIT(&requestMsg);
    KINETIC_MESSAGE_INIT(&responseMsg);
    request.message = &requestMsg;
    request.proto = NULL;
    response.message = NULL;
    response.proto = NULL;
    KineticOperation op = {
        .connection = &connection,
        .request = &request,
        .response = &response
    };

    KineticConnection_IncrementSequence_Expect(&connection);

    KineticOperation_BuildNoop(&op);

    // NOOP
    // The NOOP operation can be used as a quick test of whether the Kinetic
    // Device is running and available. If the Kinetic Device is running,
    // this operation will always succeed.
    //
    // Request Message:
    //
    // command {
    //   header {
    //     clusterVersion: ...
    //     identity: ...
    //     connectionID: ...
    //     sequence: ...
    //     messageType: NOOP
    //   }
    // }
    // hmac: "..."
    //
    TEST_ASSERT_TRUE(requestMsg.header.has_messagetype);
    TEST_ASSERT_EQUAL(KINETIC_PROTO_MESSAGE_TYPE_NOOP, requestMsg.header.messagetype);
}

void test_KineticOperation_BuildPut_should_build_and_execute_a_PUT_operation_to_create_a_new_object(void)
{
    KineticConnection connection;
    KineticMessage requestMsg, responseMsg;
    KineticPDU request, response;

    KINETIC_MESSAGE_INIT(&requestMsg);
    KINETIC_MESSAGE_INIT(&responseMsg);
    request.message = &requestMsg;
    request.proto = NULL;
    response.message = NULL;
    response.proto = NULL;
    const uint8_t valueData[] = {1,2,3,4,5,6,7,8};
    const ByteArray value = BYTE_ARRAY_INIT(valueData);
    KineticOperation op = {
        .connection = &connection,
        .request = &request,
        .response = &response,
        .value = value,
    };
    const ByteArray key = BYTE_ARRAY_INIT_FROM_CSTRING("foobar");
    const ByteArray newVersion = BYTE_ARRAY_INIT_FROM_CSTRING("v1.0");
    const ByteArray tag = BYTE_ARRAY_INIT_FROM_CSTRING("some_tag");

    KineticConnection_IncrementSequence_Expect(&connection);

    // PUT
    // The PUT operation sets the value and metadata for a given key. If a value
    // already exists in the store for the given key, the client must pass a
    // value for dbVersion which matches the stored version for this key to
    // overwrite the value metadata. This behavior can be overridden (so that
    // the version is ignored and the value and metadata are always written) by
    // setting forced to true in the KeyValue option.
    //
    // Request Message:
    //
    // command {
    //   // See top level cross cutting concerns for header details
    //   header {
    //     clusterVersion: ...
    //     identity: ...
    //     connectionID: ...
    //     sequence: ...
    //     messageType: PUT
    //   }
    //   body: {
    //     keyValue {
    //       // Required bytes
    //       // The key for the value being set
    //       key: "..."
    //
    //       // Required bytes
    //       // Versions are set on objects to support optimistic locking.
    //       // For operations that modify data, if the dbVersion sent in the
    //       // request message does not match the version stored in the db, the
    //       // request will fail.
    //       dbVersion: "..."
    //
    //       // Required bytes
    //       // Specifies what the next version of the data will be if this
    //       // operation is successful.
    //       newVersion: "..."
    //
    //       // Optional bool, default false
    //       // Setting force to true ignores potential version mismatches
    //       // and carries out the operation.
    //       force: true
    //
    //       // Optional bytes
    //       // The integrity value for the data. This value should be computed
    //       // by the client application by applying the hash algorithm
    //       // specified below to the value (and only to the value).
    //       // The algorithm used should be specified in the algorithm field.
    //       // The Kinetic Device will not do any processing on this value.
    //       tag: "..."
    //
    //       // Optional enum
    //       // The algorithm used by the client to compute the tag.
    //       // The allowed values are: SHA1, SHA2, SHA3, CRC32, CRC64
    //       algorithm: ...
    //
    //       // Optional Synchronization enum value, defaults to WRITETHROUGH
    //       // Allows client to specify if the data must be written to disk
    //       // immediately, or can be written in the future.
    //       //
    //       // WRITETHROUGH:  This request is made persistent before returning.
    //       //                This does not effect any other pending operations.
    //       // WRITEBACK:     They can be made persistent when the drive chooses,
    //       //            or when a subsequent FLUSH is give to the drive.
    //       // FLUSH:     All pending information that has not been written is
    //       //        pushed to the disk and the command that specifies
    //       //        FLUSH is written last and then returned. All WRITEBACK writes
    //       //        that have received ending status will be guaranteed to be
    //       //        written before the FLUSH operation is returned completed.
    //       synchronization: ...
    //     }
    KineticMessage_ConfigureKeyValue_Expect(op.request->message, key, newVersion, BYTE_ARRAY_NONE, tag, false);
    //   }
    // }
    // hmac: "..."
    //

    // Build the operation
    KineticOperation_BuildPut(&op, key, newVersion, BYTE_ARRAY_NONE, tag, value);

    // Ensure proper message type
    TEST_ASSERT_TRUE(requestMsg.header.has_messagetype);
    TEST_ASSERT_EQUAL(KINETIC_PROTO_MESSAGE_TYPE_PUT, requestMsg.header.messagetype);

    // Ensure value payload populated approapriately
    ByteArray requestValue = {.len = request.valueLength, .data = request.value};
    TEST_ASSERT_EQUAL_BYTE_ARRAY(value, requestValue);
}


void test_KineticOperation_BuildGet_should_build_a_GET_operation(void)
{
    KineticConnection connection;
    KineticMessage requestMsg, responseMsg;
    KineticPDU request, response;
    const uint8_t valueData[] = {1,2,3,4,5,6,7,8};
    const ByteArray value = BYTE_ARRAY_INIT(valueData);
    const ByteArray key = BYTE_ARRAY_INIT_FROM_CSTRING("foobar");

    KINETIC_MESSAGE_INIT(&requestMsg);
    KINETIC_MESSAGE_INIT(&responseMsg);
    request.message = &requestMsg;
    request.proto = NULL;
    response.message = NULL;
    response.proto = NULL;
    KineticOperation op = {
        .connection = &connection,
        .request = &request,
        .response = &response,
        .value = value,
    };

    KineticConnection_IncrementSequence_Expect(&connection);
    KineticMessage_ConfigureKeyValue_Expect(op.request->message, key, BYTE_ARRAY_NONE, BYTE_ARRAY_NONE, BYTE_ARRAY_NONE, false);

    KineticOperation_BuildGet(&op, key, value, false);

    // GET
    // The GET operation is used to retrieve the value and metadata for a given key.
    //
    // Request Message:
    // command {
    //   header {
    //     // See above for descriptions of these fields
    //     clusterVersion: ...
    //     identity: ...
    //     connectionID: ...
    //     sequence: ...
    //
    //     // The mesageType should be GET
    //     messageType: GET
    TEST_ASSERT_TRUE(requestMsg.header.has_messagetype);
    TEST_ASSERT_EQUAL(KINETIC_PROTO_MESSAGE_TYPE_GET, requestMsg.header.messagetype);
    //   }
    //   body {
    //     keyValue {
    //       // See above
    //       key: "..."
    //     }
    //   }
    // }
    // // See above
    // hmac: "..."

    TEST_ASSERT_EQUAL_PTR(NULL, request.value);
    TEST_ASSERT_EQUAL_INT64(0, request.valueLength);
}

void test_KineticOperation_BuildGet_should_build_a_GET_operation_requesting_metadata_only(void)
{
    KineticConnection connection;
    KineticMessage requestMsg, responseMsg;
    KineticPDU request, response;
    const uint8_t valueData[] = {1,2,3,4,5,6,7,8};
    const ByteArray value = BYTE_ARRAY_INIT(valueData);
    const ByteArray key = BYTE_ARRAY_INIT_FROM_CSTRING("foobar");

    KINETIC_MESSAGE_INIT(&requestMsg);
    KINETIC_MESSAGE_INIT(&responseMsg);
    request.message = &requestMsg;
    request.proto = NULL;
    response.message = NULL;
    response.proto = NULL;
    KineticOperation op = {
        .connection = &connection,
        .request = &request,
        .response = &response,
        .value = value,
    };

    KineticConnection_IncrementSequence_Expect(&connection);
    KineticMessage_ConfigureKeyValue_Expect(op.request->message, key, BYTE_ARRAY_NONE, BYTE_ARRAY_NONE, BYTE_ARRAY_NONE, true);

    KineticOperation_BuildGet(&op, key, value, true);

    // GET
    // The GET operation is used to retrieve the value and metadata for a given key.
    //
    // Request Message:
    // command {
    //   header {
    //     // See above for descriptions of these fields
    //     clusterVersion: ...
    //     identity: ...
    //     connectionID: ...
    //     sequence: ...
    //
    //     // The mesageType should be GET
    //     messageType: GET
    TEST_ASSERT_TRUE(requestMsg.header.has_messagetype);
    TEST_ASSERT_EQUAL(KINETIC_PROTO_MESSAGE_TYPE_GET, requestMsg.header.messagetype);
    //   }
    //   body {
    //     keyValue {
    //       // See above
    //       key: "..."
    //     }
    //   }
    // }
    // // See above
    // hmac: "..."

    TEST_ASSERT_EQUAL_PTR(NULL, request.value);
    TEST_ASSERT_EQUAL_INT64(0, request.valueLength);
}
