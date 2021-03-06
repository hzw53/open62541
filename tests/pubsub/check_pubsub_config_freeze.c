/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <open62541/types_generated_encoding_binary.h>

#include "ua_server_internal.h"

#include <check.h>
#include <stdio.h>
#include <time.h>

UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    config->pubsubTransportLayers = (UA_PubSubTransportLayer*)
            UA_malloc(sizeof(UA_PubSubTransportLayer));
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;

    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(CreateAndLockConfiguration) {
    //create config
    UA_NodeId connection1, writerGroup1, publishedDataSet1, dataSetWriter1, dataSetField1;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
    &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_Server_addPubSubConnection(server, &connectionConfig, &connection1);

    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
    writerGroupConfig.name = UA_STRING("WriterGroup 1");
    writerGroupConfig.publishingInterval = 10;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &writerGroup1);

    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("PublishedDataSet 1");
    UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet1);

    UA_DataSetFieldConfig fieldConfig;
    memset(&fieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    fieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    fieldConfig.field.variable.fieldNameAlias = UA_STRING("field 1");
    UA_Server_addDataSetField(server, publishedDataSet1, &fieldConfig, &dataSetField1);

    UA_DataSetField *dataSetField = UA_DataSetField_findDSFbyId(server, dataSetField1);
    ck_assert(dataSetField->config.configurationFrozen == UA_FALSE);

    //get internal WG Pointer
    UA_WriterGroup *writerGroup = UA_WriterGroup_findWGbyId(server, writerGroup1);
    ck_assert(writerGroup->state == UA_PUBSUBSTATE_DISABLED);

    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("DataSetWriter 1");
    UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter1);
    UA_DataSetWriter *dataSetWriter = UA_DataSetWriter_findDSWbyId(server, dataSetWriter1);

    //get internal PubSubConnection Pointer
    UA_PubSubConnection *pubSubConnection = UA_PubSubConnection_findConnectionbyId(server, connection1);

    ck_assert(dataSetWriter->config.configurationFrozen == UA_FALSE);
    //Lock the writer group and the child pubsub entities
        UA_Server_freezeWriterGroupConfiguration(server, writerGroup1);
    ck_assert(dataSetWriter->config.configurationFrozen == UA_TRUE);
    ck_assert(dataSetField->config.configurationFrozen == UA_TRUE);
    ck_assert(pubSubConnection->config->configurationFrozen == UA_TRUE);
    UA_PublishedDataSet *publishedDataSet = UA_PublishedDataSet_findPDSbyId(server, dataSetWriter->connectedDataSet);
    ck_assert(publishedDataSet->config.configurationFrozen == UA_TRUE);
    UA_DataSetField *dsf;
    TAILQ_FOREACH(dsf ,&publishedDataSet->fields , listEntry){
        ck_assert(dsf->config.configurationFrozen == UA_TRUE);
    }
    //set state to disabled and implicit unlock the configuration
        UA_Server_unfreezeWriterGroupConfiguration(server, writerGroup1);
} END_TEST

START_TEST(CreateAndLockConfigurationWithExternalAPI) {
        //create config
        UA_NodeId connection1, writerGroup1, publishedDataSet1, dataSetWriter1, dataSetField1;
        UA_PubSubConnectionConfig connectionConfig;
        memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
        connectionConfig.name = UA_STRING("UADP Connection");
        UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
        UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                             &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
        connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
        UA_Server_addPubSubConnection(server, &connectionConfig, &connection1);

        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name = UA_STRING("WriterGroup 1");
        writerGroupConfig.publishingInterval = 10;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
        UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &writerGroup1);

        UA_PublishedDataSetConfig pdsConfig;
        memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
        pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
        pdsConfig.name = UA_STRING("PublishedDataSet 1");
        UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet1);

        UA_DataSetFieldConfig fieldConfig;
        memset(&fieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        fieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        fieldConfig.field.variable.fieldNameAlias = UA_STRING("field 1");
        UA_Server_addDataSetField(server, publishedDataSet1, &fieldConfig, &dataSetField1);

        UA_DataSetField *dataSetField = UA_DataSetField_findDSFbyId(server, dataSetField1);
        ck_assert(dataSetField->config.configurationFrozen == UA_FALSE);

        //get internal WG Pointer
        UA_WriterGroup *writerGroup = UA_WriterGroup_findWGbyId(server, writerGroup1);
        ck_assert(writerGroup->state == UA_PUBSUBSTATE_DISABLED);

        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("DataSetWriter 1");
        UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter1);
        UA_DataSetWriter *dataSetWriter = UA_DataSetWriter_findDSWbyId(server, dataSetWriter1);

        //get internal PubSubConnection Pointer
        UA_PubSubConnection *pubSubConnection = UA_PubSubConnection_findConnectionbyId(server, connection1);

        ck_assert(dataSetWriter->config.configurationFrozen == UA_FALSE);
        //Lock the with the freeze function
        UA_Server_freezeWriterGroupConfiguration(server, writerGroup1);
        ck_assert(dataSetWriter->config.configurationFrozen == UA_TRUE);
        ck_assert(dataSetField->config.configurationFrozen == UA_TRUE);
        ck_assert(pubSubConnection->config->configurationFrozen == UA_TRUE);
        UA_PublishedDataSet *publishedDataSet = UA_PublishedDataSet_findPDSbyId(server, dataSetWriter->connectedDataSet);
        ck_assert(publishedDataSet->config.configurationFrozen == UA_TRUE);
        UA_DataSetField *dsf;
        TAILQ_FOREACH(dsf ,&publishedDataSet->fields , listEntry){
            ck_assert(dsf->config.configurationFrozen == UA_TRUE);
        }
        //set state to disabled and implicit unlock the configuration
        UA_Server_unfreezeWriterGroupConfiguration(server, writerGroup1);
    } END_TEST

