#ifndef LDP_VLDP_GL_H
#define LDP_VLDP_GL_H

#ifdef USE_OPENGL

#ifdef MAC_OSX
#include <glew.h>
#else
#include <GL/glew.h>
#endif


void ldp_vldp_gl_think(unsigned int uVblankCount);

bool init_vldp_opengl();
bool init_shader();
void free_gl_resources();

// gets called when the color palette changes
void on_palette_change_gl();

void report_mpeg_dimensions_GL_callback(int width, int height);
int prepare_frame_GL_callback(struct yuv_buf *buf);
void display_frame_GL_callback(struct yuv_buf *buf);
void render_blank_frame_GL_callback();

#endif // USE_OPENGL

#endif // LDP_VLDP_GL_H

