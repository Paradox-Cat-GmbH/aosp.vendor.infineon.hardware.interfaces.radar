package vendor.infineon.radar;

/**
 * NOTE: this file is AIDL adaptation of ifxAvian/DeviceConfig.h, 
 *       make sure it coresponds to current version when updating SDK!
 *       It is identical to ifx_Avian_Config_t, but all integers must be kept unsigned.
 *
 * @brief Defines the structure for acquisition of time domain data related settings.
 *
 * When a connection to sensor device is established, the device is configured according to the
 * parameters of this struct.
 *
 * The power modes (shape end power mode and frame end power mode) are
 * configured to minimize the power consumption. Depending on how much time is
 * left at the end of a shape and at the end of the frame, the lowest power
 * modes possible are chosen.
 */
@VintfStability
parcelable SensorConfig {
    /** @brief ADC sampling rate (in Hz)
     *
     * Sampling rate of the ADC used to acquire the samples during a chirp. The
     * duration of a single chirp depends on the number of samples and the
     * sampling rate.
     */
    int sample_rate_Hz;

    /** @brief Bitmask of activated RX antennas
     *
     * Bitmask where each bit represents one RX antenna of the radar device. If
     * a bit is set the according RX antenna is enabled during the chirps and
     * the signal received through that antenna is captured. The least
     * significant bit corresponds to antenna RX1.
     */
    int rx_mask;

    /** @brief Bitmask of activated TX antennas
     *
     * Bitmask where each bit represents one TX antenna. The least significant
     * bit corresponds to antenna TX1. It is also possible to disable all TX
     * antennas by setting tx_mask to 0.
     */
    int tx_mask;

    /** @brief TX power level
     *
     * This value controls the transmitted power (allowed values in the range
     * [0,31]). Higher values correspond to higher TX powers.
     */
    int tx_power_level;

    /** @brief IF Gain (in dB)
     *
     * Amplification factor that is applied to the IF signal coming from the RF
     * mixer before it is fed into the ADC (allowed values in the range
     * [18,60]).
     *
     * Internally, the values for HP gain (possible values: 18dB, 30dB) and
     * VGA gain (possible values: 0dB, 5dB, 10dB, ..., 30dB) are computed from
     * if_gain_dB. HP gain is chosen as high and VGA gain is chosen as low as
     * possible to reduce noise. HP gain is 18dB if_gain_dB is less than 30,
     * otherwise HP gain is 30dB. VGA gain is chosen such that the sum of
     * HP gain and VGA gain is as close to if_gain_dB as possible.
     *
     * Note that it is not possible to set all values of if_gain_dB exactly.
     * The configured value and actually set value of if_gain_dB might differ
     * by up to 2dB. The value actually set for if_gain_dB can be read using
     * ifx_avian_get_config.
     */
    int if_gain_dB;

    /** @brief Start frequency (in Hz)
     *
     * Start frequency of FMCW chirp. See also sct_radarsdk_introduction_parametersexplained.
     */
    long start_frequency_Hz;

    /** @brief End frequency (in Hz)
     *
     * End frequency of the FMCW chirp. See also sct_radarsdk_introduction_parametersexplained.
     */
    long end_frequency_Hz;

    /** @brief Number of samples per chirp
     *
     * This is the number of samples acquired during each chirp of a frame. The
     * duration of a single chirp depends on the number of samples and the
     * sampling rate.
     */
    int num_samples_per_chirp;

    /** @brief Number of chirps per frame
     *
     * This is the number of chirps in a frame.
     */
    int num_chirps_per_frame;

    /** @brief Chirp repetition time (in seconds)
     *
     * This is the time period that elapses between the beginnings of two
     * consecutive chirps in a frame.
     *
     * The chirp repetition time is also commonly referred to as pulse
     * repetition time or chirp-to-chirp-time)
     */
    float chirp_repetition_time_s;

    /** @brief Frame repetition time (in seconds)
     *
     * This is the time period that elapses between the beginnings of two
     * consecutive frames. The reciprocal of the frame repetition time is the
     * frame rate.
     *
     * The frame repetition time is also commonly referred to as frame time or
     * frame period.
     */
    float frame_repetition_time_s;

    /** @brief Cutoff frequency of the high pass filter (in Hz)
     *
     * The high pass filter is used in order to remove the DC-offset at the
     * output of the RX mixer and also suppress the reflected signal from close
     * in unwanted targets (radome, e.g.).
     *
     * @note Different sensors support different cutoff frequencies. The frequency
     * provided by the user will be rounded to the closest supported cutoff frequency.
     * You can read the cutoff frequency that was actually set using ifx_avian_get_config.
     */
    int hp_cutoff_Hz;

    /** @brief Cutoff frequency of the anti-aliasing filter (in Hz)
     */
    int aaf_cutoff_Hz;

    /** 
     * int value represents following enum:
     * enum ifx_Avian_MIMO_Mode {
     *   IFX_MIMO_OFF = 0,  //!< MIMO is deactivated
     *   IFX_MIMO_TDM = 1   //!< time-domain multiplexing MIMO
     * };
     *
     * @brief Mode of MIMO
     *
     * Mode of MIMO (multiple-input and multiple-output).
     *
     * @note If mimo_mode is IFX_MIMO_TDM the value of tx_mask is ignored.
     */
    int mimo_mode;
}