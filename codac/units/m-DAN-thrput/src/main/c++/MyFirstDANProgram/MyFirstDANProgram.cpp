/******************************************************************************
 * $HeadURL: https://svnpub.iter.org/codac/iter/codac/dev/units/m-codac-unit-templates/branches/codac-core-5.3/templates/cpp-sdn/main/c++/prog/prog.cpp $
 * $Id: prog.cpp 63347 2016-02-21 00:27:22Z zagara $
 *
 * Project	: CODAC Core System
 *
 * Description	: Program template
 *
 * Author        : DAN team
 *
 * Copyright (c) : 2010-2016 ITER Organization,
 *				  CS 90 046
 *				  13067 St. Paul-lez-Durance Cedex
 *				  France
 *
 * This file is part of ITER CODAC software.
 * For the terms and conditions of redistribution or use of this software
 * refer to the file ITER-LICENSE.TXT located in the top level directory
 * of the distribution package.
 ******************************************************************************/

/* Global header files */

/* Local header files */

/* sys-headers.h -- Part of the program template to host all OS-specific
   program header files */
#include "sys-headers.h"

/* ccs-headers.h -- Part of the program template to host all CCS-specific
   program header files */
#include "ccs-headers.h"

/* includetopics.h -- Created and maintained by SDD. It includes the list of
   header files for all the topics the application is participating to. */
#include "includetopics.h"

/* topicvars.h -- Created and maintained by SDD. Contains the declaration of
   the application-specific topics and participants. The file is optional and
   associated to the configureSDN, cleanupSDN helper functions which can be
   replaced by explicit class instantiation */
#include "topicvars.h"

#include "sdd-dan.h"
#include "dan_MetadataTypes.h"

#include "pv_access.h"
#include "assert.h"


#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a > _b ? _a : _b; })


using namespace std;

#define SAMPLE_TYPE float

extern int configureSDN();
extern int cleanupSDN();

volatile sig_atomic_t __terminate = 0;


void signal_handler (int signal)
{
    log_info("Received signal '%d' to terminate", signal);
    __terminate = true;
};




///
/// \brief initBufferWithData
/// \param buffer data buffer
/// \param size size of chunk
/// \param pos current position to generation start
/// \param R number of samples per second ( Sampling Rate )
/// \param T time period for sine
///
static void initBufferWithSineData(char *buffer, size_t size, size_t pos, double R, double T=1)
{
    SAMPLE_TYPE *data=(SAMPLE_TYPE *)buffer;
    pos  /= sizeof(SAMPLE_TYPE);
    size /= sizeof(SAMPLE_TYPE);
    for(size_t i=0;i<size;i++)
        data[i]=sin(2 * M_PI * (i+pos) / R  / T);
}

static SAMPLE_TYPE * alloc_static_sine(size_t N) {
    SAMPLE_TYPE *data = (SAMPLE_TYPE*)malloc(N*sizeof(SAMPLE_TYPE));
    for(size_t i=0;i<N;i++)
        data[i]=sin(2 * M_PI * (double)i / N);
    return data;
}


///
/// \brief initBufferWithStaticSineData
/// \param buffer data buffer to fill
/// \param size size of buffer in char
/// \param ipos initial position in char
///
static void initBufferWithStaticSineData(char *buffer, size_t size, size_t ipos)
{
    static SAMPLE_TYPE *data = NULL;
    static const size_t N = 1024000;
    if(!data) data = alloc_static_sine(N/sizeof(SAMPLE_TYPE));

    //  NAIVE FILL IN  //
    //
    //    pos  /= sizeof(SAMPLE_TYPE);
    //    size /= sizeof(SAMPLE_TYPE);
    //    SAMPLE_TYPE *buffer_t = (SAMPLE_TYPE *)buffer;
    //    for(size_t i=0;i<size;i++)
    //        buffer_t[i]=data[(i+pos)%N];

    //  EFFICIENT COPY  //
    //
    assert(size >= N);
    ipos %= N;
    memcpy(buffer, data+ipos, N-ipos);
    buffer += N-ipos;
    size_t stps = (size-ipos)/N;
    for(size_t i=0; i < stps; ++i ) {
        memcpy(buffer,data,N);
        buffer += N;
    }
    memcpy(buffer,data,(size-ipos)%N);
}



