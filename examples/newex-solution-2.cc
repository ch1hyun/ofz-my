#include <fuzzer/FuzzedDataProvider.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "ivorbisfile.h"

// vorbis_data struct for the ov_callbacks
struct vorbis_data {
  const uint8_t *current;
  const uint8_t *data;
  size_t size;
};

// Proxy read function for ov_callbacks
size_t read_func(void *ptr, size_t size1, size_t size2, void *datasource) {
  vorbis_data *vd = (vorbis_data *)datasource;
  size_t len = size1 * size2;
  if (vd->current + len > vd->data + vd->size) {
    len = vd->data + vd->size - vd->current;
  }
  memcpy(ptr, vd->current, len);
  vd->current += len;
  return len;
} 

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  FuzzedDataProvider stream(data, size);
  
  // Create a new vorbis file with callbacks to our fuzzed data
  OggVorbis_File vf;
  ov_callbacks callbacks;
  callbacks.read_func = read_func;
  callbacks.seek_func = nullptr;
  callbacks.tell_func = nullptr;
  callbacks.close_func = nullptr;

  vorbis_data vd;
  auto remaining_bytes = stream.ConsumeRemainingBytes<uint8_t>();
  vd.data = remaining_bytes.data();
  vd.current = vd.data;
  vd.size = remaining_bytes.size();

  if (ov_open_callbacks(&vd, &vf, NULL, 0, callbacks) < 0) {
    return 0; // Unable to create a vorbis_file from the data
  }

  // Calls the function-under-test
  ov_test_open(&vf);

  ov_clear(&vf);

  return 0;
}
