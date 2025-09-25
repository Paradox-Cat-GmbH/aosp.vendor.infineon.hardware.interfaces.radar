///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package vendor.infineon.radar;
@VintfStability
parcelable SensorConfig {
  int sample_rate_Hz;
  int rx_mask;
  int tx_mask;
  int tx_power_level;
  int if_gain_dB;
  long start_frequency_Hz;
  long end_frequency_Hz;
  int num_samples_per_chirp;
  int num_chirps_per_frame;
  float chirp_repetition_time_s;
  float frame_repetition_time_s;
  int hp_cutoff_Hz;
  int aaf_cutoff_Hz;
  int mimo_mode;
}
