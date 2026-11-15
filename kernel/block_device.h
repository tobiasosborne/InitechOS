/* Compatibility include for kernel-owned sources. The existing INT13/drive-map
 * declarations are public so host transports use the same contract. */
#include "include/block_device.h"
/* Kernel startup only: identify the running system separately from the A:
 * raw-write watch. The C: delivery move must preserve A: cache coherence. */
void block_device_running(uint8_t drive, uint32_t base);
