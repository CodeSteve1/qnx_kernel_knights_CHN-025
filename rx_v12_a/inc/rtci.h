#ifndef RTCI_H
#define RTCI_H

/* Real-Time Clinical Index: combines NEWS2 and APACHE II scores
 * (via the external RTCI engine, see PatientParameters.h) into a
 * single severity score used to rank and prioritize critical
 * patients on-screen. Higher = more severe. */
float calculate_rtci(int p_idx);

#endif /* RTCI_H */
