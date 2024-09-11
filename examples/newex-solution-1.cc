#include <stdint.h>
#include <cstdlib>
#include <string>
#include <fuzzer/FuzzedDataProvider.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  if (Size < 1)
    return 0;
  FuzzedDataProvider stream(Data, Size);
  const size_t arr_size = stream.ConsumeIntegralInRange<uint8_t>(1, Size > 128 ? 128 : Size);
  const std::string str = stream.ConsumeRandomLengthString();
  unsigned int values[arr_size];
  double dvalue;
  memcpy(&dvalue, str.data(), str.size());

  return 0;
}