START_TEST(CreateAndReleaseMultiplePDSLocks) {
    //create config
    UA_NodeId connection1, writerGroup1, writerGroup2, publishedDataSet1, dataSetWriter1, dataSetWriter2, dataSetWriter3, dataSetField1;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_Server_addPubSubConnection(server, &connectionConfig, &connection1);

    //Add two writer groups
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
    writerGroupConfig.name = UA_STRING("WriterGroup 1");
    writerGroupConfig.publishingInterval = 10;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &writerGroup1);
    writerGroupConfig.name = UA_STRING("WriterGroup 2");
    UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &writerGroup2);

    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("PublishedDataSet 1");
    UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet1);

    UA_DataSetFieldConfig fieldConfig;
    memset(&fieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    fieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    fieldConfig.field.variable.fieldNameAlias = UA_STRING("field 1");
    UA_Server_addDataSetField(server, publishedDataSet1, &fieldConfig, &dataSetField1);

    UA_DataSetField *dataSetField = UA_DataSetField_findDSFbyId(server, dataSetField1);

    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("DataSetWriter 1");
    UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter1);
    dataSetWriterConfig.name = UA_STRING("DataSetWriter 2");
    UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter2);
    dataSetWriterConfig.name = UA_STRING("DataSetWriter 3");
    UA_Server_addDataSetWriter(server, writerGroup2, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter3);

    UA_WriterGroup *writerGroup_1 = UA_WriterGroup_findWGbyId(server, writerGroup1);
    UA_WriterGroup *writerGroup_2 = UA_WriterGroup_findWGbyId(server, writerGroup2);
    UA_PublishedDataSet *publishedDataSet = UA_PublishedDataSet_findPDSbyId(server, publishedDataSet1);
    UA_PubSubConnection *pubSubConnection = UA_PubSubConnection_findConnectionbyId(server, connection1);
    //freeze configuratoin of both WG
    ck_assert(writerGroup_1->config.configurationFrozen == UA_FALSE);
    ck_assert(writerGroup_2->config.configurationFrozen == UA_FALSE);
    ck_assert(publishedDataSet->config.configurationFrozen == UA_FALSE);
    ck_assert(pubSubConnection->config->configurationFrozen == UA_FALSE);
        UA_Server_freezeWriterGroupConfiguration(server, writerGroup1);
        UA_Server_freezeWriterGroupConfiguration(server, writerGroup2);
    ck_assert(writerGroup_1->config.configurationFrozen == UA_TRUE);
    ck_assert(writerGroup_2->config.configurationFrozen == UA_TRUE);
    ck_assert(publishedDataSet->config.configurationFrozen == UA_TRUE);
    ck_assert(pubSubConnection->config->configurationFrozen == UA_TRUE);
    //unlock one tree, get sure pds still locked
        UA_Server_unfreezeWriterGroupConfiguration(server, writerGroup1);
    ck_assert(writerGroup_1->config.configurationFrozen == UA_FALSE);
    ck_assert(publishedDataSet->config.configurationFrozen == UA_TRUE);
    ck_assert(dataSetField->config.configurationFrozen == UA_TRUE);
        UA_Server_unfreezeWriterGroupConfiguration(server, writerGroup2);
    ck_assert(publishedDataSet->config.configurationFrozen == UA_FALSE);
    ck_assert(dataSetField->config.configurationFrozen == UA_FALSE);
    ck_assert(pubSubConnection->config->configurationFrozen == UA_FALSE);

    } END_TEST

