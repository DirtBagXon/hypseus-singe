#ifndef LDP_VLDP_GP2X_H
#define LDP_VLDP_GP2X_H

#ifdef GP2X
int prepare_gp2x_frame_callback(struct yuy2_buf *src);

void display_gp2x_frame_callback();
#endif // GP2X

#endif // LDP_VLDP_GP2X_H
