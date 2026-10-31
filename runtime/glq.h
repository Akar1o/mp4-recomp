/* SwitchRecomp - Copyright 2026 MZ Labs
   Licensed under the Apache License 2.0. See LICENSE.txt and NOTICE.txt;
   any copy or derived work must keep the credit to SwitchRecomp by MZ Labs. */
/* GL call queue: the GPU thread records calls, a dedicated thread replays them */
#ifndef SWR_GLQ_H
#define SWR_GLQ_H
int glq_start(void);                         /* starts the GL thread and makes its context, 0 if that failed */
int glq_active(void);
void* glq_proc(const char* name);           /* like backend_gl_proc but hands out the queued wrappers */
void glq_swap(void);                        /* queued swap, never more than one frame in flight */
int glq_worker_current(void);               /* worker context current + bypass the queue on this thread */
void glq_finish_all(void);                  /* block until the GL thread drained the queue */
#include <stdint.h>
void glq_read_pixels_pbo(GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum t, uintptr_t off);   /* with a pack buffer bound, never waits */
void glq_get_tex_image_pbo(GLenum tg, GLint lv, GLenum f, GLenum t, uintptr_t off);
void glq_compressed_sub_pbo(int is3d, GLenum tg, GLint lv, GLint x, GLint y, GLint z, GLsizei w, GLsizei h, GLsizei d, GLenum f, GLsizei n, uintptr_t off);
#endif
