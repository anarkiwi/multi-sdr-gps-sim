/**
 * multi-sdr-gps-sim generates a IQ data stream on-the-fly to simulate a
 * GPS L1 baseband signal using a SDR platform like HackRF or ADLAM-Uhd.
 *
 * This file is part of the Github project at
 * https://github.com/mictronics/multi-sdr-gps-sim.git
 *
 * Copyright © 2021 Mictronics
 * Distributed under the MIT License.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <sched.h>
#include <unistd.h>
/* for PRIX64 */
#include <inttypes.h>
#include <uhd.h>
#include "fifo.h"
#include "gui.h"
#include "sdr.h"
#include "sdr_uhd.h"

#include <sys/time.h>

static atomic_bool uhd_tx_thread_exit = false;
static pthread_t uhd_tx_thread;
static const int gui_y_offset = 4;
static const int gui_x_offset = 2;

static struct {
    uhd_usrp_handle usrp;
    uhd_tx_metadata_handle md;
    uhd_tx_streamer_handle tx_streamer;
    size_t channel;
    size_t samps_per_buff;
} uhd;

static void *uhd_tx_thread_ep(void *arg) {
    (void) arg; // Not used

    // Try sticking this thread to core 2
    thread_to_core(2);
    set_thread_name("uhdsdr-thread");

    size_t num_samps_sent = 0;
    int ret = 0;

    while (!uhd_tx_thread_exit) {
         // Get a fifo block
         struct iq_buf *iq = fifo_dequeue();
         if (iq != NULL && iq->data16 != NULL) {
             // Fifo has transfer block size
             ret = uhd_tx_streamer_send(
               uhd.tx_streamer, &(iq->data16), iq->validLength / 2, &uhd.md, 0.1, &num_samps_sent);
             if (ret) {
               gui_status_wprintw(RED, "Error pushing TX buffer: %d\n", ret);
               break;
             }
             // Release and free up used block
             fifo_release(iq);
         } else {
             break;
         }
    }

    // TODO: cleanup
    pthread_exit(NULL);
}

int sdr_uhd_init(simulator_t *simulator) {
    int ret;
    int y = gui_y_offset;

    if (simulator->sample_size == SC08) {
        gui_status_wprintw(YELLOW, "8 bit sample size requested. Reset to 16 bit with Uhd.\n");
    }
    simulator->sample_size = SC16;

    ret = uhd_usrp_make(&uhd.usrp, "");
    if (ret) {
       gui_status_wprintw(RED, "uhd_usrp_make failed: %d\n", ret);
       return -1;
    }

    ret = uhd_tx_streamer_make(&uhd.tx_streamer);
    if (ret) {
       gui_status_wprintw(RED, "uhd_tx_streamer_make failed: %d\n", ret);
       return -1;
    }

    ret = uhd_tx_metadata_make(&uhd.md, false, 0, 0.1, true, false);
    if (ret) {
       gui_status_wprintw(RED, "uhd_tx_metadata_make failed: %d\n", ret);
       return -1;
    }

    uint64_t freq_gps_hz = TX_FREQUENCY;
    freq_gps_hz = freq_gps_hz * (10000000 - simulator->ppb) / 10000000;

    uhd_tune_request_t tune_request = {.target_freq = freq_gps_hz,
        .rf_freq_policy                             = UHD_TUNE_REQUEST_POLICY_AUTO,
        .dsp_freq_policy                            = UHD_TUNE_REQUEST_POLICY_AUTO};
    uhd_tune_result_t tune_result;

    uhd_stream_args_t stream_args = {.cpu_format = "sc16",
        .otw_format                              = "sc16",
        .args                                    = "",
        .channel_list                            = &uhd.channel,
        .n_channels                              = 1};

    ret = uhd_usrp_set_tx_rate(uhd.usrp, TX_SAMPLERATE, uhd.channel);
    if (ret) {
       gui_status_wprintw(RED, "uhd_usrp_set_tx_rate failed: %d\n", ret);
       return -1;
    }

    ret = uhd_usrp_set_tx_freq(uhd.usrp, &tune_request, uhd.channel, &tune_result);
    if (ret) {
       gui_status_wprintw(RED, "uhd_usrp_set_tx_req failed: %d\n", ret);
       return -1;
    }

    stream_args.channel_list = &uhd.channel;
    ret = uhd_usrp_get_tx_stream(uhd.usrp, &stream_args, uhd.tx_streamer);
    if (ret) {
       gui_status_wprintw(RED, "uhd_usrp_get_tx_stream failed: %d\n", ret);
       return -1;
    }

    ret = uhd_tx_streamer_max_num_samps(uhd.tx_streamer, &uhd.samps_per_buff);
    if (ret) {
       gui_status_wprintw(RED, "uhd_tx_streamer_max_num_samps failed: %d\n", ret);
       return -1;
    }

    if (!fifo_create(NUM_FIFO_BUFFERS, IQ_BUFFER_SIZE, SC16)) {
        gui_status_wprintw(RED, "Error creating IQ file fifo!\n");
        return -1;
    }

    gui_status_wprintw(RED, "SUCCESS!\n");

    return 0;
}

void sdr_uhd_close(void) {
    uhd_tx_thread_exit = true;
    fifo_halt();
    fifo_destroy();
    pthread_join(uhd_tx_thread, NULL);
}

int sdr_uhd_run(void) {
    fifo_wait_full();
    pthread_create(&uhd_tx_thread, NULL, uhd_tx_thread_ep, NULL);
    return 0;
}

int sdr_uhd_set_gain(const int gain) {
    double g = gain;
    int ret;

    if (g > UHD_TX_GAIN_MAX) g = UHD_TX_GAIN_MAX;
    if (g < UHD_TX_GAIN_MIN) g = UHD_TX_GAIN_MIN;

    ret = uhd_usrp_set_tx_gain(&uhd.usrp, g, uhd.channel, "");
    if (ret) {
        gui_status_wprintw(RED, "Error setting gain %f: %d\n", g, ret);
    }

    return (int) (g);
}
