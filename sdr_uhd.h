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

#ifndef SDR_UHD_H
#define SDR_UHD_H

#define UHD_TX_GAIN_MIN -80
#define UHD_TX_GAIN_MAX 0

int sdr_uhd_init(simulator_t *simulator);
void sdr_uhd_close(void);
int sdr_uhd_run(void);
int sdr_uhd_set_gain(const int gain);

#endif /* SDR_UHD_H */

