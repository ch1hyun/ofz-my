#include "ivorbisfile.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string.h>

#define INPUT_LIMIT 16384

struct vorbis_data {
  const uint8_t *current;
  const uint8_t *data;
  size_t size;
};

size_t read_func(void *ptr, size_t size1, size_t size2, void *datasource) {
  vorbis_data *vd = (vorbis_data *)(datasource);
  size_t len = size1 * size2;
  if (vd->current + len > vd->data + vd->size) {
    len = vd->data + vd->size - vd->current;
  }
  memcpy(ptr, vd->current, len);
  vd->current += len;
  return len;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  ov_callbacks memory_callbacks = {0};
  memory_callbacks.read_func = read_func;
  vorbis_data data_st;
  data_st.size = Size > INPUT_LIMIT ? INPUT_LIMIT : Size;
  data_st.current = Data;
  data_st.data = Data;
  OggVorbis_File vf;
  int result = ov_open_callbacks(&data_st, &vf, NULL, 0, memory_callbacks);
  
  if (result < 0) {
    return 0;
  }
  
  ogg_int64_t pcm_offset = static_cast<ogg_int64_t>(result);
  int seek_result = ov_pcm_seek(&vf, pcm_offset);
  
  ov_clear(&vf);
  
  return 0;
}
