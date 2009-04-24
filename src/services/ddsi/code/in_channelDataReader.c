#include "os_heap.h"
#include "v_group.h"
#include "v_groupSet.h"
#include "v_networkReader.h"
#include "in__configChannel.h"
#include "in__configDataChannel.h"
#include "in__ddsiPublication.h"
#include "in_commonTypes.h"
#include "in_streamReader.h"
#include "in_channel.h"
#include "in_channelReader.h"
#include "in_channelDataReader.h"
#include "in_report.h"

/** */
static in_result
in_channelDataReaderProcessDataFunc(
    in_streamReaderCallbackArg _this,
    v_message message,
    in_connectivityPeerWriter peerWriter,
    in_ddsiReceiver receiver);

/** */
static in_result
in_channelDataReaderProcessAckNackFunc(
        in_streamReaderCallbackArg _this,
        in_ddsiAckNack event,
        in_ddsiReceiver receiver);

/** */
static void
in_channelDataReaderMain(
    v_entity e,
    c_voidp arg);

/** */
static void *
in_channelDataReaderMainFunc(
    in_runnable runnable);

/** */
static void
in_channelDataReaderTrigger(
    in_runnable runnable);

/** */
static void
in_channelDataReaderDeinit(
    in_object obj);

/** */
static os_boolean
in_channelDataReaderInit(
    in_channelDataReader _this,
    in_channelData channelData,
    in_configChannel config,
    u_networkReader reader);

OS_STRUCT(in_channelDataReader){
    OS_EXTENDS(in_channelReader);
    in_streamReader streamReader;
    u_networkReader userReader;
    v_networkReader kernelReader;
    os_time timeout;
};

/** */
in_channelDataReader
in_channelDataReaderNew(
    in_channelData channelData,
    in_configChannel config,
    u_networkReader reader)
{
    in_channelDataReader _this;
    os_boolean success;

    assert(channelData);
    assert(config);
    assert(reader);

    _this = (in_channelDataReader)os_malloc(sizeof(*_this));
    if(_this)
    {
        success = in_channelDataReaderInit(
            _this,
            channelData,config,reader);

        if(!success)
        {
            os_free(_this);
            _this = NULL;
            IN_TRACE_1(Construction,2,"in_channelDataReader creation failed = %x",_this);
        } else
        {
            IN_TRACE_1(Construction, 2, "in_channelDataReader creation successful = %x", _this);
        }
    }


    return _this;
}

/** */
static OS_STRUCT(in_streamReaderCallbackTable)
in_channelDataReaderCallbackTable =
{
        NULL, /* processPeerEntity*/
        in_channelDataReaderProcessDataFunc, /* processDataFunc */
        NULL, /* processDataFrag */
        in_channelDataReaderProcessAckNackFunc, /* processAckNack */
        NULL, /* processNackFrag */
        NULL, /* processHeartbeat */
        NULL /* requestNackFrag */
};

/** */
os_boolean
in_channelDataReaderInit(
    in_channelDataReader _this,
    in_channelData channelData,
    in_configChannel config,
    u_networkReader reader)
{
    os_boolean success;

    assert(_this);
    assert(channelData);
    assert(config);
    assert(reader);

    success = in_channelReaderInit(
        OS_SUPER(_this),
        IN_OBJECT_KIND_DATA_CHANNEL_READER,
        in_channelDataReaderDeinit,
        in_configDataChannelGetName(in_configDataChannel(config)),
        in_configChannelGetPathName(config),
        in_channelDataReaderMainFunc,
        in_channelDataReaderTrigger,
        in_channel(channelData));

    if(success)
    {
        _this->streamReader = in_streamGetReader(in_channelGetStream(in_channel(channelData)));
        _this->userReader = reader;
        _this->timeout.tv_sec = 1;
        _this->timeout.tv_nsec = 0;
    }
    return success;
}

/** */
void
in_channelDataReaderDeinit(
    in_object obj)
{
    in_channelDataReader _this;

    assert(obj);
    assert(in_channelDataReaderIsValid(obj));

    _this = (in_channelDataReader)obj;

    in_streamReaderFree(_this->streamReader);

    /*Call parent deinit*/
    in_channelReaderDeinit(obj);
}

