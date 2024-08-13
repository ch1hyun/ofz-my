#include <fuzzer/FuzzedDataProvider.h>
#include "PDFDoc.h"
#include "SplashOutputDev.h"
#include "GString.h"
#include "GlobalParams.h"

struct InfiniteLoop {
    int field1;
    int field2;
    double field3;
};

GBool abortCheckCbk(void *data) {
    return gFalse;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    FuzzedDataProvider fuzzed_data_provider(data, size);

    // Initialise non-integer parameters.
    SplashColor paperColor;
    paperColor[0] = 255;
    paperColor[1] = 255;
    paperColor[2] = 255;

    // Initialize GlobalParams before creating SplashOutputDev
    globalParams = new GlobalParams("dummy.cfg");

    SplashOutputDev *outputDev = new SplashOutputDev(splashModeRGB8, 4, gFalse, paperColor);
    GBool gBool1 = fuzzed_data_provider.ConsumeBool();
    GBool gBool2 = fuzzed_data_provider.ConsumeBool();
    GBool gBool3 = fuzzed_data_provider.ConsumeBool();
    void *displayParam = static_cast<void*>(outputDev); 

    // Initialise data for infinite loop object.
    InfiniteLoop infLoop;
    infLoop.field1 = fuzzed_data_provider.ConsumeIntegral<int>();
    infLoop.field2 = fuzzed_data_provider.ConsumeIntegral<int>();
    infLoop.field3 = fuzzed_data_provider.ConsumeFloatingPoint<double>();

    if (!fuzzed_data_provider.remaining_bytes()) {
    	delete outputDev;
        delete globalParams;
    	return 0;
    }

    // Consume the data to integer type parameters.
    int intParam1 = fuzzed_data_provider.ConsumeIntegral<int>();
    double doubleParam1 = fuzzed_data_provider.ConsumeFloatingPoint<double>();
    double doubleParam2 = fuzzed_data_provider.ConsumeFloatingPoint<double>();
    int intParam2 = fuzzed_data_provider.ConsumeIntegral<int>();
    int intParam3 = fuzzed_data_provider.ConsumeIntegral<int>();
    int intParam4 = fuzzed_data_provider.ConsumeIntegral<int>();
    int intParam5 = fuzzed_data_provider.ConsumeIntegral<int>();
    int intParam6 = fuzzed_data_provider.ConsumeIntegral<int>();


    // Function Call to fuzz target.
    GString fileNameA("dummy.pdf");
    PDFDoc pdfDoc(&fileNameA); 
    pdfDoc.displayPageSlice(outputDev, intParam1, doubleParam1, doubleParam2, intParam2, gBool1, gBool2, gBool3, intParam3, intParam4, intParam5, intParam6, abortCheckCbk, displayParam);

    delete outputDev;
    delete globalParams;
    return 0;
}
