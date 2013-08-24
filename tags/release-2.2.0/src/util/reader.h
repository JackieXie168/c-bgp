// ==================================================================
// @(#)reader.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 22/12/2008
// $Id: reader.h,v 1.1 2009-08-31 09:57:31 bqu Exp $
// ==================================================================

#ifndef __UTIL_READER_H__
#define __UTIL_READER_H__

struct reader_t;
typedef struct reader_t reader_t;

#ifdef _cplusplus
extern "C" {
#endif

  // -----[ file_reader ]--------------------------------------------
  reader_t * file_reader(const char * filename);

  // -----[ stream_reader ]------------------------------------------
  reader_t * stream_reader(FILE * stream);

  // -----[ reader_close ]-------------------------------------------
  void reader_close(reader_t * reader);

  // -----[ reader_destroy ]-----------------------------------------
  void reader_destroy(reader_t ** reader_ref);

  // -----[ reader_eof ]---------------------------------------------
  int reader_eof(reader_t * reader);

  // -----[ reader_flush ]-------------------------------------------
  void reader_flush(reader_t * reader);

  // -----[ reader_gets ]--------------------------------------------
  char * reader_gets(reader_t * reader, char * buf, size_t buf_size);

#ifdef _cplusplus
}
#endif

#endif /* __UTIL_READER_H__ */