int main(int argc, char **argv)
{
    // example of code to demonstrate a DAN source
    // DummySrc - which publishes 100 sample very 1 ms.

    dan_DataCore dc = NULL; // DAN Datacore reference declaration
    dan_Source   ds = NULL; // DAN source declaration

    int exi_err = 0;
    int pon_err = 0;
    int tcn_err = 0;
    int dan_err = 0;
    size_t counter = 0;
    SAMPLE_TYPE *values = NULL;

    // DATA BLOCK HEADER //
    DAN_DATA_BLOCK_HEADER_TYPE_IMG blockHead;

    // SIGNALS FOR GRACEFUL TERMINATION //
    sigset(SIGTERM, signal_handler);
    sigset(SIGINT,  signal_handler);
    sigset(SIGHUP,  signal_handler);

    // LOG INIT //
    InitializeLOG();

    pon_err = ca_context_create(ca_disable_preemptive_callback);
    if(pon_err != ECA_NORMAL) {
        log_error("Error during PON init");
        exi_err = 1;
    }

    PV<bool> gen_enable  ("EC-DAN-ACQ:GEN_ENABLE"); (void)gen_enable;
    PV<long> gen_rate_KB ("EC-DAN-ACQ:GEN_FREQ");    // KBps
    PV<long> gen_csiz_KB ("EC-DAN-ACQ:GEN_PKTSIZE"); // KB

    size_t gen_rate  = gen_rate_KB * 1024;
    size_t gen_csiz  = gen_csiz_KB * 1024;

    // Block size constant in Bytes
    size_t chunkSize  = ((gen_csiz == 0 ) ? 102400 : gen_csiz);
    size_t bufferSize = 2*chunkSize;  // Source DAQBuffer size
    double sampleRate = gen_rate / sizeof(SAMPLE_TYPE); // Ksps

    log_info("[GEN] rate: %d, chunk size: %d", (long)gen_rate, chunkSize);

    // DAN DATACORE INIT //
    dc = dan_initLibrary();
    if (dc == NULL) {
        log_error("Error during DAN init ");
        exi_err = 1;
        __terminate = true;
    }

    // TCN INIT //
    tcn_err = tcn_init();
    if (tcn_err != TCN_SUCCESS)
    {
        log_error( "TCN init NOK : %s",tcn_strerror(tcn_err));
        exi_err =1;
        __terminate = true;
    }

    // DAN SOURCE PUBLISH //
    // Initialize source with a internal DAQBuffer allocating a size in bytes
    // of ( blockSize * nBlocks * sampleSize )
    ds = dan_publisher_publishSource_withDAQBuffer( dc,
                                                    (const char *) DummySrc,
                                                    bufferSize);
    if (ds == NULL) {
        log_final("Error while publishing DAN source %s ", DummySrc);
        exi_err =1;
        __terminate = true;
    }
    log_info("DAN PUBLISH SOURCE OK ");


    // OPEN STREAM //
    // Open a new stream in the source previously initialized with 1000
    // samples/second of sampling rate and 0 bytes of initial offset into
    // DAQBuffer.
    dan_publisher_openStream(ds,sampleRate,0);

    // TCN get time //
    uint64_t max_duration = 6E9; // 6s
    uint64_t till;
    uint64_t init_acq = 0;

    tcn_err = tcn_get_time(&till);
    if (tcn_err != TCN_SUCCESS) {
        log_error( "TCN gettime NOK : %s\n", tcn_strerror(tcn_err));
        exi_err =1;
        __terminate = true;
    } else {
        init_acq=till;
    }

    // FILL DATA //
    values = (SAMPLE_TYPE *) malloc ( 2 * MAX(gen_rate, chunkSize) );
    initBufferWithSineData((char *)values,
                           2 * MAX(gen_rate, chunkSize), // size Bytes
                           0,             // pos  Bytes
                           sampleRate,    // R
                           1);            // T (1s period)


    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    size_t pos = 0;
    while(!__terminate && ( tcn_wait_until(till,0) == TCN_SUCCESS )
                       && ( till-init_acq ) < max_duration )
        // WAIT UNTIL ABSOLUTE TIME OR MAX DURATION //
    {

//        // -- DATA BLOCK HEADER ---- //
//        blockHead.std_header.dim1=1;
//        blockHead.std_header.dim2=1;
        blockHead.std_header.operational_mode = 0;
        blockHead.std_header.sampling_rate    = sampleRate;
//        blockHead.pos_x += 1;
//        blockHead.pos_y += 1;

        // PUBLISHER PUT ///////////////////////////////////////////////////////
        //
        // Push new data block and its modified header into source
        // put NULL if you didn’t define blockHead
        dan_err = dan_publisher_putDataBlock (ds,
                                              till,
                                              ((char *)values) + pos,
                                              chunkSize,
                                              (char *)&blockHead);
        pos = (pos+chunkSize)%gen_rate;

        // Detected overflow in pass datablock reference queue
        if (dan_err == -1) {
            log_warning( "[GEN] %s, Queue overflow with policy %d\n",
                      DummySrc, dan_publisher_getCheckPolicy(ds));
        }
        // Detected overflow in DAQBuffer
        else if (dan_err == -2) {
            log_warning("[GEN] %s, DAQ buffer overflow with policy %d\n",
                     DummySrc, dan_publisher_getCheckPolicy(ds));
        }

        // ACQ interval
        double dt_ns = (double)chunkSize / gen_rate * 1E9;
        till += (int)dt_ns;
        //        ++counter;

    } // LOOP END //////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////

    log_info( "DAN end of acquisition, closing all resources");

    // CLOSE STREAM //
    dan_publisher_closeStream(ds);

    if (values != NULL) { free(values); }

    // UNPUBLISH SOURCE //
    if (ds != NULL) {
        dan_publisher_unpublishSource(dc, ds);
    }

    // CLOSE DAN API //
    if (dc != NULL) {
        dan_closeLibrary(dc);
    }

    // CLOSE LOG //
    log_info( "DAN end of acquisition, resources are now closed");
    log_flush_queue();
    log_finalize();
    TerminateLOG();

    return (exi_err );
}