START_TEST(CreateLockAndEditConfiguration) {
    UA_NodeId connection1, writerGroup1, publishedDataSet1, dataSetWriter1;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_Server_addPubSubConnection(server, &connectionConfig, &connection1);

    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
    writerGroupConfig.name = UA_STRING("WriterGroup 1");
    writerGroupConfig.publishingInterval = 10;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &writerGroup1);

    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("PublishedDataSet 1");
    UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet1);

    UA_DataSetFieldConfig fieldConfig;
    memset(&fieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    fieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    fieldConfig.field.variable.fieldNameAlias = UA_STRING("field 1");
    UA_NodeId localDataSetField;
    UA_Server_addDataSetField(server, publishedDataSet1, &fieldConfig, &localDataSetField);

    //get internal WG Pointer
    UA_WriterGroup *writerGroup = UA_WriterGroup_findWGbyId(server, writerGroup1);
    ck_assert(writerGroup->state == UA_PUBSUBSTATE_DISABLED);

    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("DataSetWriter 1");
    UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter1);
    UA_DataSetWriter *dataSetWriter = UA_DataSetWriter_findDSWbyId(server, dataSetWriter1);

    ck_assert(dataSetWriter->config.configurationFrozen == UA_FALSE);
    //Lock the writer group and the child pubsub entities
        UA_Server_freezeWriterGroupConfiguration(server, writerGroup1);
    //call not allowed configuration methods
    UA_DataSetFieldResult fieldRemoveResult = UA_Server_removeDataSetField(server, localDataSetField);
    ck_assert(fieldRemoveResult.result == UA_STATUSCODE_BADCONFIGURATIONERROR);
    ck_assert(UA_Server_removePublishedDataSet(server, publishedDataSet1) == UA_STATUSCODE_BADCONFIGURATIONERROR);
        UA_Server_unfreezeWriterGroupConfiguration(server, writerGroup1);
    fieldRemoveResult = UA_Server_removeDataSetField(server, localDataSetField);
    ck_assert(fieldRemoveResult.result == UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(CreateConfigWithStaticFieldSource) {
    UA_NodeId connection1, writerGroup1, publishedDataSet1, dataSetWriter1;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_Server_addPubSubConnection(server, &connectionConfig, &connection1);

    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
    writerGroupConfig.name = UA_STRING("WriterGroup 1");
    writerGroupConfig.publishingInterval = 10;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &writerGroup1);

    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("PublishedDataSet 1");
    UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet1);

    UA_UInt32 *intValue = UA_UInt32_new();
    UA_Variant variant;
    memset(&variant, 0, sizeof(UA_Variant));
    UA_Variant_setScalar(&variant, intValue, &UA_TYPES[UA_TYPES_UINT32]);
    UA_DataValue staticValueSource;
    memset(&staticValueSource, 0, sizeof(staticValueSource));
    staticValueSource.value = variant;

    UA_DataSetFieldConfig fieldConfig;
    memset(&fieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    fieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    fieldConfig.field.variable.fieldNameAlias = UA_STRING("field 1");
    fieldConfig.field.variable.staticValueSourceEnabled = UA_TRUE;
    fieldConfig.field.variable.staticValueSource.value = variant;
    UA_NodeId localDataSetField;
    UA_Server_addDataSetField(server, publishedDataSet1, &fieldConfig, &localDataSetField);

    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("DataSetWriter 1");
    UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter1);
    UA_DataValue_deleteMembers(&staticValueSource);
    } END_TEST

int main(void) {
    TCase *tc_lock_configuration = tcase_create("Create and Lock");
    tcase_add_checked_fixture(tc_lock_configuration, setup, teardown);
    tcase_add_test(tc_lock_configuration, CreateAndLockConfiguration);
    tcase_add_test(tc_lock_configuration, CreateAndLockConfigurationWithExternalAPI);
    tcase_add_test(tc_lock_configuration, CreateAndReleaseMultiplePDSLocks);
    tcase_add_test(tc_lock_configuration, CreateLockAndEditConfiguration);
    tcase_add_test(tc_lock_configuration, CreateConfigWithStaticFieldSource);

    Suite *s = suite_create("PubSub RT configuration lock mechanism");
    suite_add_tcase(s, tc_lock_configuration);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
