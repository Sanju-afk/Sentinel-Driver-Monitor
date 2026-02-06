cmake_minimum_required(VERSION 3.10)
project(Sentinel)

set(CMAKE_CXX_STANDARD 17)

# --- DEPENDENCIES ---
find_package(OpenCV REQUIRED)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# --- TENSORFLOW LITE SETUP ---
# Update this path if your tensorflow folder is elsewhere
set(TFLITE_DIR "/home/sanju/tf_build/tensorflow/tensorflow/lite") 
include_directories(${TFLITE_DIR}/../../) 
include_directories(${TFLITE_DIR}/tools/make/downloads/flatbuffers/include)
include_directories(${TFLITE_DIR}/tools/make/downloads/absl)
include_directories("/home/sanju/tf_build/tensorflow/tflite_build/flatbuffers/include")

# --- SOURCE FILES ---
include_directories(include)

# Use explicit paths instead of GLOB for reliability
set(SOURCES 
    src/main.cpp 
    src/InferenceEngine.cpp
)

# --- EXECUTABLE ---
add_executable(Sentinel ${SOURCES})

# --- LINK LIBRARIES ---
# Note: We need to link all the TFLite static libraries we built earlier
# You might need to adjust the path '/home/sanju/tf_build/...' to match your previous project
# --- LINKING ---

# --- LINKING ---
# --- LINKING ---
target_link_libraries(Sentinel
    ${OpenCV_LIBS}
    ${CURL_LIBRARIES}
    Threads::Threads
    
    # TFLite Main Lib
    /home/sanju/tf_build/tensorflow/tflite_build/libtensorflow-lite.a
    
    # Dependencies
    # REMOVED conflicting fft4f2d.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/fft2d-build/libfft2d_fftsg.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/fft2d-build/libfft2d_fftsg2d.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/flatbuffers-build/libflatbuffers.a
    
    # --- ADDED PTHREADPOOL ---
   /home/sanju/tf_build/tensorflow/tflite_build/pthreadpool/libpthreadpool.a

    # --- RUY LIBRARIES (Keep exactly as they were) ---
    # Core
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_context.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_context_get_ctx.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_ctx.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_allocator.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_frontend.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_system_aligned_alloc.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_thread_pool.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_blocking_counter.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_prepare_packed_matrices.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_trmul.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_tune.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_wait.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_cpuinfo.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_denormal.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_block_map.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_apply_multiplier.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_prepacked_cache.a

    # AVX (Standard)
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_kernel_avx.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_pack_avx.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_have_built_path_for_avx.a

    # AVX2 FMA (High Performance)
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_kernel_avx2_fma.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_pack_avx2_fma.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_have_built_path_for_avx2_fma.a

    # AVX 512 (Ultra High Performance)
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_kernel_avx512.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_pack_avx512.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/ruy-build/ruy/libruy_have_built_path_for_avx512.a
    
    # Other Dependencies	
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/cpuinfo-build/libcpuinfo.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/xnnpack-build/libXNNPACK.a
    /home/sanju/tf_build/tensorflow/tflite_build/_deps/farmhash-build/libfarmhash.a
    dl
)
