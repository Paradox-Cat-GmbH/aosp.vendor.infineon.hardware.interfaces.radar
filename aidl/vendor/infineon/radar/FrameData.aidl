package vendor.infineon.radar;

@VintfStability
parcelable FrameData {
    /**
     * Data array according to configuration of the sensor.
     *
     * Typically expect 3D array with following dimensions:
     * "num_antennas" x "num_chirps_per_frame" x "num_samples_per_chirp"
     * e.g., 3 x 32 x 64
     */
    float[] data;
}