/** */
in_result
in_channelDataReaderProcessDataFunc(
    in_streamReaderCallbackArg _this,
    v_message message,
    in_connectivityPeerWriter peerWriter,
    in_ddsiReceiver receiver)
{
    in_result result = IN_RESULT_ERROR;
    in_channelDataReader channelReader;
    v_kernel kernel = NULL;
    in_ddsiDiscoveredWriterData data;

    assert(_this);
    assert(message);
    assert(peerWriter);
    assert(receiver);

    channelReader = in_channelDataReader(_this);
    IN_TRACE(Receive, 3, "in_channelDataReaderProcessDataFunc");
    kernel = v_objectKernel(channelReader->kernelReader);
    if ( kernel )
    {
        c_string partitionName;
        v_group group = NULL;

        data = in_connectivityPeerWriterGetInfo(peerWriter);
        /* resolve the group, based on the topic- and partitionname from the
         * peerwriter
         * ToDo: currently we only write to the first partition in the
         * partition-array
         */

        partitionName =  (c_string)(data->topicData.info.partition.name);//TODO temp code
        if(!partitionName)//
        {
            partitionName = c_stringNew(c_getBase(kernel), "");//
        }
        group = v_groupSetGet(
            kernel->groupSet,
            partitionName,
            data->topicData.info.topic_name);

        IN_TRACE_2(
            Receive,
            3,
            "in_channelDataReader Process Message (%s,%s)",
            partitionName,//
            data->topicData.info.topic_name);
        if ( group )
        {
            v_networkReaderEntry entry = NULL;

            /* find the entry to obtain the networkId */
            entry = v_networkReaderLookupEntry(channelReader->kernelReader, group);
            if ( entry )
            {
                v_writeResult kernelResult;

                /* Write the v_message to the group */
                kernelResult = v_groupWrite(group, message, NULL, entry->networkId);
                if ( kernelResult == V_WRITE_SUCCESS )
                {
                   result = IN_RESULT_OK;
                }
            }
        }
    }
    return result;
}

/** */
static in_result
in_channelDataReaderProcessAckNackFunc(
        in_streamReaderCallbackArg _this,
        in_ddsiAckNack event,
        in_ddsiReceiver receiver)
{
    IN_TRACE(Receive,3,"in_channelDataReaderProcessDataFuncFunc CALLBACK");
    return IN_RESULT_OK;
}

/** */
void
in_channelDataReaderMain(
    v_entity e,
    c_voidp arg)
{
    const os_uint32 ERROR_INTERVAL = 500;
    os_uint32 errorCounter = 0;

    os_time pollTimeout = {0,0};
    in_channelDataReader channelReader;
    in_result result = IN_RESULT_ERROR;
    assert(e);
    assert(arg);

    channelReader = in_channelDataReader(arg);
    channelReader->kernelReader = v_networkReader(e);

    while (!(int)in_runnableTerminationRequested((in_runnable)channelReader))
    {
        /* renew timeout */
        pollTimeout = channelReader->timeout;

        result =
            in_streamReaderScan(
                channelReader->streamReader,
                &in_channelDataReaderCallbackTable, /* static vtable */
                (in_streamReaderCallbackArg)channelReader,
                &pollTimeout);
        /** may return before timeout as exceeded, pollTimeout
         * contains the remaning time  */
        if (result != IN_RESULT_OK && result != IN_RESULT_TIMEDOUT) {
            if (errorCounter == 0) {
                /* first occurance */
                IN_REPORT_WARNING(IN_SPOT,
                        "unexpected data read error");
            } else if (errorCounter >= ERROR_INTERVAL) {
                IN_REPORT_WARNING_1(IN_SPOT,
                        "unexpected data read error (repeated %d times)",
                        errorCounter);
                errorCounter = 0;
            }
            ++errorCounter;
            /* POST: errorCounter >= 1 */
        }
    }
    channelReader->kernelReader = NULL;
}

/** */
void*
in_channelDataReaderMainFunc(
    in_runnable runnable)
{
    u_result result;
    in_channelDataReader channelReader;

    assert(runnable);

    channelReader = in_channelDataReader(runnable);

    in_runnableSetRunState(runnable, IN_RUNSTATE_RUNNING);
    result = u_entityAction(
        u_entity(channelReader->userReader),
        in_channelDataReaderMain,
        channelReader);
    in_runnableSetRunState(runnable, IN_RUNSTATE_TERMINATED);

    return NULL;
}

/** */
void
in_channelDataReaderTrigger(
    in_runnable runnable)
{
    /* TODO tbd */
}

in_result ProcessPeerEntityFuncDummy(
    in_streamReaderCallbackArg _this,
    in_discoveredPeer discoveredPeer)
{
    IN_TRACE_1(Receive,3,"ProcessPeerEntityFuncDummy called on datachannel %x",_this);
    return IN_RESULT_OK;
}

